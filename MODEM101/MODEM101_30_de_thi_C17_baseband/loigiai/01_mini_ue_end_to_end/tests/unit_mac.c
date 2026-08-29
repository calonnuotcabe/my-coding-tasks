#include "bb/bb.h"
#include "bb/mac.h"
#include "bb/protocol.h"
#include "internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

_Alignas(max_align_t) static uint8_t arena[1024U * 1024U];

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

int main(void)
{
    const uint8_t first_data[3] = { 1U, 2U, 3U };
    const uint8_t second_data[5] = { 4U, 5U, 6U, 7U, 8U };
    const struct bb_mac_sdu source[2] = {
        { (uint8_t)BB_MAC_LCID_CONTROL, first_data, 3U },
        { (uint8_t)BB_MAC_LCID_DATA, second_data, 5U }
    };
    struct bb_mac_sdu decoded[BB_MAC_MAX_SDUS];
    uint8_t transport_block[BB_MAX_TB_BYTES];
    size_t transport_length = 0U;
    size_t decoded_count = 0U;
    struct bb_config config = {
        7680000U, 1U, 0, 0U, 42U, 0U,
        (uint8_t)BB_MOD_QPSK, 0U, NULL, NULL
    };
    const struct bb_platform_ops ops = {
        test_ticks, test_mmio_read, test_mmio_write,
        test_cache_clean, test_cache_invalidate
    };
    struct bb_modem *modem = NULL;
    const uint8_t cell_ie[5] = { 1U, 2U, 0U, 42U, 0U };
    const struct bb_wire_pdu controls[4] = {
        { (uint8_t)BB_PDU_CELL_BEACON, 0U, 0U, 0U, 0U,
          0U, 0U, cell_ie, 5U },
        { (uint8_t)BB_PDU_CELL_BEACON, 0U, 0U, 0U, 0U,
          0U, 0U, cell_ie, 5U },
        { (uint8_t)BB_PDU_RRC_SETUP, 0U, 0U, 0U, 0U,
          0U, 0U, NULL, 0U },
        { (uint8_t)BB_PDU_RRC_SETUP, 0U, 0U, 0U, 0U,
          0U, 0U, NULL, 0U }
    };
    uint8_t encoded[4][BB_MAX_TB_BYTES];
    size_t encoded_length[4] = { 0U, 0U, 0U, 0U };
    struct bb_mac_sdu control_sdus[4];
    size_t index;

    if ((bb_mac_multiplex(source, 2U, transport_block,
                          sizeof(transport_block), &transport_length) != 0) ||
        (bb_mac_demultiplex(transport_block, transport_length, decoded,
                            BB_MAC_MAX_SDUS, &decoded_count) != 0) ||
        (decoded_count != 2U) || (decoded[0].length != 3U) ||
        (decoded[1].length != 5U)) {
        (void)fprintf(stderr, "MAC two-LCID round trip failed\n");
        return 1;
    }
    for (index = 0U; index < decoded[0].length; ++index) {
        if (decoded[0].data[index] != first_data[index]) return 1;
    }
    for (index = 0U; index < decoded[1].length; ++index) {
        if (decoded[1].data[index] != second_data[index]) return 1;
    }
    transport_block[6] = 1U;
    if (bb_mac_demultiplex(transport_block, transport_length, decoded,
                           BB_MAC_MAX_SDUS, &decoded_count) == 0) {
        (void)fprintf(stderr, "MAC noncanonical padding accepted\n");
        return 1;
    }
    transport_block[6] = 0U;
    if ((bb_mac_demultiplex(transport_block, transport_length - 1U, decoded,
                            BB_MAC_MAX_SDUS, &decoded_count) == 0) ||
        (bb_mac_demultiplex(transport_block, transport_length + 1U, decoded,
                            BB_MAC_MAX_SDUS, &decoded_count) == 0)) {
        (void)fprintf(stderr, "MAC length mismatch accepted\n");
        return 1;
    }

    if (bb_modem_init(arena, sizeof(arena), &config, &ops, NULL, &modem) != 0) {
        return 1;
    }
    for (index = 0U; index < 4U; ++index) {
        if (bb_protocol_encode(&controls[index], encoded[index],
                               sizeof(encoded[index]), &encoded_length[index],
                               false) != 0) {
            return 1;
        }
        control_sdus[index].lcid = (uint8_t)BB_MAC_LCID_CONTROL;
        control_sdus[index].data = encoded[index];
        control_sdus[index].length = (uint16_t)encoded_length[index];
    }
    if ((bb_mac_multiplex(control_sdus, 4U, transport_block,
                          sizeof(transport_block), &transport_length) != 0) ||
        (bb_modem_send_tb(modem, transport_block, transport_length, 100U) != 0)) {
        return 1;
    }
    /* Feed the same multiplexed TB down the receive stack. */
    if ((bb_stack_process_pdu(modem, transport_block, transport_length,
                              100U) != 0) ||
        (bb_modem_run_until(modem, 100U) != 0)) {
        return 1;
    }
    {
        struct bb_session_status status;
        bb_modem_get_session_status(modem, &status);
        if ((status.rrc != BB_RRC_CONNECTED) ||
            (status.nas != BB_NAS_REGISTERING)) {
            (void)fprintf(stderr, "MAC multiplexed stack dispatch failed\n");
            return 1;
        }
    }
    (void)printf("MAC multiplex/parser tests passed\n");
    return 0;
}
