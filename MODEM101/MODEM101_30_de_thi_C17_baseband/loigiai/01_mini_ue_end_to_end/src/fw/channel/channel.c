#include "internal.h"

static uint32_t prng_next(uint32_t *state)
{
    uint32_t value = *state;
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    *state = value;
    return value;
}

static int32_t gaussian_like(uint32_t *state, uint16_t amplitude)
{
    int32_t sum = 0;
    uint32_t draw;
    for (draw = 0U; draw < 6U; ++draw) {
        sum += (int32_t)(prng_next(state) & 0xFFFFU) - 32768;
    }
    return (int32_t)(((int64_t)sum * (int64_t)amplitude) / 65536LL);
}

int bb_channel_apply(const struct bb_config *cfg,
                     const struct bb_iq_block *in, struct bb_iq_block *out)
{
    struct bb_cpx16 oscillator = { 32767, 0 };
    int16_t step_i = 32767;
    int16_t step_q = 0;
    uint32_t random_state;
    uint32_t sample_index;
    const uint32_t delay = (cfg != NULL) ? cfg->timing_offset_samples : 0U;

    if ((cfg == NULL) || (in == NULL) || (out == NULL) ||
        (in->samples == NULL) || (out->samples == NULL) ||
        (in->count > BB_MAX_IQ_SAMPLES) ||
        (delay > BB_MAX_IQ_SAMPLES - in->count) ||
        (out->count < in->count + delay) ||
        (in->sample_rate_hz == 0U) ||
        (in->sample_rate_hz != cfg->sample_rate_hz)) {
        return -1;
    }
    for (sample_index = 0U; sample_index < delay; ++sample_index) {
        out->samples[sample_index].i = 0;
        out->samples[sample_index].q = 0;
    }
    random_state = (cfg->seed == 0U) ? 1U : cfg->seed;
    bb_oscillator_init(cfg->cfo_hz, in->sample_rate_hz, &step_i, &step_q);
    for (sample_index = 0U; sample_index < in->count; ++sample_index) {
        const int32_t rotated_i =
            ((int32_t)in->samples[sample_index].i * oscillator.i -
             (int32_t)in->samples[sample_index].q * oscillator.q) / 32768;
        const int32_t rotated_q =
            ((int32_t)in->samples[sample_index].i * oscillator.q +
             (int32_t)in->samples[sample_index].q * oscillator.i) / 32768;
        out->samples[delay + sample_index].i =
            bb_sat16(rotated_i + gaussian_like(&random_state,
                                               cfg->noise_amplitude));
        out->samples[delay + sample_index].q =
            bb_sat16(rotated_q + gaussian_like(&random_state,
                                               cfg->noise_amplitude));
        oscillator = bb_oscillator_step(oscillator, step_i, step_q);
    }
    out->t0 = in->t0;
    out->sample_rate_hz = in->sample_rate_hz;
    out->count = in->count + delay;
    return 0;
}
