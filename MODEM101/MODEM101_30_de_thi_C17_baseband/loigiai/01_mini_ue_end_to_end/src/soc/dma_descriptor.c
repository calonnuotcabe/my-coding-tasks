#include "bb/ipc.h"
#include "bb/soc.h"

#include <stdbool.h>

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

static bool in_region(uint32_t address, uint32_t length,
                      uint32_t base, uint32_t size)
{
    if ((length == 0U) || (address < base) || (address - base >= size)) {
        return false;
    }
    return length <= size - (address - base);
}

static bool valid_buffer(uint32_t address, uint32_t length)
{
    return in_region(address, length, 0x00100000U, 0x00080000U) ||
           in_region(address, length, 0x10000000U, 0x00200000U) ||
           /* Reserve the final 64 KiB DDR page for accelerator-only keys. */
           in_region(address, length, 0x40000000U, 0x03FF0000U) ||
           in_region(address, length, 0x50000000U, 0x00400000U);
}

int bb_dma_descriptor_encode(const struct bb_dma_descriptor *descriptor,
                             uint8_t output[BB_DMA_DESCRIPTOR_SIZE])
{
    uint32_t crc;
    if ((descriptor == NULL) || (output == NULL) ||
        !valid_buffer(descriptor->source, descriptor->length) ||
        !valid_buffer(descriptor->destination, descriptor->length) ||
        ((descriptor->flags & ~0x0FU) != 0U) ||
        (descriptor->reserved != 0U) ||
        ((descriptor->next != 0U) && ((descriptor->next & 31U) != 0U))) {
        return -1;
    }
    put_u32(&output[0], descriptor->source);
    put_u32(&output[4], descriptor->destination);
    put_u32(&output[8], descriptor->length);
    put_u32(&output[12], descriptor->flags);
    put_u16(&output[16], descriptor->cookie);
    put_u16(&output[18], descriptor->generation);
    put_u32(&output[20], descriptor->next);
    put_u32(&output[24], descriptor->reserved);
    crc = bb_crc32c(output, 28U);
    put_u32(&output[28], crc);
    return 0;
}

int bb_dma_descriptor_decode(const uint8_t input[BB_DMA_DESCRIPTOR_SIZE],
                             struct bb_dma_descriptor *descriptor)
{
    if ((input == NULL) || (descriptor == NULL) ||
        (bb_crc32c(input, 28U) != get_u32(&input[28]))) {
        return -1;
    }
    descriptor->source = get_u32(&input[0]);
    descriptor->destination = get_u32(&input[4]);
    descriptor->length = get_u32(&input[8]);
    descriptor->flags = get_u32(&input[12]);
    descriptor->cookie = get_u16(&input[16]);
    descriptor->generation = get_u16(&input[18]);
    descriptor->next = get_u32(&input[20]);
    descriptor->reserved = get_u32(&input[24]);
    if (!valid_buffer(descriptor->source, descriptor->length) ||
        !valid_buffer(descriptor->destination, descriptor->length) ||
        ((descriptor->flags & ~0x0FU) != 0U) ||
        (descriptor->reserved != 0U) ||
        ((descriptor->next != 0U) && ((descriptor->next & 31U) != 0U))) {
        return -2;
    }
    return 0;
}

int bb_dma_ring_validate(const uint8_t *descriptors, uint32_t count,
                         uint32_t physical_base)
{
    bool visited[256];
    uint32_t index = 0U;
    uint32_t traversed;
    if ((descriptors == NULL) || (count == 0U) || (count > 256U) ||
        ((physical_base & 31U) != 0U) ||
        (count > (UINT32_MAX - physical_base) / BB_DMA_DESCRIPTOR_SIZE)) {
        return -1;
    }
    for (traversed = 0U; traversed < 256U; ++traversed) {
        visited[traversed] = false;
    }
    for (traversed = 0U; traversed < count; ++traversed) {
        struct bb_dma_descriptor descriptor;
        uint32_t next_index;
        if (visited[index] ||
            (bb_dma_descriptor_decode(
                &descriptors[index * BB_DMA_DESCRIPTOR_SIZE], &descriptor) != 0)) {
            return -2;
        }
        visited[index] = true;
        if (descriptor.next == 0U) {
            return (traversed + 1U == count) ? 0 : -4;
        }
        if ((descriptor.next < physical_base) ||
            (((descriptor.next - physical_base) & 31U) != 0U)) {
            return -3;
        }
        next_index = (descriptor.next - physical_base) / BB_DMA_DESCRIPTOR_SIZE;
        if (next_index >= count) {
            return -3;
        }
        index = next_index;
    }
    return -4;
}
