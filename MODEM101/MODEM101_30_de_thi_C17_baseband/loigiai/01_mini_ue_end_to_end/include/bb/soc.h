#ifndef BB_SOC_H
#define BB_SOC_H

#include <stddef.h>
#include <stdint.h>

#define BB_DMA_DESCRIPTOR_SIZE 32U
#define BB_DMA_FLAG_OWN 0x01U
#define BB_DMA_FLAG_EOP 0x02U
#define BB_DMA_FLAG_IRQ 0x04U
#define BB_DMA_FLAG_TO_DEVICE 0x08U

struct bb_dma_descriptor {
    uint32_t source;
    uint32_t destination;
    uint32_t length;
    uint32_t flags;
    uint16_t cookie;
    uint16_t generation;
    uint32_t next;
    uint32_t reserved;
};

int bb_dma_descriptor_encode(const struct bb_dma_descriptor *descriptor,
                             uint8_t output[BB_DMA_DESCRIPTOR_SIZE]);
int bb_dma_descriptor_decode(const uint8_t input[BB_DMA_DESCRIPTOR_SIZE],
                             struct bb_dma_descriptor *descriptor);
int bb_dma_ring_validate(const uint8_t *descriptors, uint32_t count,
                         uint32_t physical_base);

#endif

