#include "bb/bb.h"
#include "bb/ipc.h"
#include "bb/mac.h"
#include "bb/protocol.h"
#include "bb/security.h"
#include "bb/soc.h"

#include <stdint.h>
#include <stdio.h>

static uint8_t bytes[BB_MAX_IQ_SAMPLES * 4U];
static struct bb_cpx16 samples[BB_MAX_IQ_SAMPLES];

static uint32_t next_random(uint32_t *state)
{
    uint32_t value = *state;
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    *state = value;
    return value;
}

int main(void)
{
    struct bb_config config = {
        7680000U, 17U, 1800, 12U, 42U, 0U,
        (uint8_t)BB_MOD_16QAM, 0U, NULL, NULL
    };
    struct bb_iq_block input = {
        0U, 7680000U, 3U * (BB_NFFT + BB_CP_LEN), samples
    };
    uint32_t random_state = 0xC017U;
    uint32_t iteration;

    for (iteration = 0U; iteration < 2000U; ++iteration) {
        struct bb_wire_pdu pdu;
        struct bb_ipc_frame frame;
        struct bb_dma_descriptor descriptor;
        struct bb_mac_sdu mac_sdus[BB_MAC_MAX_SDUS];
        size_t mac_count = 0U;
        uint8_t tag[16];
        uint32_t budget = next_random(&random_state) % 1001U;
        const size_t length = next_random(&random_state) %
                              (BB_MAX_TB_BYTES + 1U);
        size_t index;
        for (index = 0U; index < length; ++index) {
            bytes[index] = (uint8_t)next_random(&random_state);
        }
        (void)bb_protocol_decode(bytes, length, &pdu,
                                 (iteration & 1U) != 0U, &budget);
        budget = next_random(&random_state) % 1001U;
        (void)bb_protocol_validate_ies(bytes, length, &budget);
        (void)bb_ipc_decode(bytes, length, &frame);
        (void)bb_mac_demultiplex(bytes, length, mac_sdus,
                                 BB_MAC_MAX_SDUS, &mac_count);
        (void)bb_security_cmac(1U, bytes, length, tag);
        (void)bb_dma_descriptor_decode(bytes, &descriptor);
        (void)bb_dma_ring_validate(bytes, 16U, 0x40000000U);
    }
    for (iteration = 0U; iteration < 16U; ++iteration) {
        uint8_t output[BB_MAX_TB_BYTES];
        size_t output_length = 0U;
        uint32_t index;
        for (index = 0U; index < input.count; ++index) {
            samples[index].i = (int16_t)next_random(&random_state);
            samples[index].q = (int16_t)next_random(&random_state);
        }
        input.t0 = iteration;
        (void)bb_phy_receive(&config, &input, output, sizeof(output),
                             &output_length);
    }
    (void)printf("deterministic malformed-input sweep passed\n");
    return 0;
}
