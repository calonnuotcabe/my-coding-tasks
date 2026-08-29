#include "internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

_Alignas(max_align_t) static uint8_t arena[1024U * 1024U];
static struct bb_cpx16 llr_tx_samples[BB_MAX_IQ_SAMPLES];
static struct bb_cpx16 llr_rx_samples[BB_MAX_IQ_SAMPLES];
static int16_t first_reception_llr[BB_MAX_FRAME_BITS];
static int16_t second_reception_llr[BB_MAX_FRAME_BITS];

static uint64_t test_ticks(void *context)
{
    (void)context;
    return 0U;
}

static uint32_t test_mmio_read(void *context, uintptr_t address)
{
    (void)context;
    (void)address;
    return 0U;
}

static void test_mmio_write(void *context, uintptr_t address, uint32_t value)
{
    (void)context;
    (void)address;
    (void)value;
}

static void test_cache_clean(void *context, const void *address, size_t length)
{
    (void)context;
    (void)address;
    (void)length;
}

static void test_cache_invalidate(void *context, void *address, size_t length)
{
    (void)context;
    (void)address;
    (void)length;
}

static int process_wire(struct bb_modem *modem, const uint8_t *encoded,
                        size_t length, uint64_t tick)
{
    struct bb_mac_sdu sdu;
    uint8_t transport_block[BB_MAX_TB_BYTES];
    size_t transport_length = 0U;
    sdu.lcid = (encoded[5] == (uint8_t)BB_PDU_DATA) ?
               (uint8_t)BB_MAC_LCID_DATA :
               (uint8_t)BB_MAC_LCID_CONTROL;
    sdu.data = encoded;
    sdu.length = (uint16_t)length;
    if (bb_mac_multiplex(&sdu, 1U, transport_block,
                         sizeof(transport_block), &transport_length) != 0) {
        return -1;
    }
    if (bb_stack_process_pdu(modem, transport_block, transport_length,
                             tick) != 0) {
        return -1;
    }
    {
        uint32_t pass;
        for (pass = 0U; pass < 4U; ++pass) {
            const int completed = bb_crypto_accelerator_step(modem);
            if (completed < 0) {
                return -1;
            }
            if (completed == 0) {
                break;
            }
            if ((bb_modem_irq(modem, 13U) != 0) ||
                (bb_modem_run_until(modem, tick) != 0)) {
                return -1;
            }
        }
    }
    return 0;
}

static int test_harq_llr_is_not_precombined(void)
{
    struct bb_config config = {
        7680000U, 9U, 0, 0U, 42U, 0U,
        (uint8_t)BB_MOD_QPSK, 0U, NULL, NULL
    };
    struct bb_iq_block transmit = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, llr_tx_samples
    };
    struct bb_iq_block receive = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, llr_rx_samples
    };
    uint8_t payload[200];
    uint8_t decoded[BB_MAX_TB_BYTES];
    size_t decoded_length = 0U;
    size_t first_length = 0U;
    size_t second_length = 0U;
    uint32_t index;
    for (index = 0U; index < sizeof(payload); ++index) {
        payload[index] = (uint8_t)(index * 13U + 1U);
    }
    if ((bb_phy_transmit(&config, payload, sizeof(payload), &transmit) != 0) ||
        (bb_channel_apply(&config, &transmit, &receive) != 0)) {
        return -1;
    }
    for (index = 3U * (BB_NFFT + BB_CP_LEN); index < receive.count; ++index) {
        receive.samples[index].i = 0;
        receive.samples[index].q = 0;
    }
    if (bb_phy_receive_soft(&config, &receive, decoded, sizeof(decoded),
                            &decoded_length, NULL, 0U, first_reception_llr,
                            BB_MAX_FRAME_BITS, &first_length) != -5) {
        return -1;
    }
    decoded_length = 0U;
    if ((bb_phy_receive_soft(&config, &receive, decoded, sizeof(decoded),
                             &decoded_length, first_reception_llr,
                             first_length, second_reception_llr,
                             BB_MAX_FRAME_BITS, &second_length) != -5) ||
        (first_length != second_length)) {
        return -1;
    }
    for (index = 0U; index < first_length; ++index) {
        if (first_reception_llr[index] != second_reception_llr[index]) {
            return -1;
        }
    }
    return 0;
}

static int send_data(struct bb_modem *modem, uint16_t rlc_sn,
                     uint32_t count, uint16_t offset, uint8_t value,
                     uint64_t tick)
{
    struct bb_wire_pdu pdu = {
        (uint8_t)BB_PDU_DATA, 0U, 0U, rlc_sn, count, offset,
        BB_MAX_SDU_BYTES, &value, 1U
    };
    uint8_t encoded[BB_MAX_TB_BYTES];
    size_t length = 0U;
    if (bb_protocol_encode(&pdu, encoded, sizeof(encoded), &length, true) != 0) {
        return -1;
    }
    if (process_wire(modem, encoded, length, tick) != 0) {
        return -1;
    }
    return bb_modem_run_until(modem, tick);
}

