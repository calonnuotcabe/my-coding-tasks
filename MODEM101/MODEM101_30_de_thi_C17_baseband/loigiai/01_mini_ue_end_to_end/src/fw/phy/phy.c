#include "internal.h"

#include <limits.h>

#define BB_ACTIVE_CARRIERS 126U
#define BB_DATA_CARRIERS 124U
#define BB_SYMBOL_SAMPLES (BB_NFFT + BB_CP_LEN)
#define BB_FRAME_OVERHEAD 6U

static uint8_t transport_harq_id(const uint8_t *transport_block,
                                 size_t length)
{
    struct bb_mac_sdu sdus[BB_MAC_MAX_SDUS];
    size_t count = 0U;
    if ((bb_mac_demultiplex(transport_block, length, sdus,
                            BB_MAC_MAX_SDUS, &count) == 0) &&
        (count != 0U) && (sdus[0].length >= 7U) &&
        (sdus[0].data[0] == (uint8_t)'M') &&
        (sdus[0].data[1] == (uint8_t)'1') &&
        (sdus[0].data[2] == (uint8_t)'0') &&
        (sdus[0].data[3] == (uint8_t)'1') &&
        (sdus[0].data[6] < BB_HARQ_PROCESSES)) {
        return sdus[0].data[6];
    }
    return 0U;
}

static uint32_t carrier_bin(int32_t carrier)
{
    return (carrier < 0) ? (uint32_t)((int32_t)BB_NFFT + carrier) :
                           (uint32_t)carrier;
}

static void clear_symbol(struct bb_cpx16 *symbol)
{
    uint32_t index;
    for (index = 0U; index < BB_NFFT; ++index) {
        symbol[index].i = 0;
        symbol[index].q = 0;
    }
}

static void make_pss(uint32_t identity, int16_t sequence[127])
{
    uint8_t state[127];
    uint32_t index;
    state[0] = 0U;
    state[1] = 1U;
    state[2] = 1U;
    state[3] = 0U;
    state[4] = 1U;
    state[5] = 1U;
    state[6] = 1U;
    for (index = 0U; index < 120U; ++index) {
        state[index + 7U] = (uint8_t)((state[index + 4U] + state[index]) & 1U);
    }
    for (index = 0U; index < 127U; ++index) {
        const uint32_t shifted = (index + (43U * (identity % 3U))) % 127U;
        sequence[index] = (state[shifted] == 0U) ? 12000 : -12000;
    }
}

static void map_pss(struct bb_cpx16 *frequency, uint32_t identity)
{
    int16_t sequence[127];
    uint32_t index;
    make_pss(identity, sequence);
    for (index = 0U; index < 127U; ++index) {
        const int32_t carrier = (int32_t)index - 63;
        frequency[carrier_bin(carrier)].i = sequence[index];
    }
}

static int16_t pilot_value(uint32_t active_index)
{
    return ((active_index & 1U) == 0U) ? 12000 : -12000;
}

static void map_pilot(struct bb_cpx16 *frequency)
{
    int32_t carrier;
    uint32_t active_index = 0U;
    for (carrier = -63; carrier <= 63; ++carrier) {
        if (carrier != 0) {
            frequency[carrier_bin(carrier)].i = pilot_value(active_index);
            active_index++;
        }
    }
}

static uint8_t get_bit(const uint8_t *bytes, size_t bit_index)
{
    const uint32_t byte_value = (uint32_t)bytes[bit_index / 8U];
    return (uint8_t)((byte_value >>
                     (7U - (uint32_t)(bit_index & 7U))) & 1U);
}

static void set_bit(uint8_t *bytes, size_t bit_index, uint8_t value)
{
    const uint8_t mask = (uint8_t)(1U << (7U - (uint32_t)(bit_index & 7U)));
    if (value != 0U) {
        bytes[bit_index / 8U] |= mask;
    } else {
        bytes[bit_index / 8U] &= (uint8_t)~mask;
    }
}

