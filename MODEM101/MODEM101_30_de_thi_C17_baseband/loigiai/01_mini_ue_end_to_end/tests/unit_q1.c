#include "bb/bb.h"

#include <stdio.h>
#include <string.h>

static struct bb_cpx16 tx_samples[BB_MAX_IQ_SAMPLES];
static struct bb_cpx16 rx_samples[BB_MAX_IQ_SAMPLES];
static struct bb_cpx16 repeat_samples[BB_MAX_IQ_SAMPLES];

static int test_crc(void)
{
    static const uint8_t text[] = "123456789";
    return (bb_crc24a(text, sizeof(text) - 1U) == 0xCDE703U) ? 0 : -1;
}

static int test_constellations(void)
{
    static const struct bb_cpx16 qpsk_expected[4] = {
        { 11585, 11585 }, { 11585, -11585 },
        { -11585, 11585 }, { -11585, -11585 }
    };
    static const int16_t qam_axis_expected[4] = {
        9486, 3162, -9486, -3162
    };
    uint32_t bits;
    for (bits = 0U; bits < 4U; ++bits) {
        struct bb_cpx16 symbol;
        uint8_t first;
        uint8_t second;
        bb_qpsk_map((uint8_t)(bits >> 1U), (uint8_t)bits, &symbol);
        bb_qpsk_demap(symbol, &first, &second);
        if ((symbol.i != qpsk_expected[bits].i) ||
            (symbol.q != qpsk_expected[bits].q) ||
            (first != (uint8_t)(bits >> 1U)) ||
            (second != (uint8_t)(bits & 1U))) {
            return -1;
        }
    }
    for (bits = 0U; bits < 16U; ++bits) {
        struct bb_cpx16 symbol;
        bb_qam16_map((uint8_t)bits, &symbol);
        if ((symbol.i != qam_axis_expected[(bits >> 2U) & 3U]) ||
            (symbol.q != qam_axis_expected[bits & 3U]) ||
            (bb_qam16_demap(symbol) != (uint8_t)bits)) {
            return -1;
        }
    }
    return 0;
}

static int test_fft(void)
{
    static const struct bb_cpx16 expected[8] = {
        { 4000, 0 }, { -1828, -12071 }, { 0, 0 }, { 3828, -2071 },
        { 0, 0 }, { 3828, 2071 }, { 0, 0 }, { -1828, 12071 }
    };
    struct bb_cpx16 vector[8] = {
        { 1000, 0 }, { 2000, 0 }, { 3000, 0 }, { 4000, 0 },
        { 0, 0 }, { -1000, 0 }, { -2000, 0 }, { -3000, 0 }
    };
    struct bb_cpx16 values[BB_NFFT];
    uint32_t index;
    for (index = 0U; index < BB_NFFT; ++index) {
        values[index].i = 0;
        values[index].q = 0;
    }
    values[0].i = 12000;
    bb_fft_fixed(values, BB_NFFT, true);
    bb_fft_fixed(values, BB_NFFT, false);
    if ((values[0].i < 11500) || (values[0].i > 12500)) {
        return -1;
    }
    bb_fft_fixed(vector, 8U, false);
    for (index = 0U; index < 8U; ++index) {
        const int32_t error_i = (int32_t)vector[index].i - expected[index].i;
        const int32_t error_q = (int32_t)vector[index].q - expected[index].q;
        if ((error_i < -8) || (error_i > 8) ||
            (error_q < -8) || (error_q > 8)) {
            return -1;
        }
    }
    return 0;
}

