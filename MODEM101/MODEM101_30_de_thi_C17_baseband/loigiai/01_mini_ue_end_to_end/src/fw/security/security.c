#include "internal.h"

extern const uint8_t bb_accelerator_test_key[16];

static const uint8_t aes_sbox[256] = {
    0x63U,0x7cU,0x77U,0x7bU,0xf2U,0x6bU,0x6fU,0xc5U,0x30U,0x01U,0x67U,0x2bU,0xfeU,0xd7U,0xabU,0x76U,
    0xcaU,0x82U,0xc9U,0x7dU,0xfaU,0x59U,0x47U,0xf0U,0xadU,0xd4U,0xa2U,0xafU,0x9cU,0xa4U,0x72U,0xc0U,
    0xb7U,0xfdU,0x93U,0x26U,0x36U,0x3fU,0xf7U,0xccU,0x34U,0xa5U,0xe5U,0xf1U,0x71U,0xd8U,0x31U,0x15U,
    0x04U,0xc7U,0x23U,0xc3U,0x18U,0x96U,0x05U,0x9aU,0x07U,0x12U,0x80U,0xe2U,0xebU,0x27U,0xb2U,0x75U,
    0x09U,0x83U,0x2cU,0x1aU,0x1bU,0x6eU,0x5aU,0xa0U,0x52U,0x3bU,0xd6U,0xb3U,0x29U,0xe3U,0x2fU,0x84U,
    0x53U,0xd1U,0x00U,0xedU,0x20U,0xfcU,0xb1U,0x5bU,0x6aU,0xcbU,0xbeU,0x39U,0x4aU,0x4cU,0x58U,0xcfU,
    0xd0U,0xefU,0xaaU,0xfbU,0x43U,0x4dU,0x33U,0x85U,0x45U,0xf9U,0x02U,0x7fU,0x50U,0x3cU,0x9fU,0xa8U,
    0x51U,0xa3U,0x40U,0x8fU,0x92U,0x9dU,0x38U,0xf5U,0xbcU,0xb6U,0xdaU,0x21U,0x10U,0xffU,0xf3U,0xd2U,
    0xcdU,0x0cU,0x13U,0xecU,0x5fU,0x97U,0x44U,0x17U,0xc4U,0xa7U,0x7eU,0x3dU,0x64U,0x5dU,0x19U,0x73U,
    0x60U,0x81U,0x4fU,0xdcU,0x22U,0x2aU,0x90U,0x88U,0x46U,0xeeU,0xb8U,0x14U,0xdeU,0x5eU,0x0bU,0xdbU,
    0xe0U,0x32U,0x3aU,0x0aU,0x49U,0x06U,0x24U,0x5cU,0xc2U,0xd3U,0xacU,0x62U,0x91U,0x95U,0xe4U,0x79U,
    0xe7U,0xc8U,0x37U,0x6dU,0x8dU,0xd5U,0x4eU,0xa9U,0x6cU,0x56U,0xf4U,0xeaU,0x65U,0x7aU,0xaeU,0x08U,
    0xbaU,0x78U,0x25U,0x2eU,0x1cU,0xa6U,0xb4U,0xc6U,0xe8U,0xddU,0x74U,0x1fU,0x4bU,0xbdU,0x8bU,0x8aU,
    0x70U,0x3eU,0xb5U,0x66U,0x48U,0x03U,0xf6U,0x0eU,0x61U,0x35U,0x57U,0xb9U,0x86U,0xc1U,0x1dU,0x9eU,
    0xe1U,0xf8U,0x98U,0x11U,0x69U,0xd9U,0x8eU,0x94U,0x9bU,0x1eU,0x87U,0xe9U,0xceU,0x55U,0x28U,0xdfU,
    0x8cU,0xa1U,0x89U,0x0dU,0xbfU,0xe6U,0x42U,0x68U,0x41U,0x99U,0x2dU,0x0fU,0xb0U,0x54U,0xbbU,0x16U
};

static uint8_t xtime(uint8_t value)
{
    return (uint8_t)((uint8_t)(value << 1U) ^
                     (((value & 0x80U) != 0U) ? 0x1bU : 0U));
}

