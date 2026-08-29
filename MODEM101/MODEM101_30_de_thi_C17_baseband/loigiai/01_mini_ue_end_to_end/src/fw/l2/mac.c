#include "internal.h"

static void put_u16(uint8_t *output, uint16_t value)
{
    output[0] = (uint8_t)value;
    output[1] = (uint8_t)(value >> 8U);
}

static uint16_t get_u16(const uint8_t *input)
{
    return (uint16_t)((uint16_t)input[0] | ((uint16_t)input[1] << 8U));
}

int bb_mac_multiplex(const struct bb_mac_sdu *sdus, size_t count,
                     uint8_t *output, size_t capacity, size_t *length)
{
    size_t position = BB_MAC_HEADER_SIZE;
    size_t sdu_index;
    if ((sdus == NULL) || (output == NULL) || (length == NULL) ||
        (count == 0U) || (count > BB_MAC_MAX_SDUS) ||
        (capacity < BB_MAC_HEADER_SIZE)) {
        return -1;
    }
    output[0] = (uint8_t)'B';
    output[1] = (uint8_t)'M';
    output[2] = (uint8_t)'A';
    output[3] = (uint8_t)'C';
    output[4] = 1U;
    output[5] = (uint8_t)count;
    output[6] = 0U;
    output[7] = 0U;
    for (sdu_index = 0U; sdu_index < count; ++sdu_index) {
        size_t byte_index;
        const struct bb_mac_sdu *const sdu = &sdus[sdu_index];
        if ((sdu->data == NULL) || (sdu->length == 0U) ||
            ((sdu->lcid != (uint8_t)BB_MAC_LCID_CONTROL) &&
             (sdu->lcid != (uint8_t)BB_MAC_LCID_DATA)) ||
            (position > capacity) ||
            ((size_t)sdu->length > capacity - position) ||
            (BB_MAC_SUBHEADER_SIZE > capacity - position) ||
            ((size_t)sdu->length >
             capacity - position - BB_MAC_SUBHEADER_SIZE)) {
            return -2;
        }
        output[position] = sdu->lcid;
        output[position + 1U] = 0U;
        put_u16(&output[position + 2U], sdu->length);
        position += BB_MAC_SUBHEADER_SIZE;
        for (byte_index = 0U; byte_index < sdu->length; ++byte_index) {
            output[position + byte_index] = sdu->data[byte_index];
        }
        position += sdu->length;
    }
    if (position > BB_MAX_TB_BYTES) {
        return -2;
    }
    *length = position;
    return 0;
}

int bb_mac_demultiplex(const uint8_t *input, size_t length,
                       struct bb_mac_sdu *sdus, size_t capacity,
                       size_t *count)
{
    size_t position = BB_MAC_HEADER_SIZE;
    size_t sdu_index;
    size_t encoded_count;
    if ((input == NULL) || (sdus == NULL) || (count == NULL) ||
        (length < BB_MAC_HEADER_SIZE) || (length > BB_MAX_TB_BYTES) ||
        (input[0] != (uint8_t)'B') || (input[1] != (uint8_t)'M') ||
        (input[2] != (uint8_t)'A') || (input[3] != (uint8_t)'C') ||
        (input[4] != 1U) || (input[6] != 0U) || (input[7] != 0U)) {
        return -1;
    }
    encoded_count = input[5];
    if ((encoded_count == 0U) || (encoded_count > BB_MAC_MAX_SDUS) ||
        (encoded_count > capacity)) {
        return -2;
    }
    for (sdu_index = 0U; sdu_index < encoded_count; ++sdu_index) {
        uint16_t sdu_length;
        if ((position > length) ||
            (BB_MAC_SUBHEADER_SIZE > length - position)) {
            return -3;
        }
        sdu_length = get_u16(&input[position + 2U]);
        if (((input[position] != (uint8_t)BB_MAC_LCID_CONTROL) &&
             (input[position] != (uint8_t)BB_MAC_LCID_DATA)) ||
            (input[position + 1U] != 0U) || (sdu_length == 0U) ||
            ((size_t)sdu_length >
             length - position - BB_MAC_SUBHEADER_SIZE)) {
            return -4;
        }
        sdus[sdu_index].lcid = input[position];
        sdus[sdu_index].length = sdu_length;
        sdus[sdu_index].data = &input[position + BB_MAC_SUBHEADER_SIZE];
        position += BB_MAC_SUBHEADER_SIZE + sdu_length;
    }
    if (position != length) {
        return -5;
    }
    *count = encoded_count;
    return 0;
}