static int test_pss_fixture(void)
{
    /* Literal N_ID2=1 fixture generated from the public NR PSS recurrence,
     * kept independent of the transmitter's runtime sequence generator. */
    static const int8_t expected[127] = {
         1, 1, 1,-1,-1, 1, 1,-1, 1, 1, 1,-1, 1, 1, 1, 1,
         1, 1,-1, 1, 1,-1, 1, 1,-1,-1, 1,-1, 1, 1,-1,-1,
        -1,-1, 1,-1,-1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,
        -1, 1, 1, 1,-1,-1,-1, 1,-1,-1, 1, 1, 1,-1, 1,-1,
         1, 1,-1, 1,-1,-1,-1,-1,-1, 1,-1, 1,-1, 1,-1, 1,
         1, 1, 1,-1, 1,-1,-1, 1,-1,-1,-1,-1, 1, 1,-1,-1,
        -1, 1, 1,-1, 1,-1, 1,-1,-1, 1, 1,-1,-1, 1, 1, 1,
         1, 1,-1,-1, 1,-1,-1, 1,-1, 1,-1,-1,-1, 1,-1
    };
    const struct bb_config config = {
        7680000U, 1U, 0, 0U, 1U, 0U,
        (uint8_t)BB_MOD_QPSK, 0U, NULL, NULL
    };
    const uint8_t payload = 0xA5U;
    struct bb_iq_block waveform = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, tx_samples
    };
    struct bb_cpx16 frequency[BB_NFFT];
    uint32_t index;
    if (bb_phy_transmit(&config, &payload, 1U, &waveform) != 0) {
        return -1;
    }
    for (index = 0U; index < BB_NFFT; ++index) {
        frequency[index] = waveform.samples[BB_CP_LEN + index];
    }
    bb_fft_fixed(frequency, BB_NFFT, false);
    for (index = 0U; index < 127U; ++index) {
        const int32_t carrier = (int32_t)index - 63;
        const uint32_t bin = (carrier < 0) ?
            (uint32_t)((int32_t)BB_NFFT + carrier) : (uint32_t)carrier;
        const int32_t target = (int32_t)expected[index] * 12000;
        const int32_t error_i = (int32_t)frequency[bin].i - target;
        const int32_t imag = frequency[bin].q;
        if ((error_i < -300) || (error_i > 300) ||
            (imag < -150) || (imag > 150)) {
            return -1;
        }
    }
    return 0;
}

static int test_waveform(void)
{
    static const uint8_t payload[] = {
        0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0xAAU, 0xFFU,
        0x5AU, 0xC3U, 0x7EU, 0x81U, 0x10U, 0x20U, 0x30U, 0x40U
    };
    struct bb_config config = {
        .sample_rate_hz = 7680000U,
        .seed = 77U,
        .cfo_hz = 1800,
        .noise_amplitude = 8U,
        .pci = 1U,
        .timing_offset_samples = 0U,
        .modulation = (uint8_t)BB_MOD_QPSK,
        .reserved = 0U,
        .trace = NULL,
        .trace_ctx = NULL
    };
    struct bb_iq_block transmit = { 77U, 7680000U, BB_MAX_IQ_SAMPLES,
                                    tx_samples };
    struct bb_iq_block receive = { 0U, 7680000U, BB_MAX_IQ_SAMPLES,
                                   rx_samples };
    uint8_t decoded[BB_MAX_TB_BYTES];
    size_t decoded_length = 0U;
    if (bb_phy_transmit(&config, payload, sizeof(payload), &transmit) != 0) {
        return -1;
    }
    if (transmit.t0 != 77U) {
        return -1;
    }
    if (bb_channel_apply(&config, &transmit, &receive) != 0) {
        return -1;
    }
    if (bb_phy_receive(&config, &receive, decoded, sizeof(decoded),
                       &decoded_length) != 0) {
        return -1;
    }
    if ((decoded_length != sizeof(payload)) ||
        (memcmp(decoded, payload, sizeof(payload)) != 0)) {
        return -1;
    }
    config.pci = 2U;
    if (bb_phy_receive(&config, &receive, decoded, sizeof(decoded),
                       &decoded_length) == 0) {
        return -1;
    }
    return 0;
}