static void aes_key_expand(const uint8_t key[16], uint8_t round_key[176])
{
    static const uint8_t rcon[10] = {
        0x01U,0x02U,0x04U,0x08U,0x10U,0x20U,0x40U,0x80U,0x1bU,0x36U
    };
    uint32_t index;
    uint32_t generated = 16U;
    uint32_t round = 0U;
    uint8_t temporary[4];
    for (index = 0U; index < 16U; ++index) {
        round_key[index] = key[index];
    }
    while (generated < 176U) {
        for (index = 0U; index < 4U; ++index) {
            temporary[index] = round_key[generated - 4U + index];
        }
        if ((generated & 15U) == 0U) {
            const uint8_t first = temporary[0];
            temporary[0] = (uint8_t)(aes_sbox[temporary[1]] ^ rcon[round]);
            temporary[1] = aes_sbox[temporary[2]];
            temporary[2] = aes_sbox[temporary[3]];
            temporary[3] = aes_sbox[first];
            round++;
        }
        for (index = 0U; index < 4U; ++index) {
            round_key[generated] =
                (uint8_t)(round_key[generated - 16U] ^ temporary[index]);
            generated++;
        }
    }
}

static void add_round_key(uint8_t state[16], const uint8_t *round_key)
{
    uint32_t index;
    for (index = 0U; index < 16U; ++index) {
        state[index] ^= round_key[index];
    }
}

static void sub_shift(uint8_t state[16])
{
    uint8_t temporary[16];
    uint32_t index;
    static const uint8_t source[16] = {
        0U,5U,10U,15U,4U,9U,14U,3U,8U,13U,2U,7U,12U,1U,6U,11U
    };
    for (index = 0U; index < 16U; ++index) {
        temporary[index] = aes_sbox[state[source[index]]];
    }
    for (index = 0U; index < 16U; ++index) {
        state[index] = temporary[index];
    }
}

static void mix_columns(uint8_t state[16])
{
    uint32_t column;
    for (column = 0U; column < 4U; ++column) {
        const uint32_t offset = column * 4U;
        const uint8_t a = state[offset];
        const uint8_t b = state[offset + 1U];
        const uint8_t c = state[offset + 2U];
        const uint8_t d = state[offset + 3U];
        const uint8_t common = (uint8_t)(a ^ b ^ c ^ d);
        state[offset] ^= (uint8_t)(common ^ xtime((uint8_t)(a ^ b)));
        state[offset + 1U] ^= (uint8_t)(common ^ xtime((uint8_t)(b ^ c)));
        state[offset + 2U] ^= (uint8_t)(common ^ xtime((uint8_t)(c ^ d)));
        state[offset + 3U] ^= (uint8_t)(common ^ xtime((uint8_t)(d ^ a)));
    }
}

static void aes_encrypt(const uint8_t key[16], const uint8_t input[16],
                        uint8_t output[16])
{
    uint8_t round_key[176];
    uint8_t state[16];
    uint32_t round;
    uint32_t index;
    aes_key_expand(key, round_key);
    for (index = 0U; index < 16U; ++index) {
        state[index] = input[index];
    }
    add_round_key(state, round_key);
    for (round = 1U; round < 10U; ++round) {
        sub_shift(state);
        mix_columns(state);
        add_round_key(state, &round_key[round * 16U]);
    }
    sub_shift(state);
    add_round_key(state, &round_key[160]);
    for (index = 0U; index < 16U; ++index) {
        output[index] = state[index];
    }
}

static void shift_subkey(const uint8_t input[16], uint8_t output[16])
{
    uint32_t index;
    uint8_t carry = 0U;
    const bool reduce = (input[0] & 0x80U) != 0U;
    for (index = 16U; index > 0U; --index) {
        const uint8_t value = input[index - 1U];
        output[index - 1U] = (uint8_t)((value << 1U) | carry);
        carry = (uint8_t)(value >> 7U);
    }
    if (reduce) {
        output[15] ^= 0x87U;
    }
}

