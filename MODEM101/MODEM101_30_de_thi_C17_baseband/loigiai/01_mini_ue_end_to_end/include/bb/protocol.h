#ifndef BB_PROTOCOL_H
#define BB_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BB_WIRE_HEADER_SIZE 28U
#define BB_WIRE_MAX_PAYLOAD (512U - 12U - BB_WIRE_HEADER_SIZE)
#define BB_PDCP_SN_BITS 18U
#define BB_PDCP_SN_MASK 0x3FFFFU

enum bb_pdu_type {
    BB_PDU_CELL_BEACON = 1,
    BB_PDU_RRC_SETUP = 2,
    BB_PDU_SECURITY_MODE = 3,
    BB_PDU_REG_ACCEPT = 4,
    BB_PDU_SESSION_ACCEPT = 5,
    BB_PDU_DATA_GRANT = 6,
    BB_PDU_DATA = 7,
    BB_PDU_LINK_LOSS = 8,
    BB_PDU_RRC_COMPLETE = 0x82,
    BB_PDU_SECURITY_COMPLETE = 0x83,
    BB_PDU_REG_REQUEST = 0x84,
    BB_PDU_NACK = 0x86,
    BB_PDU_DATA_ACK = 0x87
};

struct bb_wire_pdu {
    uint8_t type;
    uint8_t harq_id;
    uint8_t flags;
    uint16_t rlc_sn;
    uint32_t pdcp_count;
    uint16_t offset;
    uint16_t total_length;
    const uint8_t *payload;
    uint16_t payload_length;
};

int bb_protocol_encode(const struct bb_wire_pdu *pdu, uint8_t *output,
                       size_t capacity, size_t *length, bool integrity);
int bb_protocol_decode(const uint8_t *input, size_t length,
                       struct bb_wire_pdu *pdu, bool require_integrity,
                       uint32_t *work_budget);
int bb_protocol_decode_with_count(const uint8_t *input, size_t length,
                                  struct bb_wire_pdu *pdu,
                                  bool require_integrity,
                                  uint32_t integrity_count,
                                  uint32_t *work_budget);
int bb_protocol_validate_ies(const uint8_t *data, size_t length,
                             uint32_t *work_budget);

#endif