static void map_data(struct bb_cpx16 *frequency, const uint8_t *frame,
                     size_t total_bits, size_t *bit_position,
                     uint8_t modulation)
{
    int32_t carrier;
    uint32_t active_index = 0U;
    for (carrier = -63; carrier <= 63; ++carrier) {
        if (carrier != 0) {
            if ((carrier == -63) || (carrier == 63)) {
                frequency[carrier_bin(carrier)].i = pilot_value(active_index);
            } else if (modulation == (uint8_t)BB_MOD_16QAM) {
                uint8_t nibble = 0U;
                uint32_t bit_index;
                for (bit_index = 0U; bit_index < 4U; ++bit_index) {
                    nibble = (uint8_t)(nibble << 1U);
                    if (*bit_position < total_bits) {
                        nibble |= get_bit(frame, *bit_position);
                        (*bit_position)++;
                    }
                }
                bb_qam16_map(nibble, &frequency[carrier_bin(carrier)]);
            } else {
                uint8_t first = 0U;
                uint8_t second = 0U;
                if (*bit_position < total_bits) {
                    first = get_bit(frame, *bit_position);
                    (*bit_position)++;
                }
                if (*bit_position < total_bits) {
                    second = get_bit(frame, *bit_position);
                    (*bit_position)++;
                }
                bb_qpsk_map(first, second, &frequency[carrier_bin(carrier)]);
            }
            active_index++;
        }
    }
}

static void emit_symbol(struct bb_cpx16 *frequency,
                        struct bb_cpx16 *destination)
{
    uint32_t sample_index;
    bb_fft_fixed(frequency, BB_NFFT, true);
    for (sample_index = 0U; sample_index < BB_CP_LEN; ++sample_index) {
        destination[sample_index] =
            frequency[BB_NFFT - BB_CP_LEN + sample_index];
    }
    for (sample_index = 0U; sample_index < BB_NFFT; ++sample_index) {
        destination[BB_CP_LEN + sample_index] = frequency[sample_index];
    }
}

int bb_phy_transmit(const struct bb_config *cfg, const uint8_t *tb,
                    size_t tb_len, struct bb_iq_block *out)
{
    uint8_t frame[BB_MAX_TB_BYTES + BB_FRAME_OVERHEAD];
    struct bb_cpx16 frequency[BB_NFFT];
    const size_t frame_len = tb_len + BB_FRAME_OVERHEAD;
    const size_t total_bits = frame_len * 8U;
    size_t bits_per_symbol;
    size_t data_symbols;
    size_t symbol_count;
    size_t required_samples;
    uint32_t crc;
    size_t index;
    size_t bit_position = 0U;

    if ((cfg == NULL) || (out == NULL) || (out->samples == NULL) ||
        (tb == NULL) || (tb_len == 0U) || (tb_len > BB_MAX_TB_BYTES) ||
        (cfg->sample_rate_hz == 0U) ||
        ((cfg->modulation != (uint8_t)BB_MOD_QPSK) &&
         (cfg->modulation != (uint8_t)BB_MOD_16QAM))) {
        return -1;
    }
    bits_per_symbol = BB_DATA_CARRIERS * (size_t)cfg->modulation;
    data_symbols = (total_bits + bits_per_symbol - 1U) / bits_per_symbol;
    symbol_count = data_symbols + 2U;
    required_samples = symbol_count * BB_SYMBOL_SAMPLES;
    if ((required_samples > BB_MAX_IQ_SAMPLES) ||
        ((size_t)out->count < required_samples)) {
        return -1;
    }
    frame[0] = (uint8_t)(tb_len & 0xFFU);
    frame[1] = (uint8_t)((tb_len >> 8U) & 0xFFU);
    frame[2] = transport_harq_id(tb, tb_len);
    for (index = 0U; index < tb_len; ++index) {
        frame[3U + index] = tb[index];
    }
    crc = bb_crc24a(tb, tb_len);
    frame[3U + tb_len] = (uint8_t)(crc >> 16U);
    frame[4U + tb_len] = (uint8_t)(crc >> 8U);
    frame[5U + tb_len] = (uint8_t)crc;

    clear_symbol(frequency);
    map_pss(frequency, (uint32_t)(cfg->pci % 3U));
    emit_symbol(frequency, &out->samples[0]);
    clear_symbol(frequency);
    map_pilot(frequency);
    emit_symbol(frequency, &out->samples[BB_SYMBOL_SAMPLES]);
    for (index = 0U; index < data_symbols; ++index) {
        clear_symbol(frequency);
        map_data(frequency, frame, total_bits, &bit_position,
                 cfg->modulation);
        emit_symbol(frequency,
                    &out->samples[(index + 2U) * BB_SYMBOL_SAMPLES]);
    }
    /* t0 belongs to the caller's virtual-time domain; modulation must not
     * silently rewrite it. */
    out->sample_rate_hz = cfg->sample_rate_hz;
    out->count = (uint32_t)required_samples;
    bb_trace(cfg, out->t0, "TX_WAVEFORM", (int32_t)out->count);
    return 0;
}