int bb_security_cmac(uint16_t key_slot, const uint8_t *data, size_t length,
                     uint8_t output[16])
{
    uint8_t zero[16] = { 0U };
    uint8_t subkey_one[16];
    uint8_t subkey_two[16];
    uint8_t state[16] = { 0U };
    uint8_t block[16];
    size_t block_count;
    size_t block_index;
    uint32_t index;
    bool complete;
    if ((key_slot != 1U) || (output == NULL) ||
        ((data == NULL) && (length != 0U)) || (length > 65535U)) {
        return -1;
    }
    aes_encrypt(bb_accelerator_test_key, zero, subkey_one);
    shift_subkey(subkey_one, subkey_one);
    shift_subkey(subkey_one, subkey_two);
    block_count = (length + 15U) / 16U;
    if (block_count == 0U) {
        block_count = 1U;
    }
    complete = (length != 0U) && ((length & 15U) == 0U);
    for (block_index = 0U; block_index < block_count; ++block_index) {
        const bool last = (block_index + 1U) == block_count;
        for (index = 0U; index < 16U; ++index) {
            const size_t position = block_index * 16U + index;
            uint8_t value = (position < length) ? data[position] : 0U;
            if (last && !complete && (position == length)) {
                value = 0x80U;
            }
            if (last) {
                value = (uint8_t)(value ^
                        (complete ? subkey_one[index] : subkey_two[index]));
            }
            block[index] = (uint8_t)(state[index] ^ value);
        }
        aes_encrypt(bb_accelerator_test_key, block, state);
    }
    for (index = 0U; index < 16U; ++index) {
        output[index] = state[index];
    }
    return 0;
}

bool bb_security_verify(uint16_t key_slot, const uint8_t *data, size_t length,
                        const uint8_t tag[8])
{
    uint8_t computed[16];
    uint8_t difference = 0U;
    uint32_t index;
    if ((tag == NULL) ||
        (bb_security_cmac(key_slot, data, length, computed) != 0)) {
        return false;
    }
    for (index = 0U; index < 8U; ++index) {
        difference |= (uint8_t)(computed[index] ^ tag[index]);
    }
    return difference == 0U;
}

int bb_security_nia2(uint16_t key_slot, uint32_t count, uint8_t bearer,
                     uint8_t direction, const uint8_t *message, size_t length,
                     uint8_t output[16])
{
    uint8_t authenticated[8U + BB_MAX_TB_BYTES];
    size_t index;
    if ((bearer >= 32U) || (direction >= 2U) ||
        ((message == NULL) && (length != 0U)) ||
        (length > BB_MAX_TB_BYTES) || (output == NULL)) {
        return -1;
    }
    authenticated[0] = (uint8_t)(count >> 24U);
    authenticated[1] = (uint8_t)(count >> 16U);
    authenticated[2] = (uint8_t)(count >> 8U);
    authenticated[3] = (uint8_t)count;
    authenticated[4] = (uint8_t)((bearer << 3U) | (direction << 2U));
    authenticated[5] = 0U;
    authenticated[6] = 0U;
    authenticated[7] = 0U;
    for (index = 0U; index < length; ++index) {
        authenticated[8U + index] = message[index];
    }
    return bb_security_cmac(key_slot, authenticated, length + 8U, output);
}

static uint32_t crypto_descriptor_address(uint32_t slot)
{
    return 0x50000000U + slot * 64U;
}

static uint32_t crypto_source_address(uint32_t slot)
{
    return 0x40010000U + slot * 0x1000U;
}

static uint32_t crypto_destination_address(uint32_t slot)
{
    return 0x40020000U + slot * 0x1000U;
}

static uint8_t *crypto_resolve_address(struct bb_modem *modem,
                                       uint32_t address, uint32_t length)
{
    uint32_t slot;
    if ((modem == NULL) || (length == 0U) ||
        (length > BB_MAX_TB_BYTES)) {
        return NULL;
    }
    for (slot = 0U; slot < BB_CRYPTO_JOB_SLOTS; ++slot) {
        if (address == crypto_source_address(slot)) {
            return modem->crypto_jobs[slot].input;
        }
        if (address == crypto_destination_address(slot)) {
            return modem->crypto_jobs[slot].output;
        }
    }
    return NULL;
}

