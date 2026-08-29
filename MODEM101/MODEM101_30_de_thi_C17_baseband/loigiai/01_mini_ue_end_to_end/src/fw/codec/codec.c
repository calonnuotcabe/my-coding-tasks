#include "internal.h"

#include <limits.h>

int16_t bb_sat16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

uint32_t bb_crc24a(const uint8_t *data, size_t len)
{
    uint32_t crc = 0U;
    size_t byte_index;
    if ((data == NULL) && (len != 0U)) {
        return 0U;
    }
    for (byte_index = 0U; byte_index < len; ++byte_index) {
        uint32_t bit_index;
        crc ^= (uint32_t)data[byte_index] << 16U;
        for (bit_index = 0U; bit_index < 8U; ++bit_index) {
            crc <<= 1U;
            if ((crc & 0x1000000U) != 0U) {
                crc ^= 0x1864CFBU;
            }
        }
    }
    return crc & 0xFFFFFFU;
}

void bb_qpsk_map(uint8_t first, uint8_t second, struct bb_cpx16 *symbol)
{
    if (symbol != NULL) {
        symbol->i = ((first & 1U) == 0U) ? 11585 : -11585;
        symbol->q = ((second & 1U) == 0U) ? 11585 : -11585;
    }
}

void bb_qpsk_demap(struct bb_cpx16 symbol, uint8_t *first, uint8_t *second)
{
    if ((first != NULL) && (second != NULL)) {
        *first = (symbol.i < 0) ? 1U : 0U;
        *second = (symbol.q < 0) ? 1U : 0U;
    }
}

static int16_t qam_axis(uint8_t pair)
{
    static const int16_t levels[4] = { 9486, 3162, -9486, -3162 };
    return levels[pair & 3U];
}

void bb_qam16_map(uint8_t bits, struct bb_cpx16 *symbol)
{
    if (symbol != NULL) {
        symbol->i = qam_axis((uint8_t)((bits >> 2U) & 3U));
        symbol->q = qam_axis((uint8_t)(bits & 3U));
    }
}

static uint8_t qam_axis_demap(int16_t value)
{
    if (value >= 6324) {
        return 0U;
    }
    if (value >= 0) {
        return 1U;
    }
    if (value <= -6324) {
        return 2U;
    }
    return 3U;
}

uint8_t bb_qam16_demap(struct bb_cpx16 symbol)
{
    return (uint8_t)((qam_axis_demap(symbol.i) << 2U) |
                     qam_axis_demap(symbol.q));
}

static uint32_t reverse_bits(uint32_t value, uint32_t width)
{
    uint32_t reversed = 0U;
    uint32_t bit_index;
    for (bit_index = 0U; bit_index < width; ++bit_index) {
        reversed = (reversed << 1U) | (value & 1U);
        value >>= 1U;
    }
    return reversed;
}

static void fft_twiddle(uint32_t length, bool inverse,
                        int16_t *step_i, int16_t *step_q)
{
    static const int16_t real_table[8] = {
        -32767, 0, 23170, 30273, 32137, 32609, 32728, 32757
    };
    static const int16_t imag_table[8] = {
        0, 32767, 23170, 12539, 6393, 3212, 1608, 804
    };
    uint32_t index = 0U;
    uint32_t current = 2U;
    while ((current < length) && (index < 7U)) {
        current <<= 1U;
        index++;
    }
    *step_i = real_table[index];
    *step_q = inverse ? imag_table[index] : (int16_t)-imag_table[index];
}

static struct bb_cpx16 complex_q15(struct bb_cpx16 left,
                                   struct bb_cpx16 right)
{
    const int32_t real = ((int32_t)left.i * (int32_t)right.i -
                          (int32_t)left.q * (int32_t)right.q) / 32768;
    const int32_t imag = ((int32_t)left.i * (int32_t)right.q +
                          (int32_t)left.q * (int32_t)right.i) / 32768;
    struct bb_cpx16 result;
    result.i = bb_sat16(real);
    result.q = bb_sat16(imag);
    return result;
}

void bb_fft_fixed(struct bb_cpx16 *samples, uint32_t count, bool inverse)
{
    uint32_t width = 0U;
    uint32_t length;
    uint32_t index;

    if ((samples == NULL) || (count < 2U) || (count > BB_NFFT) ||
        ((count & (count - 1U)) != 0U)) {
        return;
    }
    while ((1U << width) < count) {
        width++;
    }
    for (index = 0U; index < count; ++index) {
        const uint32_t reversed = reverse_bits(index, width);
        if (reversed > index) {
            const struct bb_cpx16 temporary = samples[index];
            samples[index] = samples[reversed];
            samples[reversed] = temporary;
        }
    }
    for (length = 2U; length <= count; length <<= 1U) {
        int16_t step_i;
        int16_t step_q;
        uint32_t group;
        fft_twiddle(length, inverse, &step_i, &step_q);
        for (group = 0U; group < count; group += length) {
            struct bb_cpx16 twiddle = { 32767, 0 };
            uint32_t offset;
            for (offset = 0U; offset < (length / 2U); ++offset) {
                const uint32_t even_index = group + offset;
                const uint32_t odd_index = even_index + length / 2U;
                const struct bb_cpx16 odd =
                    complex_q15(samples[odd_index], twiddle);
                const int32_t sum_i = (int32_t)samples[even_index].i + odd.i;
                const int32_t sum_q = (int32_t)samples[even_index].q + odd.q;
                const int32_t diff_i = (int32_t)samples[even_index].i - odd.i;
                const int32_t diff_q = (int32_t)samples[even_index].q - odd.q;
                if (inverse) {
                    samples[even_index].i = bb_sat16(sum_i / 2);
                    samples[even_index].q = bb_sat16(sum_q / 2);
                    samples[odd_index].i = bb_sat16(diff_i / 2);
                    samples[odd_index].q = bb_sat16(diff_q / 2);
                } else {
                    samples[even_index].i = bb_sat16(sum_i);
                    samples[even_index].q = bb_sat16(sum_q);
                    samples[odd_index].i = bb_sat16(diff_i);
                    samples[odd_index].q = bb_sat16(diff_q);
                }
                twiddle = complex_q15(twiddle,
                                      (struct bb_cpx16){ step_i, step_q });
            }
        }
        if (length == count) {
            break;
        }
    }
}

void bb_oscillator_init(int32_t frequency_hz, uint32_t sample_rate_hz,
                        int16_t *step_i, int16_t *step_q)
{
    int64_t sine;
    int64_t cosine;
    if ((sample_rate_hz == 0U) || (step_i == NULL) || (step_q == NULL)) {
        return;
    }
    /* Q15 small-angle oscillator; accurate for |f| <= Fs/32. */
    sine = ((int64_t)frequency_hz * 205887LL) / (int64_t)sample_rate_hz;
    if (sine > 32767LL) {
        sine = 32767LL;
    } else if (sine < -32767LL) {
        sine = -32767LL;
    }
    cosine = 32767LL - ((sine * sine) / 65534LL);
    *step_i = (int16_t)cosine;
    *step_q = (int16_t)sine;
}

struct bb_cpx16 bb_oscillator_step(struct bb_cpx16 current,
                                   int16_t step_i, int16_t step_q)
{
    return complex_q15(current, (struct bb_cpx16){ step_i, step_q });
}