static int send_corrupted_data(struct bb_modem *modem, uint16_t rlc_sn,
                               uint32_t count, uint64_t tick)
{
    const uint8_t value = 0x5AU;
    const struct bb_wire_pdu pdu = {
        (uint8_t)BB_PDU_DATA, 0U, 0U, rlc_sn, count, 3U,
        BB_MAX_SDU_BYTES, &value, 1U
    };
    uint8_t encoded[BB_MAX_TB_BYTES];
    size_t length = 0U;
    if (bb_protocol_encode(&pdu, encoded, sizeof(encoded), &length, true) != 0) {
        return -1;
    }
    encoded[20] ^= 1U;
    if (process_wire(modem, encoded, length, tick) != 0) {
        return -1;
    }
    return bb_modem_run_until(modem, tick);
}

static int submit_corrupted_harq_waveform(struct bb_modem *modem,
                                          const struct bb_config *config,
                                          uint8_t harq_id, uint32_t count,
                                          uint64_t tick)
{
    const uint8_t value = (uint8_t)(0x40U + harq_id);
    const struct bb_wire_pdu pdu = {
        (uint8_t)BB_PDU_DATA, harq_id, 0U, (uint16_t)count, count,
        0U, BB_MAX_SDU_BYTES, &value, 1U
    };
    struct bb_mac_sdu sdu;
    struct bb_iq_block transmit = {
        tick, 7680000U, BB_MAX_IQ_SAMPLES, llr_tx_samples
    };
    struct bb_iq_block receive = {
        tick, 7680000U, BB_MAX_IQ_SAMPLES, llr_rx_samples
    };
    uint8_t encoded[BB_MAX_TB_BYTES];
    uint8_t transport_block[BB_MAX_TB_BYTES];
    size_t encoded_length = 0U;
    size_t transport_length = 0U;
    uint32_t sample;

    if (bb_protocol_encode(&pdu, encoded, sizeof(encoded), &encoded_length,
                           true) != 0) {
        return -1;
    }
    sdu.lcid = (uint8_t)BB_MAC_LCID_DATA;
    sdu.data = encoded;
    sdu.length = (uint16_t)encoded_length;
    if ((bb_mac_multiplex(&sdu, 1U, transport_block,
                          sizeof(transport_block), &transport_length) != 0) ||
        (bb_phy_transmit(config, transport_block, transport_length,
                         &transmit) != 0) ||
        (bb_channel_apply(config, &transmit, &receive) != 0)) {
        return -2;
    }
    /* Preserve the PHY metadata symbols and erase the data tail so that CRC
     * fails while the receiver can still identify the exact HARQ process. */
    for (sample = 3U * (BB_NFFT + BB_CP_LEN); sample < receive.count;
         ++sample) {
        receive.samples[sample].i = 0;
        receive.samples[sample].q = 0;
    }
    if (bb_modem_rx_iq_submit(modem, &receive, tick) != 0) return -3;
    if (bb_modem_run_until(modem, tick) != 0) return -4;
    return 0;
}