void bb_crypto_reset(struct bb_modem *modem)
{
    uint32_t index;
    if (modem == NULL) {
        return;
    }
    for (index = 0U; index < BB_CRYPTO_JOB_SLOTS; ++index) {
        modem->crypto_jobs[index].state = (uint8_t)BB_CRYPTO_JOB_IDLE;
        modem->crypto_jobs[index].require_mask = 0U;
        modem->crypto_jobs[index].operation = 0U;
        modem->crypto_jobs[index].reserved = 0U;
        modem->crypto_jobs[index].length = 0U;
        modem->crypto_jobs[index].result = -1;
        modem->crypto_jobs[index].transmit_tick = 0U;
        modem->crypto_jobs[index].generation++;
        if (modem->crypto_jobs[index].generation == 0U) {
            modem->crypto_jobs[index].generation = 1U;
        }
    }
}

static int crypto_submit_mac(struct bb_modem *modem, const uint8_t *data,
                             size_t length, uint8_t require_mask,
                             uint8_t operation, uint64_t tick)
{
    uint32_t slot_index;
    struct bb_crypto_job *job = NULL;
    struct bb_dma_descriptor descriptor;
    size_t index;
    if ((modem == NULL) || (data == NULL) || (length == 0U) ||
        (length > BB_MAX_TB_BYTES) || (require_mask == 0U) ||
        ((operation != (uint8_t)BB_CRYPTO_VERIFY_MAC) &&
         (operation != (uint8_t)BB_CRYPTO_SIGN_MAC)) ||
        (modem->stack.key_slot != 1U)) {
        return -1;
    }
    for (slot_index = 0U; slot_index < BB_CRYPTO_JOB_SLOTS; ++slot_index) {
        if (modem->crypto_jobs[slot_index].state ==
            (uint8_t)BB_CRYPTO_JOB_IDLE) {
            job = &modem->crypto_jobs[slot_index];
            break;
        }
    }
    if (job == NULL) {
        return -2;
    }
    for (index = 0U; index < length; ++index) {
        job->input[index] = data[index];
    }
    modem->crypto_cookie++;
    if (modem->crypto_cookie == 0U) {
        modem->crypto_cookie = 1U;
    }
    job->cookie = modem->crypto_cookie;
    job->key_slot = modem->stack.key_slot;
    job->length = (uint16_t)length;
    job->require_mask = require_mask;
    job->operation = operation;
    job->reserved = 0U;
    job->result = -1;
    job->transmit_tick = tick;
    descriptor.source = crypto_source_address(slot_index);
    descriptor.destination = crypto_destination_address(slot_index);
    descriptor.length = (uint32_t)length;
    descriptor.flags = BB_DMA_FLAG_EOP | BB_DMA_FLAG_IRQ;
    if (operation == (uint8_t)BB_CRYPTO_VERIFY_MAC) {
        descriptor.flags |= BB_DMA_FLAG_TO_DEVICE;
    }
    descriptor.cookie = job->cookie;
    descriptor.generation = job->generation;
    descriptor.next = 0U;
    descriptor.reserved = 0U;
    if (bb_dma_descriptor_encode(&descriptor, job->descriptor) != 0) {
        return -3;
    }
    if (modem->plat.cache_clean != NULL) {
        modem->plat.cache_clean(modem->plat_ctx, job->input, length);
        modem->plat.cache_clean(modem->plat_ctx, job->descriptor,
                                BB_DMA_DESCRIPTOR_SIZE);
    }
    atomic_thread_fence(memory_order_release);
    descriptor.flags |= BB_DMA_FLAG_OWN;
    if (bb_dma_descriptor_encode(&descriptor, job->descriptor) != 0) {
        return -3;
    }
    if (modem->plat.cache_clean != NULL) {
        modem->plat.cache_clean(modem->plat_ctx, job->descriptor,
                                BB_DMA_DESCRIPTOR_SIZE);
    }
    atomic_thread_fence(memory_order_release);
    job->state = (uint8_t)BB_CRYPTO_JOB_SUBMITTED;
    (void)bb_mmio_write32(modem, 0x60007008U,
                          crypto_descriptor_address(slot_index));
    (void)bb_mmio_write32(modem, 0x6000700CU, 1U);
    (void)bb_mmio_write32(modem, 0x60007000U, 0x5U);
    (void)bb_mmio_write32(modem, 0x60007018U, 1U);
    bb_trace(&modem->cfg, tick,
             (operation == (uint8_t)BB_CRYPTO_VERIFY_MAC) ?
             "CRYPTO_VERIFY_SUBMIT" : "CRYPTO_SIGN_SUBMIT", job->cookie);
    return 0;
}