static void reference_pss_symbol(uint32_t identity,
                                 struct bb_cpx16 reference[BB_SYMBOL_SAMPLES])
{
    struct bb_cpx16 frequency[BB_NFFT];
    clear_symbol(frequency);
    map_pss(frequency, identity);
    emit_symbol(frequency, reference);
}

static int find_pss(const struct bb_iq_block *input, uint32_t *timing,
                    uint32_t *identity)
{
    struct bb_cpx16 reference[BB_SYMBOL_SAMPLES];
    int64_t best_metric = -1;
    uint32_t best_offset = 0U;
    uint32_t best_identity = 0U;
    uint32_t pss_identity;
    const uint32_t last_offset = input->count - 2U * BB_SYMBOL_SAMPLES;

    for (pss_identity = 0U; pss_identity < 3U; ++pss_identity) {
        uint32_t offset;
        reference_pss_symbol(pss_identity, reference);
        for (offset = 0U; offset <= last_offset; ++offset) {
            int64_t correlation_i = 0;
            int64_t correlation_q = 0;
            uint32_t sample_index;
            for (sample_index = 0U; sample_index < BB_SYMBOL_SAMPLES;
                 ++sample_index) {
                const struct bb_cpx16 received = input->samples[offset + sample_index];
                const struct bb_cpx16 known = reference[sample_index];
                correlation_i += (int32_t)received.i * known.i +
                                 (int32_t)received.q * known.q;
                correlation_q += (int32_t)received.q * known.i -
                                 (int32_t)received.i * known.q;
            }
            correlation_i /= (int64_t)BB_SYMBOL_SAMPLES;
            correlation_q /= (int64_t)BB_SYMBOL_SAMPLES;
            {
                const int64_t metric =
                    (correlation_i * correlation_i) +
                    (correlation_q * correlation_q);
                if (metric > best_metric) {
                    best_metric = metric;
                    best_offset = offset;
                    best_identity = pss_identity;
                }
            }
        }
    }
    if (best_metric < 1000000LL) {
        return -1;
    }
    *timing = best_offset;
    *identity = best_identity;
    return 0;
}

static int32_t estimate_cfo(const struct bb_iq_block *input, uint32_t timing)
{
    int64_t real = 0;
    int64_t imag = 0;
    uint32_t index;
    int64_t angle_millirad;
    uint64_t absolute_real;
    uint64_t absolute_imag;
    uint64_t ratio_q20;
    int64_t acute_millirad;
    for (index = 0U; index < BB_CP_LEN; ++index) {
        const struct bb_cpx16 first = input->samples[timing + index];
        const struct bb_cpx16 second = input->samples[timing + BB_NFFT + index];
        real += (int32_t)first.i * second.i + (int32_t)first.q * second.q;
        imag += (int32_t)first.i * second.q - (int32_t)first.q * second.i;
    }
    if ((real == 0) && (imag == 0)) {
        return 0;
    }
    absolute_real = (real < 0) ? (uint64_t)(-real) : (uint64_t)real;
    absolute_imag = (imag < 0) ? (uint64_t)(-imag) : (uint64_t)imag;
    if (absolute_imag <= absolute_real) {
        ratio_q20 = (absolute_real == 0U) ? 0U :
                    (absolute_imag << 20U) / absolute_real;
        acute_millirad =
            (int64_t)((ratio_q20 *
             (785U + (273U * ((1U << 20U) - ratio_q20) >> 20U))) >> 20U);
    } else {
        ratio_q20 = (absolute_imag == 0U) ? 0U :
                    (absolute_real << 20U) / absolute_imag;
        acute_millirad = 1571LL -
            (int64_t)((ratio_q20 *
             (785U + (273U * ((1U << 20U) - ratio_q20) >> 20U))) >> 20U);
    }
    if (real >= 0) {
        angle_millirad = (imag >= 0) ? acute_millirad : -acute_millirad;
    } else {
        angle_millirad = (imag >= 0) ?
                           3142LL - acute_millirad :
                          -3142LL + acute_millirad;
    }
    return (int32_t)((angle_millirad * (int64_t)input->sample_rate_hz) /
                     (6283LL * (int64_t)BB_NFFT));
}

