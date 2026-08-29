#include "bb/ipc.h"
#include "bb/protocol.h"
#include "bb/security.h"
#include "bb/soc.h"

#include <stdio.h>
#include <string.h>

static int test_cmac(void)
{
    static const uint8_t expected[16] = {
        0xbbU,0x1dU,0x69U,0x29U,0xe9U,0x59U,0x37U,0x28U,
        0x7fU,0xa3U,0x7dU,0x12U,0x9bU,0x75U,0x67U,0x46U
    };
    uint8_t output[16];
    if ((bb_security_cmac(1U, NULL, 0U, output) != 0) ||
        (memcmp(output, expected, sizeof(expected)) != 0)) {
        return -1;
    }
    {
        uint8_t downlink[16];
        uint8_t uplink[16];
        static const uint8_t message[] = { 0x10U, 0x20U, 0x30U };
        if ((bb_security_nia2(1U, 0x40002U, 1U, 0U, message,
                              sizeof(message), downlink) != 0) ||
            (bb_security_nia2(1U, 0x40002U, 1U, 1U, message,
                              sizeof(message), uplink) != 0) ||
            (memcmp(downlink, uplink, sizeof(downlink)) == 0)) {
            return -1;
        }
    }
    return 0;
}

static int test_protocol(void)
{
    static const uint8_t payload[] = { 1U, 2U, 3U, 4U, 5U };
    struct bb_wire_pdu source = {
        (uint8_t)BB_PDU_DATA, 3U, 0U, 4095U, 0x00040002U,
        20U, 4096U, payload, (uint16_t)sizeof(payload)
    };
    struct bb_wire_pdu decoded;
    uint8_t encoded[BB_WIRE_HEADER_SIZE + sizeof(payload)];
    size_t encoded_length = 0U;
    uint32_t budget = 1000U;
    if (bb_protocol_encode(&source, encoded, sizeof(encoded), &encoded_length,
                           true) != 0) {
        return -1;
    }
    if ((encoded[12] != 0x02U) || (encoded[13] != 0x00U) ||
        (encoded[14] != 0x00U) || (encoded[15] != 0x00U) ||
        (bb_protocol_decode_with_count(encoded, encoded_length, &decoded,
                                       true, source.pdcp_count,
                                       &budget) != 0)) {
        return -1;
    }
    if ((decoded.pdcp_count != source.pdcp_count) ||
        (decoded.rlc_sn != source.rlc_sn) ||
        (memcmp(decoded.payload, payload, sizeof(payload)) != 0)) {
        return -1;
    }
    encoded[BB_WIRE_HEADER_SIZE] ^= 1U;
    budget = 1000U;
    return (bb_protocol_decode_with_count(encoded, encoded_length, &decoded,
                                          true, source.pdcp_count,
                                          &budget) != 0) ? 0 : -1;
}

static int test_ie_limits(void)
{
    static const uint8_t nested[] = {
        0x80U, 0x04U, 0x00U, 0x01U, 0x01U, 0x00U, 0xAAU
    };
    static const uint8_t bad_length[] = { 0x01U, 0x10U, 0x00U, 0xAAU };
    uint32_t budget = 1000U;
    if (bb_protocol_validate_ies(nested, sizeof(nested), &budget) != 0) {
        return -1;
    }
    budget = 1000U;
    return (bb_protocol_validate_ies(bad_length, sizeof(bad_length),
                                     &budget) != 0) ? 0 : -1;
}

static int test_ipc(void)
{
    static const uint8_t crc_text[] = "123456789";
    static const uint8_t tlv[] = {
        0x01U,0x00U,0x03U,0x00U,0xAAU,0xBBU,0xCCU,0x00U
    };
    const struct bb_ipc_frame source = {
        1U, 7U, 99U, 3U, 0x10U, tlv, (uint32_t)sizeof(tlv)
    };
    struct bb_ipc_frame decoded;
    uint8_t encoded[64];
    size_t encoded_length = 0U;
    if (bb_crc32c(crc_text, sizeof(crc_text) - 1U) != 0xE3069283U) {
        return -1;
    }
    if ((bb_ipc_encode(&source, encoded, sizeof(encoded), &encoded_length) != 0) ||
        (bb_ipc_decode(encoded, encoded_length, &decoded) != 0)) {
        return -1;
    }
    if ((decoded.sequence != source.sequence) ||
        (decoded.epoch != source.epoch) ||
        (decoded.tlv_length != source.tlv_length)) {
        return -1;
    }
    encoded[12] ^= 1U;
    if (bb_ipc_decode(encoded, encoded_length, &decoded) == 0) {
        return -1;
    }
    {
        static const uint8_t noncanonical_tlv[] = {
            0x01U,0x00U,0x03U,0x00U,0xAAU,0xBBU,0xCCU,0x7FU
        };
        const struct bb_ipc_frame bad_padding = {
            1U, 7U, 1U, 1U, 0U, noncanonical_tlv,
            (uint32_t)sizeof(noncanonical_tlv)
        };
        const struct bb_ipc_frame bad_version = {
            2U, 7U, 1U, 1U, 0U, tlv, (uint32_t)sizeof(tlv)
        };
        if ((bb_ipc_encode(&bad_padding, encoded, sizeof(encoded),
                           &encoded_length) == 0) ||
            (bb_ipc_encode(&bad_version, encoded, sizeof(encoded),
                           &encoded_length) == 0)) {
            return -1;
        }
    }
    return 0;
}

static int test_dma_descriptor(void)
{
    struct bb_dma_descriptor descriptor = {
        0x40001000U, 0x10001000U, 256U,
        BB_DMA_FLAG_OWN | BB_DMA_FLAG_EOP | BB_DMA_FLAG_IRQ,
        7U, 2U, 0U, 0U
    };
    struct bb_dma_descriptor decoded;
    uint8_t wire[BB_DMA_DESCRIPTOR_SIZE];
    if ((bb_dma_descriptor_encode(&descriptor, wire) != 0) ||
        (bb_dma_descriptor_decode(wire, &decoded) != 0) ||
        (decoded.cookie != descriptor.cookie) ||
        (bb_dma_ring_validate(wire, 1U, 0x40000000U) != 0)) {
        return -1;
    }
    {
        uint8_t two_descriptors[2U * BB_DMA_DESCRIPTOR_SIZE] = { 0U };
        (void)memcpy(two_descriptors, wire, BB_DMA_DESCRIPTOR_SIZE);
        if (bb_dma_ring_validate(two_descriptors, 2U, 0x40000000U) == 0) {
            return -1;
        }
    }
    wire[8] = 0U;
    wire[9] = 0U;
    wire[10] = 0U;
    wire[11] = 0U;
    return (bb_dma_descriptor_decode(wire, &decoded) != 0) ? 0 : -1;
}

int main(void)
{
    if (test_cmac() != 0) { (void)fprintf(stderr, "CMAC failed\n"); return 1; }
    if (test_protocol() != 0) { (void)fprintf(stderr, "protocol failed\n"); return 1; }
    if (test_ie_limits() != 0) { (void)fprintf(stderr, "IE parser failed\n"); return 1; }
    if (test_ipc() != 0) { (void)fprintf(stderr, "IPC failed\n"); return 1; }
    if (test_dma_descriptor() != 0) {
        (void)fprintf(stderr, "DMA descriptor failed\n"); return 1;
    }
    (void)printf("all Q2 tests passed\n");
    return 0;
}