int bb_crypto_submit_mac(struct bb_modem *modem, const uint8_t *data,
                         size_t length, uint8_t require_mask, uint64_t tick)
{
    return crypto_submit_mac(modem, data, length, require_mask,
                             (uint8_t)BB_CRYPTO_VERIFY_MAC, tick);
}

int bb_crypto_submit_mac_sign(struct bb_modem *modem, const uint8_t *data,
                              size_t length, uint8_t require_mask,
                              uint64_t tick)
{
    return crypto_submit_mac(modem, data, length, require_mask,
                             (uint8_t)BB_CRYPTO_SIGN_MAC, tick);
}

int bb_crypto_accelerator_step(struct bb_modem *modem)
{
    uint32_t slot_index;
    int completed = 0;
    if (modem == NULL) {
        return -1;
    }
    for (slot_index = 0U; slot_index < BB_CRYPTO_JOB_SLOTS; ++slot_index) {
        struct bb_crypto_job *const job = &modem->crypto_jobs[slot_index];
        struct bb_mac_sdu sdus[BB_MAC_MAX_SDUS];
        struct bb_dma_descriptor descriptor;
        size_t count = 0U;
        size_t index;
        uint32_t expected_flags;
        uint8_t *dma_input = NULL;
        uint8_t *dma_output = NULL;
        int result = 0;
        if (job->state != (uint8_t)BB_CRYPTO_JOB_SUBMITTED) {
            continue;
        }
        atomic_thread_fence(memory_order_acquire);
        expected_flags = BB_DMA_FLAG_OWN | BB_DMA_FLAG_EOP |
                         BB_DMA_FLAG_IRQ;
        if (job->operation == (uint8_t)BB_CRYPTO_VERIFY_MAC) {
            expected_flags |= BB_DMA_FLAG_TO_DEVICE;
        }
        if ((job->key_slot != 1U) || (job->length == 0U) ||
            ((job->operation != (uint8_t)BB_CRYPTO_VERIFY_MAC) &&
             (job->operation != (uint8_t)BB_CRYPTO_SIGN_MAC)) ||
            (bb_dma_ring_validate(job->descriptor, 1U,
                                  crypto_descriptor_address(slot_index)) != 0) ||
            (bb_dma_descriptor_decode(job->descriptor, &descriptor) != 0) ||
            (descriptor.flags != expected_flags) ||
            (descriptor.cookie != job->cookie) ||
            (descriptor.generation != job->generation) ||
            (descriptor.length != job->length) ||
            (descriptor.source != crypto_source_address(slot_index)) ||
            (descriptor.destination !=
             crypto_destination_address(slot_index))) {
            result = -1;
        }
        if (result == 0) {
            dma_input = crypto_resolve_address(modem, descriptor.source,
                                               descriptor.length);
            dma_output = crypto_resolve_address(modem, descriptor.destination,
                                                descriptor.length);
            if ((dma_input == NULL) || (dma_output == NULL)) {
                result = -1;
            }
        }
        if (result == 0) {
            for (index = 0U; index < job->length; ++index) {
                dma_output[index] = dma_input[index];
            }
        }
        if ((result == 0) &&
            ((bb_mac_demultiplex(dma_input, job->length, sdus,
                                BB_MAC_MAX_SDUS, &count) != 0) ||
             ((uint32_t)job->require_mask >= (UINT32_C(1) << count)))) {
            result = -1;
        }
        for (index = 0U; (result == 0) && (index < count); ++index) {
            if ((job->require_mask & (uint8_t)(1U << index)) != 0U) {
                struct bb_wire_pdu pdu;
                uint32_t budget = 1000U;
                uint32_t full_count;
                if (bb_protocol_decode(sdus[index].data, sdus[index].length,
                                       &pdu, false, &budget) != 0) {
                    result = -2;
                    continue;
                }
                full_count = bb_pdcp_reconstruct_count(&modem->stack,
                                                        pdu.pdcp_count);
                if (job->operation == (uint8_t)BB_CRYPTO_VERIFY_MAC) {
                    budget = 1000U;
                    if (bb_protocol_decode_with_count(
                            sdus[index].data, sdus[index].length, &pdu, true,
                            full_count, &budget) != 0) {
                        result = -2;
                    }
                } else if (job->operation ==
                           (uint8_t)BB_CRYPTO_SIGN_MAC) {
                    uint8_t authenticated[20U + BB_WIRE_MAX_PAYLOAD];
                    uint8_t tag[16];
                    const size_t wire_offset =
                        (size_t)(sdus[index].data - dma_input);
                    size_t byte_index;
                    for (byte_index = 0U; byte_index < 20U; ++byte_index) {
                        authenticated[byte_index] = sdus[index].data[byte_index];
                    }
                    for (byte_index = 0U; byte_index < pdu.payload_length;
                         ++byte_index) {
                        authenticated[20U + byte_index] =
                            pdu.payload[byte_index];
                    }
                    if (bb_security_nia2(
                            job->key_slot, full_count, 1U,
                            (uint8_t)((pdu.flags >> 1U) & 1U), authenticated,
                            20U + pdu.payload_length, tag) != 0) {
                        result = -3;
                    } else {
                        for (byte_index = 0U; byte_index < 8U; ++byte_index) {
                            dma_output[wire_offset + 20U + byte_index] =
                                tag[byte_index];
                        }
                    }
                }
            }
        }
        job->result = result;
        atomic_thread_fence(memory_order_release);
        job->state = (uint8_t)BB_CRYPTO_JOB_COMPLETED;
        completed++;
    }
    return completed;
}