static int test_phy_profiles(void)
{
    uint8_t payload[BB_MAX_TB_BYTES];
    uint8_t decoded[BB_MAX_TB_BYTES];
    struct bb_config config = {
        7680000U, 991U, -2200, 10U, 2U, 63U,
        (uint8_t)BB_MOD_16QAM, 0U, NULL, NULL
    };
    struct bb_iq_block transmit = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, tx_samples
    };
    struct bb_iq_block receive = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, rx_samples
    };
    struct bb_iq_block repeated = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, repeat_samples
    };
    size_t decoded_length = 0U;
    uint32_t index;
    for (index = 0U; index < sizeof(payload); ++index) {
        payload[index] = (uint8_t)((index * 17U) & 0xFFU);
    }
    if (bb_phy_transmit(&config, payload, sizeof(payload), &transmit) != 0) {
        (void)fprintf(stderr, "max TB transmit failed\n");
        return -1;
    }
    if ((bb_channel_apply(&config, &transmit, &receive) != 0) ||
        (bb_channel_apply(&config, &transmit, &repeated) != 0)) {
        (void)fprintf(stderr, "profile channel failed\n");
        return -1;
    }
    if ((receive.count != repeated.count) ||
        (memcmp(receive.samples, repeated.samples,
                (size_t)receive.count * sizeof(receive.samples[0])) != 0)) {
        (void)fprintf(stderr, "deterministic channel failed\n");
        return -1;
    }
    {
        const int receive_result =
            bb_phy_receive(&config, &receive, decoded, sizeof(decoded),
                           &decoded_length);
        if (receive_result != 0) {
            (void)fprintf(stderr, "max TB receive failed: %d\n", receive_result);
            return -1;
        }
    }
    if ((decoded_length != sizeof(payload)) ||
        (memcmp(decoded, payload, sizeof(payload)) != 0)) {
        (void)fprintf(stderr, "max TB content failed\n");
        return -1;
    }
    for (index = config.timing_offset_samples + 2U * (BB_NFFT + BB_CP_LEN);
         index < receive.count; ++index) {
        receive.samples[index].i = 0;
        receive.samples[index].q = 0;
    }
    decoded_length = 0U;
    if (bb_phy_receive(&config, &receive, decoded, sizeof(decoded),
                       &decoded_length) == 0) {
        return -1;
    }
    return 0;
}

static int test_phy_validation(void)
{
    static const uint8_t payload[] = { 0xA5U };
    struct bb_config config = {
        7680000U, 1U, 0, UINT16_MAX, 42U, 0U,
        (uint8_t)BB_MOD_QPSK, 0U, NULL, NULL
    };
    struct bb_iq_block transmit = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, tx_samples
    };
    struct bb_iq_block receive = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, rx_samples
    };
    uint8_t decoded[BB_MAX_TB_BYTES];
    size_t decoded_length = 0U;
    if ((bb_phy_transmit(&config, payload, sizeof(payload), &transmit) != 0) ||
        (bb_channel_apply(&config, &transmit, &receive) != 0)) {
        return -1;
    }
    receive.sample_rate_hz++;
    if (bb_phy_receive(&config, &receive, decoded, sizeof(decoded),
                       &decoded_length) == 0) {
        return -1;
    }
    receive.sample_rate_hz--;
    config.modulation = 3U;
    return (bb_phy_receive(&config, &receive, decoded, sizeof(decoded),
                           &decoded_length) != 0) ? 0 : -1;
}

static int test_empty_transport_block_rejected(void)
{
    const struct bb_config config = {
        7680000U, 1U, 0, 0U, 42U, 0U,
        (uint8_t)BB_MOD_QPSK, 0U, NULL, NULL
    };
    struct bb_iq_block waveform = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, tx_samples
    };
    return (bb_phy_transmit(&config, NULL, 0U, &waveform) != 0) ? 0 : -1;
}

int main(void)
{
    if (test_crc() != 0) {
        (void)fprintf(stderr, "CRC-24A test failed\n");
        return 1;
    }
    if (test_constellations() != 0) {
        (void)fprintf(stderr, "constellation test failed\n");
        return 1;
    }
    if (test_fft() != 0) {
        (void)fprintf(stderr, "fixed-point FFT test failed\n");
        return 1;
    }
    if (test_pss_fixture() != 0) {
        (void)fprintf(stderr, "independent PSS fixture failed\n");
        return 1;
    }
    if (test_waveform() != 0) {
        (void)fprintf(stderr, "waveform loopback test failed\n");
        return 1;
    }
    if (test_phy_profiles() != 0) {
        (void)fprintf(stderr, "QPSK/16-QAM profile test failed\n");
        return 1;
    }
    if (test_phy_validation() != 0) {
        (void)fprintf(stderr, "PHY defensive validation failed\n");
        return 1;
    }
    if (test_empty_transport_block_rejected() != 0) {
        (void)fprintf(stderr, "empty TB validation failed\n");
        return 1;
    }
    (void)printf("all Q1 tests passed\n");
    return 0;
}