static int mac_process_tb(struct bb_modem *modem, const uint8_t *data,
                          size_t length, uint64_t tick, bool crypto_verified)
{
    struct bb_mac_sdu sdus[BB_MAC_MAX_SDUS];
    struct bb_wire_pdu pdus[BB_MAC_MAX_SDUS];
    size_t count = 0U;
    size_t index;
    uint8_t require_mask = 0U;
    if (bb_mac_demultiplex(data, length, sdus, BB_MAC_MAX_SDUS,
                           &count) != 0) {
        modem->stats.dropped++;
        bb_trace(&modem->cfg, tick, "MAC_MALFORMED_DROP", (int32_t)length);
        return 0;
    }
    /* Validate the complete TB before mutating HARQ state or publishing any
     * child message, so a malformed later sub-PDU cannot partially commit. */
    for (index = 0U; index < count; ++index) {
        uint32_t budget = 1000U;
        if (sdus[index].length < BB_WIRE_HEADER_SIZE) {
            modem->stats.dropped++;
            bb_trace(&modem->cfg, tick, "PDU_MALFORMED_DROP", -1);
            return 0;
        }
        if (modem->stack.security_active &&
            bb_protocol_type_requires_integrity(sdus[index].data[5])) {
            require_mask |= (uint8_t)(1U << index);
        }
        if (bb_protocol_decode(sdus[index].data, sdus[index].length,
                               &pdus[index], false,
                               &budget) != 0) {
            modem->stats.dropped++;
            bb_trace(&modem->cfg, tick, "PDU_MALFORMED_DROP", -1);
            return 0;
        }
        if (((pdus[index].type == (uint8_t)BB_PDU_DATA) &&
             (sdus[index].lcid != (uint8_t)BB_MAC_LCID_DATA)) ||
            ((pdus[index].type != (uint8_t)BB_PDU_DATA) &&
             (sdus[index].lcid != (uint8_t)BB_MAC_LCID_CONTROL))) {
            modem->stats.dropped++;
            bb_trace(&modem->cfg, tick, "MAC_LCID_DROP", sdus[index].lcid);
            return 0;
        }
    }
    if (!crypto_verified && (require_mask != 0U)) {
        if (bb_crypto_submit_mac(modem, data, length, require_mask, tick) != 0) {
            modem->stats.dropped++;
            bb_trace(&modem->cfg, tick, "CRYPTO_QUEUE_FULL", require_mask);
        }
        return 0;
    }
    bb_trace(&modem->cfg, tick, "MAC_MULTIPLEX_COUNT", (int32_t)count);
    for (index = 0U; index < count; ++index) {
        uint16_t next_stage = (uint16_t)BB_EVENT_CONTROL_RX;
        bb_trace(&modem->cfg, tick, "MAC_DEMUX", pdus[index].type);
        if (pdus[index].type == (uint8_t)BB_PDU_DATA) {
            if (!modem->stack.security_active) {
                modem->stats.dropped++;
                bb_trace(&modem->cfg, tick, "DATA_WRONG_STATE",
                         pdus[index].type);
                continue;
            }
            bb_mac_harq_accept(modem, &pdus[index], tick);
            next_stage = (uint16_t)BB_EVENT_RLC_RX;
        }
        if (bb_modem_enqueue_stage_copy(modem, sdus[index].data,
                                        sdus[index].length, tick,
                                        next_stage) != 0) {
            modem->stats.dropped++;
            bb_trace(&modem->cfg, tick, "MAC_ENQUEUE_DROP",
                     (int32_t)index);
        }
    }
    return 0;
}

int bb_mac_process_tb(struct bb_modem *modem, const uint8_t *data,
                      size_t length, uint64_t tick)
{
    return mac_process_tb(modem, data, length, tick, false);
}

int bb_mac_process_tb_verified(struct bb_modem *modem, const uint8_t *data,
                               size_t length, uint64_t tick)
{
    return mac_process_tb(modem, data, length, tick, true);
}

int bb_mac_send_control(struct bb_modem *modem, uint8_t type,
                        uint8_t harq_id, uint32_t count, uint64_t tick)
{
    struct bb_wire_pdu pdu;
    uint8_t encoded[BB_MAX_TB_BYTES];
    uint8_t transport_block[BB_MAX_TB_BYTES];
    struct bb_mac_sdu sdu;
    size_t encoded_length = 0U;
    size_t transport_length = 0U;
    int result;
    pdu.type = type;
    pdu.harq_id = harq_id;
    pdu.flags = modem->stack.security_active ? 2U : 0U;
    pdu.rlc_sn = 0U;
    pdu.pdcp_count = count;
    pdu.offset = 0U;
    pdu.total_length = 0U;
    pdu.payload = NULL;
    pdu.payload_length = 0U;
    /* Protected uplink tags are filled by the descriptor-backed accelerator,
     * never by the firmware control task itself. */
    if (bb_protocol_encode(&pdu, encoded, sizeof(encoded), &encoded_length,
                           false) != 0) {
        return -1;
    }
    sdu.lcid = (uint8_t)BB_MAC_LCID_CONTROL;
    sdu.data = encoded;
    sdu.length = (uint16_t)encoded_length;
    if (bb_mac_multiplex(&sdu, 1U, transport_block,
                         sizeof(transport_block), &transport_length) != 0) {
        return -1;
    }
    if (UINT32_MAX - modem->stack.bsr_bytes < transport_length) {
        return -1;
    }
    modem->stack.bsr_bytes += (uint32_t)transport_length;
    bb_trace(&modem->cfg, tick, "MAC_BSR", (int32_t)modem->stack.bsr_bytes);
    if (modem->stack.security_active) {
        result = bb_crypto_submit_mac_sign(modem, transport_block,
                                           transport_length, 1U, tick);
    } else {
        result = bb_modem_send_tb(modem, transport_block, transport_length,
                                  tick);
    }
    if (result != 0) {
        modem->stack.bsr_bytes -= (uint32_t)transport_length;
    }
    return result;
}

void bb_mac_harq_accept(struct bb_modem *modem,
                        const struct bb_wire_pdu *pdu, uint64_t tick)
{
    struct bb_harq_process *const process = &modem->stack.harq[pdu->harq_id];
    const uint32_t full_count =
        bb_pdcp_reconstruct_count(&modem->stack, pdu->pdcp_count);
    if ((process->state == (uint8_t)BB_HARQ_WAITING) &&
        (process->pdcp_count == full_count)) {
        process->state = (uint8_t)BB_HARQ_IDLE;
        process->timer_generation++;
        process->soft_length = 0U;
        if (modem->stack.active_harq_valid &&
            (modem->stack.active_harq_id == pdu->harq_id)) {
            modem->stack.active_harq_valid = false;
        }
        if (((pdu->flags & 1U) != 0U) || (process->transmissions != 0U)) {
            modem->stack.harq_retx++;
            bb_trace(&modem->cfg, tick, "HARQ_RETX", pdu->harq_id);
        }
    }
}