bool bb_crypto_has_completion(const struct bb_modem *modem)
{
    uint32_t index;
    if (modem == NULL) {
        return false;
    }
    for (index = 0U; index < BB_CRYPTO_JOB_SLOTS; ++index) {
        if (modem->crypto_jobs[index].state ==
            (uint8_t)BB_CRYPTO_JOB_COMPLETED) {
            return true;
        }
    }
    return false;
}

void bb_crypto_complete_irq(struct bb_modem *modem, uint64_t tick)
{
    uint32_t index;
    if (modem == NULL) {
        return;
    }
    for (index = 0U; index < BB_CRYPTO_JOB_SLOTS; ++index) {
        struct bb_crypto_job *const job = &modem->crypto_jobs[index];
        int result;
        if (job->state != (uint8_t)BB_CRYPTO_JOB_COMPLETED) {
            continue;
        }
        atomic_thread_fence(memory_order_acquire);
        if (modem->plat.cache_invalidate != NULL) {
            modem->plat.cache_invalidate(modem->plat_ctx, job->descriptor,
                                         BB_DMA_DESCRIPTOR_SIZE);
            if (job->operation == (uint8_t)BB_CRYPTO_SIGN_MAC) {
                modem->plat.cache_invalidate(modem->plat_ctx, job->output,
                                             job->length);
            }
        }
        result = job->result;
        bb_trace(&modem->cfg, tick, "CRYPTO_IRQ_COMPLETE",
                 (result == 0) ? job->cookie : result);
        if ((result == 0) &&
            (job->operation == (uint8_t)BB_CRYPTO_VERIFY_MAC)) {
            (void)bb_mac_process_tb_verified(modem, job->input, job->length,
                                             tick);
        } else if ((result == 0) &&
                   (job->operation == (uint8_t)BB_CRYPTO_SIGN_MAC)) {
            if (bb_modem_send_tb(modem, job->output, job->length,
                                 job->transmit_tick) != 0) {
                result = -4;
                if (modem->stack.bsr_bytes >= job->length) {
                    modem->stack.bsr_bytes -= job->length;
                }
                modem->stats.dropped++;
                bb_trace(&modem->cfg, tick, "CRYPTO_SIGN_TX_DROP", result);
            }
        } else {
            modem->stats.dropped++;
            bb_trace(&modem->cfg, tick, "INTEGRITY_DROP", result);
        }
        job->state = (uint8_t)BB_CRYPTO_JOB_IDLE;
        job->length = 0U;
        job->require_mask = 0U;
        job->operation = 0U;
        job->transmit_tick = 0U;
        job->generation++;
        if (job->generation == 0U) {
            job->generation = 1U;
        }
    }
}
