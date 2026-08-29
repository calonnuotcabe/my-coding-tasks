#include "internal.h"

static void put_u16(uint8_t *output, uint16_t value)
{
    output[0] = (uint8_t)value;
    output[1] = (uint8_t)(value >> 8U);
}

static void put_u32(uint8_t *output, uint32_t value)
{
    output[0] = (uint8_t)value;
    output[1] = (uint8_t)(value >> 8U);
    output[2] = (uint8_t)(value >> 16U);
    output[3] = (uint8_t)(value >> 24U);
}

static uint16_t get_u16(const uint8_t *input)
{
    return (uint16_t)((uint16_t)input[0] | ((uint16_t)input[1] << 8U));
}

static uint32_t get_u32(const uint8_t *input)
{
    return (uint32_t)input[0] | ((uint32_t)input[1] << 8U) |
           ((uint32_t)input[2] << 16U) | ((uint32_t)input[3] << 24U);
}

bool bb_protocol_type_requires_integrity(uint8_t type)
{
    return (type == (uint8_t)BB_PDU_REG_ACCEPT) ||
           (type == (uint8_t)BB_PDU_SESSION_ACCEPT) ||
           (type == (uint8_t)BB_PDU_DATA_GRANT) ||
           (type == (uint8_t)BB_PDU_DATA) ||
           (type == (uint8_t)BB_PDU_LINK_LOSS) ||
           (type == (uint8_t)BB_PDU_SECURITY_COMPLETE) ||
           (type == (uint8_t)BB_PDU_NACK) ||
           (type == (uint8_t)BB_PDU_DATA_ACK);
}

int bb_protocol_encode(const struct bb_wire_pdu *pdu, uint8_t *output,
                       size_t capacity, size_t *length, bool integrity)
{
    uint8_t authenticated[20U + BB_WIRE_MAX_PAYLOAD];
    uint8_t cmac[16];
    size_t total_length;
    uint32_t index;
    if ((pdu == NULL) || (output == NULL) || (length == NULL) ||
        ((pdu->payload == NULL) && (pdu->payload_length != 0U)) ||
        (pdu->payload_length > BB_WIRE_MAX_PAYLOAD) ||
        (pdu->harq_id >= BB_HARQ_PROCESSES) || (pdu->rlc_sn >= 4096U) ||
        ((pdu->flags & ~3U) != 0U) ||
        (pdu->total_length > BB_MAX_SDU_BYTES)) {
        return -1;
    }
    total_length = BB_WIRE_HEADER_SIZE + pdu->payload_length;
    if (capacity < total_length) {
        return -2;
    }
    output[0] = (uint8_t)'M';
    output[1] = (uint8_t)'1';
    output[2] = (uint8_t)'0';
    output[3] = (uint8_t)'1';
    output[4] = 1U;
    output[5] = pdu->type;
    output[6] = pdu->harq_id;
    output[7] = pdu->flags;
    put_u16(&output[8], pdu->rlc_sn);
    put_u16(&output[10], pdu->payload_length);
    /* PDCP transmits only the 18-bit sequence number.  HFN is receiver state
     * and must never be leaked as a full COUNT on the wire. */
    put_u32(&output[12], pdu->pdcp_count & BB_PDCP_SN_MASK);
    put_u16(&output[16], pdu->offset);
    put_u16(&output[18], pdu->total_length);
    for (index = 0U; index < 8U; ++index) {
        output[20U + index] = 0U;
    }
    for (index = 0U; index < pdu->payload_length; ++index) {
        output[BB_WIRE_HEADER_SIZE + index] = pdu->payload[index];
    }
    if (integrity) {
        for (index = 0U; index < 20U; ++index) {
            authenticated[index] = output[index];
        }
        for (index = 0U; index < pdu->payload_length; ++index) {
            authenticated[20U + index] = pdu->payload[index];
        }
        if (bb_security_nia2(1U, pdu->pdcp_count, 1U,
                             (uint8_t)((pdu->flags >> 1U) & 1U),
                             authenticated, 20U + pdu->payload_length,
                             cmac) != 0) {
            return -3;
        }
        for (index = 0U; index < 8U; ++index) {
            output[20U + index] = cmac[index];
        }
    }
    *length = total_length;
    return 0;
}