static uint32_t agc_gain(const struct bb_iq_block *input, uint32_t timing)
{
    uint32_t peak = 1U;
    uint32_t index;
    for (index = timing; index < input->count; ++index) {
        const uint32_t magnitude_i = (input->samples[index].i < 0) ?
            (uint32_t)(-(int32_t)input->samples[index].i) :
            (uint32_t)input->samples[index].i;
        const uint32_t magnitude_q = (input->samples[index].q < 0) ?
            (uint32_t)(-(int32_t)input->samples[index].q) :
            (uint32_t)input->samples[index].q;
        const uint32_t magnitude = magnitude_i + magnitude_q;
        if (magnitude > peak) {
            peak = magnitude;
        }
    }
    {
        uint32_t gain = (12000U * 4096U) / peak;
        if (gain > 65536U) {
            gain = 65536U;
        }
        return gain;
    }
}

static struct bb_cpx16 correct_sample(struct bb_cpx16 sample,
                                      struct bb_cpx16 oscillator,
                                      uint32_t gain)
{
    const int32_t real = ((int32_t)sample.i * oscillator.i +
                          (int32_t)sample.q * oscillator.q) / 32768;
    const int32_t imag = ((int32_t)sample.q * oscillator.i -
                          (int32_t)sample.i * oscillator.q) / 32768;
    struct bb_cpx16 result;
    result.i = bb_sat16((int32_t)(((int64_t)real * gain) / 4096LL));
    result.q = bb_sat16((int32_t)(((int64_t)imag * gain) / 4096LL));
    return result;
}

static void extract_fft_symbol(const struct bb_iq_block *input,
                               uint32_t symbol_start,
                               struct bb_cpx16 *oscillator,
                               int16_t step_i, int16_t step_q, uint32_t gain,
                               struct bb_cpx16 frequency[BB_NFFT])
{
    uint32_t index;
    for (index = 0U; index < BB_SYMBOL_SAMPLES; ++index) {
        if (index >= BB_CP_LEN) {
            frequency[index - BB_CP_LEN] =
                correct_sample(input->samples[symbol_start + index],
                               *oscillator, gain);
        }
        *oscillator = bb_oscillator_step(*oscillator, step_i, step_q);
    }
    bb_fft_fixed(frequency, BB_NFFT, false);
}

static void estimate_channel(const struct bb_cpx16 *pilot,
                             struct bb_cpx16 channel[BB_ACTIVE_CARRIERS])
{
    int32_t carrier;
    uint32_t active_index = 0U;
    for (carrier = -63; carrier <= 63; ++carrier) {
        if (carrier != 0) {
            const int32_t sign = (pilot_value(active_index) > 0) ? 1 : -1;
            channel[active_index].i =
                bb_sat16((int32_t)pilot[carrier_bin(carrier)].i * sign);
            channel[active_index].q =
                bb_sat16((int32_t)pilot[carrier_bin(carrier)].q * sign);
            active_index++;
        }
    }
}

