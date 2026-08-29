#include "internal.h"

void bb_pool_init(struct bb_pool *pool)
{
    uint32_t slot_index;
    if (pool == NULL) {
        return;
    }
    for (slot_index = 0U; slot_index < BB_POOL_SLOTS; ++slot_index) {
        pool->slots[slot_index].generation = 1U;
        pool->slots[slot_index].state = (uint16_t)BB_DMA_FREE;
        pool->slots[slot_index].length = 0U;
    }
}

int bb_pool_alloc(struct bb_pool *pool, uint32_t length,
                  struct bb_buf_handle *handle, uint8_t **data)
{
    uint32_t slot_index;
    if ((pool == NULL) || (handle == NULL) || (data == NULL) ||
        (length == 0U) || (length > BB_POOL_BYTES)) {
        return -1;
    }
    for (slot_index = 0U; slot_index < BB_POOL_SLOTS; ++slot_index) {
        struct bb_pool_slot *const slot = &pool->slots[slot_index];
        if (slot->state == (uint16_t)BB_DMA_FREE) {
            slot->state = (uint16_t)BB_DMA_CPU_OWNED;
            slot->length = length;
            handle->slot = (uint16_t)slot_index;
            handle->generation = slot->generation;
            handle->length = length;
            *data = slot->data;
            return 0;
        }
    }
    return -2;
}

int bb_pool_get(struct bb_pool *pool, struct bb_buf_handle handle,
                uint8_t **data)
{
    struct bb_pool_slot *slot;
    if ((pool == NULL) || (data == NULL) ||
        ((uint32_t)handle.slot >= BB_POOL_SLOTS)) {
        return -1;
    }
    slot = &pool->slots[handle.slot];
    if ((slot->generation != handle.generation) ||
        (slot->state != (uint16_t)BB_DMA_CPU_OWNED) ||
        (handle.length != slot->length)) {
        return -2;
    }
    *data = slot->data;
    return 0;
}

int bb_pool_release(struct bb_pool *pool, struct bb_buf_handle handle)
{
    struct bb_pool_slot *slot;
    if ((pool == NULL) || ((uint32_t)handle.slot >= BB_POOL_SLOTS)) {
        return -1;
    }
    slot = &pool->slots[handle.slot];
    if ((slot->generation != handle.generation) ||
        (slot->state != (uint16_t)BB_DMA_CPU_OWNED)) {
        return -2;
    }
    slot->state = (uint16_t)BB_DMA_FREE;
    slot->length = 0U;
    slot->generation++;
    if (slot->generation == 0U) {
        slot->generation = 1U;
    }
    return 0;
}