int bb_protocol_decode_with_count(const uint8_t *input, size_t length,
                                  struct bb_wire_pdu *pdu,
                                  bool require_integrity,
                                  uint32_t integrity_count,
                                  uint32_t *work_budget)
{
    uint8_t authenticated[20U + BB_WIRE_MAX_PAYLOAD];
    uint16_t payload_length;
    uint32_t index;
    uint32_t work;
    if ((input == NULL) || (pdu == NULL) || (work_budget == NULL) ||
        (length < BB_WIRE_HEADER_SIZE) || (length > BB_MAX_TB_BYTES)) {
        return -1;
    }
    work = 20U + (uint32_t)length;
    if (*work_budget < work) {
        return -2;
    }
    *work_budget -= work;
    if ((input[0] != (uint8_t)'M') || (input[1] != (uint8_t)'1') ||
        (input[2] != (uint8_t)'0') || (input[3] != (uint8_t)'1') ||
        (input[4] != 1U)) {
        return -3;
    }
    payload_length = get_u16(&input[10]);
    if (((size_t)payload_length + BB_WIRE_HEADER_SIZE != length) ||
        (input[6] >= BB_HARQ_PROCESSES) || (get_u16(&input[8]) >= 4096U) ||
        (get_u16(&input[18]) > BB_MAX_SDU_BYTES) ||
        ((get_u32(&input[12]) & ~BB_PDCP_SN_MASK) != 0U) ||
        ((input[7] & ~3U) != 0U)) {
        return -4;
    }
    if (require_integrity) {
        for (index = 0U; index < 20U; ++index) {
            authenticated[index] = input[index];
        }
        for (index = 0U; index < payload_length; ++index) {
            authenticated[20U + index] = input[BB_WIRE_HEADER_SIZE + index];
        }
        {
            uint8_t cmac[16];
            uint8_t difference = 0U;
            if (bb_security_nia2(1U, integrity_count, 1U,
                                 (uint8_t)((input[7] >> 1U) & 1U),
                                 authenticated, 20U + payload_length,
                                 cmac) != 0) {
                return -5;
            }
            for (index = 0U; index < 8U; ++index) {
                difference |= (uint8_t)(cmac[index] ^ input[20U + index]);
            }
            if (difference != 0U) {
                return -5;
            }
        }
    }
    pdu->type = input[5];
    pdu->harq_id = input[6];
    pdu->flags = input[7];
    pdu->rlc_sn = get_u16(&input[8]);
    pdu->payload_length = payload_length;
    pdu->pdcp_count = require_integrity ? integrity_count :
                      get_u32(&input[12]);
    pdu->offset = get_u16(&input[16]);
    pdu->total_length = get_u16(&input[18]);
    pdu->payload = &input[BB_WIRE_HEADER_SIZE];
    return 0;
}

int bb_protocol_decode(const uint8_t *input, size_t length,
                       struct bb_wire_pdu *pdu, bool require_integrity,
                       uint32_t *work_budget)
{
    uint32_t wire_count = 0U;
    if ((input != NULL) && (length >= 16U)) {
        wire_count = get_u32(&input[12]);
    }
    return bb_protocol_decode_with_count(input, length, pdu,
                                         require_integrity, wire_count,
                                         work_budget);
}

int bb_protocol_validate_ies(const uint8_t *data, size_t length,
                             uint32_t *work_budget)
{
    size_t positions[4];
    size_t ends[4];
    uint32_t depth = 0U;
    uint32_t ie_count = 0U;
    if ((work_budget == NULL) || ((data == NULL) && (length != 0U)) ||
        (length > 65535U)) {
        return -1;
    }
    if (length == 0U) {
        return 0;
    }
    positions[0] = 0U;
    ends[0] = length;
    for (;;) {
        uint16_t value_length;
        uint8_t type;
        size_t value_start;
        if (positions[depth] == ends[depth]) {
            if (depth == 0U) {
                break;
            }
            depth--;
            continue;
        }
        if ((*work_budget < 4U) ||
            (ends[depth] - positions[depth] < 3U) || (ie_count >= 64U)) {
            return -2;
        }
        *work_budget -= 4U;
        type = data[positions[depth]];
        value_length = get_u16(&data[positions[depth] + 1U]);
        value_start = positions[depth] + 3U;
        if ((size_t)value_length > ends[depth] - value_start) {
            return -3;
        }
        positions[depth] = value_start + value_length;
        ie_count++;
        if ((type & 0x80U) != 0U) {
            if ((depth + 1U >= 4U) || (value_length == 0U)) {
                return -4;
            }
            depth++;
            positions[depth] = value_start;
            ends[depth] = value_start + value_length;
        } else if (*work_budget < value_length) {
            return -5;
        } else {
            *work_budget -= value_length;
        }
    }
    return 0;
}