static struct bb_cpx16 equalize(struct bb_cpx16 received,
                                struct bb_cpx16 channel)
{
    const int64_t real = (int32_t)received.i * channel.i +
                         (int32_t)received.q * channel.q;
    const int64_t imag = (int32_t)received.q * channel.i -
                         (int32_t)received.i * channel.q;
    const int64_t power = (int32_t)channel.i * channel.i +
                          (int32_t)channel.q * channel.q;
    struct bb_cpx16 result;
    if (power < 64LL) {
        result.i = 0;
        result.q = 0;
    } else {
        result.i = bb_sat16((int32_t)((real * 12000LL) / power));
        result.q = bb_sat16((int32_t)((imag * 12000LL) / power));
    }
    return result;
}

static struct bb_cpx16 track_common_phase(struct bb_cpx16 symbol,
                                          struct bb_cpx16 reference)
{
    const int64_t real = (int32_t)symbol.i * reference.i +
                         (int32_t)symbol.q * reference.q;
    const int64_t imag = (int32_t)symbol.q * reference.i -
                         (int32_t)symbol.i * reference.q;
    const int64_t power = (int32_t)reference.i * reference.i +
                          (int32_t)reference.q * reference.q;
    struct bb_cpx16 result;
    if (power < 64LL) {
        return symbol;
    }
    result.i = bb_sat16((int32_t)((real * 12000LL) / power));
    result.q = bb_sat16((int32_t)((imag * 12000LL) / power));
    return result;
}

static void demap_soft_bit(uint8_t *frame, size_t *bit_position,
                           int32_t metric, const int16_t *prior_llr,
                           size_t prior_length, int16_t *output_llr)
{
    int32_t combined = metric;
    if (*bit_position < prior_length) {
        combined += prior_llr[*bit_position];
    }
    /* Return only this reception's reliability.  The HARQ owner keeps the
     * accumulated soft buffer; returning the already-combined value would
     * make a second failed retransmission store 2*old + new. */
    output_llr[*bit_position] = bb_sat16(metric);
    set_bit(frame, *bit_position,
            (bb_sat16(combined) < 0) ? 1U : 0U);
    (*bit_position)++;
}