int main(void)
{
    struct bb_config config = {
        7680000U, 1U, 0, 0U, 42U, 0U,
        (uint8_t)BB_MOD_QPSK, 0U, NULL, NULL
    };
    const struct bb_platform_ops ops = {
        test_ticks, test_mmio_read, test_mmio_write,
        test_cache_clean, test_cache_invalidate
    };
    struct bb_modem *modem;
    struct bb_wire_pdu control = {
        (uint8_t)BB_PDU_SESSION_ACCEPT, 0U, 0U, 0U, 0U,
        0U, 0U, NULL, 0U
    };
    uint8_t encoded[BB_MAX_TB_BYTES];
    size_t length = 0U;
    if (test_harq_llr_is_not_precombined() != 0) {
        (void)fprintf(stderr, "HARQ raw-LLR regression failed\n");
        return 1;
    }
    if (bb_modem_init(arena, sizeof(arena), &config, &ops, NULL, &modem) != 0) {
        return 1;
    }
    modem->stack.rrc = BB_RRC_CONNECTED;
    modem->stack.nas = BB_NAS_SESSION_ACTIVE;
    modem->stack.security_active = true;
    modem->stack.key_slot = 1U;
    modem->stack.rlc_rx_next = 4095U;
    if ((send_data(modem, 0U, 1U, 1U, 0xBBU, 1U) != 0) ||
        (modem->stack.delivered != 0U) ||
        (send_data(modem, 4095U, 0U, 0U, 0xAAU, 2U) != 0) ||
        (modem->stack.rlc_rx_next != 1U) ||
        (modem->stack.pdcp_rx_next != 2U) ||
        (modem->stack.delivered != 2U) ||
        (modem->stack.reassembly[0] != 0xAAU) ||
        (modem->stack.reassembly[1] != 0xBBU)) {
        (void)fprintf(stderr, "RLC reorder/wrap failed\n");
        return 1;
    }
    if ((send_data(modem, 1U, 0U, 2U, 0xCCU, 3U) != 0) ||
        (modem->stack.replay_drops != 1U) ||
        (modem->stack.delivered != 2U)) {
        (void)fprintf(stderr, "PDCP replay failed\n");
        return 1;
    }
    modem->stack.pdcp_rx_next = UINT32_MAX;
    modem->stack.pdcp_bitmap = 0U;
    modem->stack.pdcp_initialized = true;
    if ((send_data(modem, 2U, 0U, 2U, 0xCCU, 4U) != 0) ||
        (modem->stack.delivered != 2U) ||
        (send_data(modem, 3U, UINT32_MAX, 3U, 0xDDU, 5U) != 0) ||
        (modem->stack.pdcp_rx_next != 1U) ||
        (modem->stack.delivered != 4U)) {
        (void)fprintf(stderr, "PDCP COUNT/HFN wrap failed\n");
        return 1;
    }
    if ((send_corrupted_data(modem, 4U, 1U, 6U) != 0) ||
        (modem->stack.rlc_rx_next != 4U) ||
        (modem->stack.pdcp_rx_next != 1U)) {
        (void)fprintf(stderr, "integrity-before-state failed\n");
        return 1;
    }
    if ((send_data(modem, 4U, 1U, 0U, 0xEEU, 7U) != 0) ||
        (modem->stack.reassembly[0] != 0xAAU) ||
        (modem->stack.delivered != 4U) ||
        (modem->stack.pdcp_rx_next != 1U)) {
        (void)fprintf(stderr, "conflicting overlap handling failed\n");
        return 1;
    }
    if ((send_data(modem, 5U, 1U, 4U, 0x11U, 8U) != 0) ||
        (send_data(modem, 6U, 3U, 6U, 0x33U, 9U) != 0) ||
        (modem->stack.delivered != 5U) ||
        (send_data(modem, 7U, 2U, 5U, 0x22U, 10U) != 0) ||
        (modem->stack.pdcp_rx_next != 4U) ||
        (modem->stack.delivered != 7U)) {
        (void)fprintf(stderr, "PDCP reorder release failed\n");
        return 1;
    }
    if ((send_data(modem, 10U, 4U, 7U, 0x44U, 20U) != 0) ||
        !modem->stack.rlc_reorder_active) {
        (void)fprintf(stderr, "RLC timer setup failed\n");
        return 1;
    }
    bb_stack_check_timers(modem, 20U + BB_REORDER_TIMEOUT_TICKS);
    if ((bb_modem_run_until(modem, 20U + BB_REORDER_TIMEOUT_TICKS) != 0) ||
        (modem->stack.rlc_rx_next != 11U) ||
        (modem->stack.pdcp_rx_next != 5U) ||
        (modem->stack.reassembly[7] != 0x44U)) {
        (void)fprintf(stderr, "RLC reorder timer failed\n");
        return 1;
    }
    if ((send_data(modem, 11U, 7U, 8U, 0x77U, 1600U) != 0) ||
        !modem->stack.pdcp_reorder_active) {
        (void)fprintf(stderr, "PDCP timer setup failed\n");
        return 1;
    }
    bb_stack_check_timers(modem, 1600U + BB_REORDER_TIMEOUT_TICKS);
    if ((bb_modem_run_until(modem, 1600U + BB_REORDER_TIMEOUT_TICKS) != 0) ||
        (modem->stack.pdcp_rx_next != 8U) ||
        (modem->stack.reassembly[8] != 0x77U)) {
        (void)fprintf(stderr, "PDCP reorder timer failed\n");
        return 1;
    }
    bb_stack_init(&modem->stack);
    if ((bb_protocol_encode(&control, encoded, sizeof(encoded), &length,
                            false) != 0) ||
        (process_wire(modem, encoded, length, 3200U) != 0) ||
        (modem->stack.nas != BB_NAS_DEREGISTERED)) {
        (void)fprintf(stderr, "wrong-state guard failed\n");
        return 1;
    }
    modem->stack.rrc = BB_RRC_CONNECTED;
    modem->stack.nas = BB_NAS_REGISTERING;
    modem->stack.security_active = true;
    modem->stack.key_slot = 1U;
    control.type = (uint8_t)BB_PDU_REG_ACCEPT;
    if ((bb_protocol_encode(&control, encoded, sizeof(encoded), &length,
                            false) != 0) ||
        (process_wire(modem, encoded, length, 3300U) != 0) ||
        (modem->stack.nas != BB_NAS_REGISTERING)) {
        (void)fprintf(stderr, "unsigned protected control accepted\n");
        return 1;
    }
    {
        struct bb_wire_pdu grant_one = {
            (uint8_t)BB_PDU_DATA_GRANT, 1U, 0U, 0U, 8U,
            0U, 0U, NULL, 0U
        };
        struct bb_wire_pdu grant_two = {
            (uint8_t)BB_PDU_DATA_GRANT, 2U, 0U, 0U, 9U,
            0U, 0U, NULL, 0U
        };
        if (bb_modem_init(arena, sizeof(arena), &config, &ops, NULL,
                          &modem) != 0) {
            return 1;
        }
        modem->stack.rrc = BB_RRC_CONNECTED;
        modem->stack.nas = BB_NAS_SESSION_ACTIVE;
        modem->stack.security_active = true;
        modem->stack.key_slot = 1U;
        const int grant_one_result =
            bb_control_process(modem, &grant_one, 3320U);
        const int grant_two_result =
            bb_control_process(modem, &grant_two, 3321U);
        const int first_result = submit_corrupted_harq_waveform(
            modem, &config, 1U, 8U, 3330U);
        const uint16_t first_one = modem->stack.harq[1].soft_length;
        const uint16_t first_two = modem->stack.harq[2].soft_length;
        const int second_result = submit_corrupted_harq_waveform(
            modem, &config, 2U, 9U, 3340U);
        if ((grant_one_result != 0) || (grant_two_result != 0) ||
            (first_result != 0) || (first_one == 0U) || (first_two != 0U) ||
            (second_result != 0) ||
            (modem->stack.harq[1].soft_length == 0U) ||
            (modem->stack.harq[2].soft_length == 0U)) {
            (void)fprintf(stderr, "interleaved HARQ soft-buffer failed\n");
            (void)fprintf(stderr, "grant=%d/%d rx=%d/%d soft=%u/%u then %u/%u\n",
                          grant_one_result, grant_two_result, first_result,
                          second_result, (unsigned)first_one,
                          (unsigned)first_two,
                          (unsigned)modem->stack.harq[1].soft_length,
                          (unsigned)modem->stack.harq[2].soft_length);
            return 1;
        }
    }
    {
        struct bb_wire_pdu grant = {
            (uint8_t)BB_PDU_DATA_GRANT, 7U, 0U, 0U, 99U,
            0U, 0U, NULL, 0U
        };
        struct bb_wire_pdu accepted = {
            (uint8_t)BB_PDU_DATA, 7U, 1U, 0U, 99U,
            0U, BB_MAX_SDU_BYTES, NULL, 0U
        };
        modem->stack.nas = BB_NAS_SESSION_ACTIVE;
        if ((bb_control_process(modem, &grant, 3350U) != 0) ||
            !modem->stack.active_harq_valid ||
            (modem->stack.active_harq_id != 7U) ||
            (modem->stack.harq[7].state != (uint8_t)BB_HARQ_WAITING)) {
            (void)fprintf(stderr, "nonzero HARQ grant association failed\n");
            return 1;
        }
        bb_stack_check_timers(modem, 4350U);
        if ((modem->stack.harq[7].transmissions != 1U) ||
            !modem->stack.active_harq_valid) {
            (void)fprintf(stderr, "nonzero HARQ timer failed\n");
            return 1;
        }
        bb_mac_harq_accept(modem, &accepted, 4351U);
        if ((modem->stack.harq[7].state != (uint8_t)BB_HARQ_IDLE) ||
            modem->stack.active_harq_valid ||
            (modem->stack.harq_retx != 1U)) {
            (void)fprintf(stderr, "nonzero HARQ accept failed\n");
            return 1;
        }
    }
    {
        struct bb_event event = { 4400U, 1U, 1U, { 0U, 0U, 0U } };
        modem->task_info[7].guard_low = 0U;
        if ((bb_modem_post(modem, &event) != 0) ||
            (bb_modem_run_until(modem, 4400U) == 0) ||
            !modem->supervisor_fault) {
            (void)fprintf(stderr, "stack guard supervisor fault failed\n");
            return 1;
        }
    }
    (void)printf("L2 reorder/replay/state tests passed\n");
    return 0;
}