int bb_phy_receive_soft_meta(const struct bb_config *cfg,
                             const struct bb_iq_block *input,
                             uint8_t *tb, size_t capacity, size_t *tb_len,
                             const int16_t *prior_llr, size_t prior_length,
                             int16_t *output_llr, size_t output_capacity,
                             size_t *output_length, int32_t *harq_id,
                             bool emit_trace)
{
    uint8_t frame[BB_MAX_TB_BYTES + BB_FRAME_OVERHEAD];
    struct bb_cpx16 frequency[BB_NFFT];
    struct bb_cpx16 channel[BB_ACTIVE_CARRIERS];
    struct bb_cpx16 oscillator = { 32767, 0 };
    uint32_t timing;
    uint32_t identity;
    uint32_t gain;
    int32_t cfo;
    int16_t step_i = 32767;
    int16_t step_q = 0;
    size_t bit_position = 0U;
    size_t required_bits = 16U;
    size_t expected_length = 0U;
    bool length_decoded = false;
    uint32_t symbol_index;
    uint32_t available_symbols;

    if (tb_len != NULL) {
        *tb_len = 0U;
    }
    if (output_length != NULL) {
        *output_length = 0U;
    }
    if (harq_id != NULL) {
        *harq_id = -1;
    }
    if ((cfg == NULL) || (input == NULL) || (input->samples == NULL) ||
        (tb == NULL) || (tb_len == NULL) ||
        (output_llr == NULL) || (output_length == NULL) ||
        (harq_id == NULL) ||
        (output_capacity < BB_MAX_FRAME_BITS) ||
        ((prior_llr == NULL) && (prior_length != 0U)) ||
        (prior_length > BB_MAX_FRAME_BITS) ||
        (input->sample_rate_hz == 0U) ||
        (input->sample_rate_hz != cfg->sample_rate_hz) ||
        ((cfg->modulation != (uint8_t)BB_MOD_QPSK) &&
         (cfg->modulation != (uint8_t)BB_MOD_16QAM)) ||
        (input->count < 3U * BB_SYMBOL_SAMPLES) ||
        (input->count > BB_MAX_IQ_SAMPLES)) {
        return -1;
    }
    for (symbol_index = 0U; symbol_index < sizeof(frame); ++symbol_index) {
        frame[symbol_index] = 0U;
    }
    if (find_pss(input, &timing, &identity) != 0) {
        if (emit_trace) bb_trace(cfg, input->t0, "SYNC", -1);
        return -2;
    }
    if (identity != (uint32_t)(cfg->pci % 3U)) {
        if (emit_trace) bb_trace(cfg, input->t0, "SYNC", -1);
        return -2;
    }
    if (emit_trace) bb_trace(cfg, input->t0, "SYNC", (int32_t)identity);
    if (emit_trace) bb_trace(cfg, input->t0, "TIMING_OFFSET", (int32_t)timing);
    cfo = estimate_cfo(input, timing);
    if (emit_trace) bb_trace(cfg, input->t0, "CFO_HZ", cfo);
    gain = agc_gain(input, timing);
    bb_oscillator_init(cfo, input->sample_rate_hz, &step_i, &step_q);

    extract_fft_symbol(input, timing, &oscillator, step_i, step_q, gain,
                       frequency);
    if (emit_trace) bb_trace(cfg, input->t0, "FFT", 0);
    extract_fft_symbol(input, timing + BB_SYMBOL_SAMPLES, &oscillator,
                       step_i, step_q, gain, frequency);
    estimate_channel(frequency, channel);
    if (emit_trace) bb_trace(cfg, input->t0, "CHANNEL_EST", 0);

    available_symbols = (input->count - timing) / BB_SYMBOL_SAMPLES;
    for (symbol_index = 2U; symbol_index < available_symbols; ++symbol_index) {
        int32_t carrier;
        uint32_t active_index = 0U;
        struct bb_cpx16 phase_reference = { 0, 0 };
        extract_fft_symbol(input, timing + symbol_index * BB_SYMBOL_SAMPLES,
                           &oscillator, step_i, step_q, gain, frequency);
        {
            struct bb_cpx16 low =
                equalize(frequency[carrier_bin(-63)], channel[0]);
            struct bb_cpx16 high =
                equalize(frequency[carrier_bin(63)],
                         channel[BB_ACTIVE_CARRIERS - 1U]);
            const int32_t low_sign = (pilot_value(0U) > 0) ? 1 : -1;
            const int32_t high_sign =
                (pilot_value(BB_ACTIVE_CARRIERS - 1U) > 0) ? 1 : -1;
            phase_reference.i = bb_sat16(((int32_t)low.i * low_sign +
                                          (int32_t)high.i * high_sign) / 2);
            phase_reference.q = bb_sat16(((int32_t)low.q * low_sign +
                                          (int32_t)high.q * high_sign) / 2);
        }
        for (carrier = -63; carrier <= 63; ++carrier) {
            if (carrier != 0) {
                const struct bb_cpx16 equalized = track_common_phase(
                    equalize(frequency[carrier_bin(carrier)],
                             channel[active_index]), phase_reference);
                if ((carrier == -63) || (carrier == 63)) {
                    active_index++;
                    continue;
                } else if (cfg->modulation == (uint8_t)BB_MOD_16QAM) {
                    const int32_t absolute_i =
                        (equalized.i < 0) ? -(int32_t)equalized.i :
                                            (int32_t)equalized.i;
                    const int32_t absolute_q =
                        (equalized.q < 0) ? -(int32_t)equalized.q :
                                            (int32_t)equalized.q;
                    if (bit_position + 4U <= sizeof(frame) * 8U) {
                        /* Positive means bit zero and negative means bit one.
                         * The second bit on each 16-QAM axis distinguishes
                         * the outer (0) and inner (1) constellation levels. */
                        const bool erased =
                            (absolute_i + absolute_q) < 512;
                        demap_soft_bit(frame, &bit_position,
                                       erased ? 0 : (int32_t)equalized.i,
                                       prior_llr, prior_length, output_llr);
                        demap_soft_bit(frame, &bit_position,
                                       erased ? 0 : absolute_i - 6324,
                                       prior_llr, prior_length, output_llr);
                        demap_soft_bit(frame, &bit_position,
                                       erased ? 0 : (int32_t)equalized.q,
                                       prior_llr, prior_length, output_llr);
                        demap_soft_bit(frame, &bit_position,
                                       erased ? 0 : absolute_q - 6324,
                                       prior_llr, prior_length, output_llr);
                    }
                } else {
                    if (bit_position < sizeof(frame) * 8U) {
                        demap_soft_bit(frame, &bit_position,
                                       (int32_t)equalized.i, prior_llr,
                                       prior_length, output_llr);
                    }
                    if (bit_position < sizeof(frame) * 8U) {
                        demap_soft_bit(frame, &bit_position,
                                       (int32_t)equalized.q, prior_llr,
                                       prior_length, output_llr);
                    }
                }
                active_index++;
                if ((bit_position >= 16U) && !length_decoded) {
                    expected_length = (size_t)frame[0] |
                                      ((size_t)frame[1] << 8U);
                    if ((expected_length == 0U) ||
                        (expected_length > BB_MAX_TB_BYTES) ||
                        (expected_length > capacity)) {
                        *output_length = bit_position;
                        return -3;
                    }
                    required_bits = (expected_length + BB_FRAME_OVERHEAD) * 8U;
                    length_decoded = true;
                }
                if ((bit_position >= 24U) && (*harq_id < 0)) {
                    *harq_id = (frame[2] < BB_HARQ_PROCESSES) ?
                               (int32_t)frame[2] : -1;
                }
                if (length_decoded &&
                    (bit_position >= required_bits)) {
                    break;
                }
            }
        }
        if (length_decoded && (bit_position >= required_bits)) {
            break;
        }
    }
    *output_length = bit_position;
    if (emit_trace) bb_trace(cfg, input->t0, "DEMAP", (int32_t)bit_position);
    if (!length_decoded || (bit_position < required_bits)) {
        return -4;
    }
    {
        const uint32_t received_crc =
            ((uint32_t)frame[3U + expected_length] << 16U) |
            ((uint32_t)frame[4U + expected_length] << 8U) |
            (uint32_t)frame[5U + expected_length];
        const uint32_t computed_crc =
            bb_crc24a(&frame[3], expected_length);
        if (emit_trace) {
            bb_trace(cfg, input->t0, "DECODE", (int32_t)expected_length);
        }
        if (received_crc != computed_crc) {
            if (emit_trace) bb_trace(cfg, input->t0, "TB_CRC", 0);
            return -5;
        }
    }
    for (symbol_index = 0U; symbol_index < expected_length; ++symbol_index) {
        tb[symbol_index] = frame[3U + symbol_index];
    }
    *tb_len = expected_length;
    if (emit_trace) bb_trace(cfg, input->t0, "TB_CRC", 1);
    return 0;
}

int bb_phy_receive_soft(const struct bb_config *cfg,
                        const struct bb_iq_block *input,
                        uint8_t *tb, size_t capacity, size_t *tb_len,
                        const int16_t *prior_llr, size_t prior_length,
                        int16_t *output_llr, size_t output_capacity,
                        size_t *output_length)
{
    int32_t harq_id = -1;
    return bb_phy_receive_soft_meta(cfg, input, tb, capacity, tb_len,
                                    prior_llr, prior_length, output_llr,
                                    output_capacity, output_length, &harq_id,
                                    true);
}

int bb_phy_receive(const struct bb_config *cfg, const struct bb_iq_block *input,
                   uint8_t *tb, size_t capacity, size_t *tb_len)
{
    int16_t llr[BB_MAX_FRAME_BITS];
    size_t llr_length = 0U;
    int32_t harq_id = -1;
    return bb_phy_receive_soft_meta(cfg, input, tb, capacity, tb_len,
                                    NULL, 0U, llr, BB_MAX_FRAME_BITS,
                                    &llr_length, &harq_id, true);
}
