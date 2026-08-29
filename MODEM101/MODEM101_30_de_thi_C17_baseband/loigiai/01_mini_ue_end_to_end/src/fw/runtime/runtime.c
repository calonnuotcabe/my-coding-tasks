#include "internal.h"

#include <stdint.h>

#define BB_RF_DMA_DESCRIPTOR_BASE 0x50010000U
#define BB_RF_DMA_CPU_BUFFER_BASE 0x40080000U
#define BB_RF_DMA_DEVICE_BUFFER_BASE 0x400C0000U
#define BB_RF_DMA_BUFFER_STRIDE 0x00010000U

_Static_assert((BB_EVENT_RING_STORAGE & (BB_EVENT_RING_STORAGE - 1U)) == 0U,
               "event ring storage must be a power of two");

#if defined(__arm__)
typedef void (*bb_arch_task_entry)(struct bb_modem *,
                                   const struct bb_event *);
void bb_arch_run_task(bb_arch_task_entry entry, struct bb_modem *modem,
                      const struct bb_event *event, void *stack_top);
#elif defined(BB_HOST_TASK_STACK_SWITCH)
typedef void (*bb_arch_task_entry)(struct bb_modem *,
                                   const struct bb_event *);
void bb_arch_run_task_host(bb_arch_task_entry entry, struct bb_modem *modem,
                           const struct bb_event *event, void *stack_top);
#endif

static void dma_release(struct bb_dma_slot *slot);

static void zero_bytes(void *destination, size_t length)
{
    uint8_t *const bytes = (uint8_t *)destination;
    size_t index;
    for (index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

int bb_ring_push(struct bb_event_ring *ring, const struct bb_event *ev)
{
    uint32_t write_index;
    uint32_t read_index;

    if ((ring == NULL) || (ev == NULL)) {
        return -1;
    }
    write_index = atomic_load_explicit(&ring->write_index, memory_order_relaxed);
    read_index = atomic_load_explicit(&ring->read_index, memory_order_acquire);
    if ((ring->capacity == 0U) ||
        ((ring->capacity & (ring->capacity - 1U)) != 0U) ||
        (ring->capacity > BB_EVENT_RING_STORAGE) ||
        ((write_index - read_index) >= ring->capacity)) {
        return -2;
    }
    ring->entries[write_index & (ring->capacity - 1U)] = *ev;
    atomic_store_explicit(&ring->write_index, write_index + 1U,
                          memory_order_release);
    return 0;
}

int bb_ring_pop(struct bb_event_ring *ring, struct bb_event *ev)
{
    uint32_t write_index;
    uint32_t read_index;

    if ((ring == NULL) || (ev == NULL)) {
        return -1;
    }
    if ((ring->capacity == 0U) ||
        ((ring->capacity & (ring->capacity - 1U)) != 0U) ||
        (ring->capacity > BB_EVENT_RING_STORAGE)) {
        return -2;
    }
    read_index = atomic_load_explicit(&ring->read_index, memory_order_relaxed);
    write_index = atomic_load_explicit(&ring->write_index, memory_order_acquire);
    if (read_index == write_index) {
        return -2;
    }
    *ev = ring->entries[read_index & (ring->capacity - 1U)];
    atomic_store_explicit(&ring->read_index, read_index + 1U,
                          memory_order_release);
    return 0;
}

int bb_ring_peek(struct bb_event_ring *ring, struct bb_event *ev)
{
    uint32_t read_index;
    uint32_t write_index;
    if ((ring == NULL) || (ev == NULL)) {
        return -1;
    }
    if ((ring->capacity == 0U) ||
        ((ring->capacity & (ring->capacity - 1U)) != 0U) ||
        (ring->capacity > BB_EVENT_RING_STORAGE)) {
        return -2;
    }
    read_index = atomic_load_explicit(&ring->read_index, memory_order_relaxed);
    write_index = atomic_load_explicit(&ring->write_index, memory_order_acquire);
    if (read_index == write_index) {
        return -2;
    }
    *ev = ring->entries[read_index & (ring->capacity - 1U)];
    return 0;
}

size_t bb_modem_required_memory(const struct bb_config *cfg)
{
    (void)cfg;
    return sizeof(struct bb_modem) + _Alignof(struct bb_modem) - 1U;
}

int bb_modem_init(void *arena, size_t arena_len, const struct bb_config *cfg,
                  const struct bb_platform_ops *plat, void *plat_ctx,
                  struct bb_modem **out)
{
    uintptr_t base;
    uintptr_t aligned;
    size_t skipped;
    struct bb_modem *modem;

    if ((arena == NULL) || (cfg == NULL) || (plat == NULL) || (out == NULL) ||
        (plat->ticks == NULL) || (plat->mmio_read32 == NULL) ||
        (plat->mmio_write32 == NULL) || (plat->cache_clean == NULL) ||
        (plat->cache_invalidate == NULL)) {
        return -1;
    }
    if ((cfg->sample_rate_hz == 0U) ||
        ((cfg->modulation != (uint8_t)BB_MOD_QPSK) &&
         (cfg->modulation != (uint8_t)BB_MOD_16QAM)) ||
        (cfg->pci > 1007U) ||
        (cfg->timing_offset_samples > BB_MAX_IQ_SAMPLES -
                                             19U * (BB_NFFT + BB_CP_LEN)) ||
        (arena_len < bb_modem_required_memory(cfg))) {
        return -2;
    }
    base = (uintptr_t)arena;
    aligned = (base + _Alignof(struct bb_modem) - 1U) &
              ~((uintptr_t)_Alignof(struct bb_modem) - 1U);
    skipped = (size_t)(aligned - base);
    if ((skipped > arena_len) ||
        ((arena_len - skipped) < sizeof(struct bb_modem))) {
        return -2;
    }
    modem = (struct bb_modem *)aligned;
    zero_bytes(modem, sizeof(*modem));
    modem->cfg = *cfg;
    modem->plat = *plat;
    modem->plat_ctx = plat_ctx;
    modem->next_slot = BB_SLOT_TICKS;
    modem->irq_ring.capacity = 64U;
    {
        static const uint32_t queue_depths[BB_TASK_QUEUE_COUNT] = {
            64U, 64U, 128U, 128U, 128U, 64U, 32U, 64U
        };
        uint32_t queue_index;
        for (queue_index = 0U; queue_index < BB_TASK_QUEUE_COUNT;
             ++queue_index) {
            modem->task_queues[queue_index].capacity = queue_depths[queue_index];
        }
    }
    bb_pool_init(&modem->pool);
    {
        uint32_t slot_index;
        for (slot_index = 0U; slot_index < BB_DMA_SLOTS; ++slot_index) {
            modem->dma[slot_index].generation = 1U;
            modem->dma[slot_index].state = (uint8_t)BB_DMA_FREE;
        }
    }
    bb_stack_init(&modem->stack);
    bb_crypto_reset(modem);
    {
        static const uint16_t priorities[BB_SCHEDULED_TASKS] = {
            28U, 27U, 24U, 20U, 18U, 15U, 14U, 12U, 4U, 0U
        };
        static const uint32_t budgets[BB_SCHEDULED_TASKS] = {
            16384U, 16384U, 12288U, 16384U, 12288U,
            16384U, 16384U, 12288U, 8192U, 1024U
        };
        uint32_t task_index;
        for (task_index = 0U; task_index < BB_SCHEDULED_TASKS; ++task_index) {
            uint32_t stack_byte;
            modem->task_info[task_index].priority = priorities[task_index];
            modem->task_info[task_index].budget_bytes = budgets[task_index];
            modem->task_info[task_index].high_water_bytes = 0U;
            modem->task_info[task_index].guard_low = BB_STACK_CANARY;
            modem->task_info[task_index].guard_high = BB_STACK_CANARY;
            modem->task_stacks[task_index].guard_low = BB_STACK_CANARY;
            modem->task_stacks[task_index].guard_high = BB_STACK_CANARY;
            for (stack_byte = 0U;
                 stack_byte < BB_TASK_STACK_STORAGE_BYTES; ++stack_byte) {
                modem->task_stacks[task_index].bytes[stack_byte] =
                    (uint8_t)BB_STACK_FILL;
            }
        }
    }
    *out = modem;
    return 0;
}

static uint32_t task_queue_index(uint16_t event_type)
{
    if (event_type == (uint16_t)BB_EVENT_RX_DEFERRED) return 0U;
    if (event_type == (uint16_t)BB_EVENT_TX_REQUEST) return 1U;
    if (event_type == (uint16_t)BB_EVENT_MAC_RX) return 2U;
    if (event_type == (uint16_t)BB_EVENT_RLC_RX) return 3U;
    if (event_type == (uint16_t)BB_EVENT_PDCP_RX) return 4U;
    if (event_type == (uint16_t)BB_EVENT_CONTROL_RX) return 5U;
    if (event_type == (uint16_t)BB_EVENT_NAS_RX) return 6U;
    if (event_type == (uint16_t)BB_EVENT_CRYPTO_COMPLETE) return 4U;
    return 7U;
}

static int task_push(struct bb_modem *modem, const struct bb_event *event)
{
    return bb_ring_push(&modem->task_queues[task_queue_index(event->type)],
                        event);
}

int bb_modem_enqueue_stage_copy(struct bb_modem *modem, const uint8_t *data,
                                size_t length, uint64_t tick, uint16_t stage)
{
    struct bb_buf_handle handle;
    struct bb_event event;
    uint8_t *buffer;
    size_t index;
    if ((modem == NULL) || (data == NULL) || (length == 0U) ||
        (length > BB_POOL_BYTES) ||
        (stage < (uint16_t)BB_EVENT_MAC_RX) ||
        (stage > (uint16_t)BB_EVENT_NAS_RX)) {
        return -1;
    }
    if (bb_pool_alloc(&modem->pool, (uint32_t)length, &handle, &buffer) != 0) {
        return -2;
    }
    for (index = 0U; index < length; ++index) {
        buffer[index] = data[index];
    }
    event.tick = tick;
    event.source = 3U;
    event.type = stage;
    event.payload = handle;
    if (task_push(modem, &event) != 0) {
        (void)bb_pool_release(&modem->pool, handle);
        return -3;
    }
    return 0;
}

static int task_find_due(struct bb_modem *modem, uint64_t tick,
                         struct bb_event *event, uint32_t *selected_queue)
{
    uint32_t queue_index;
    bool found = false;
    for (queue_index = 0U; queue_index < BB_TASK_QUEUE_COUNT; ++queue_index) {
        struct bb_event candidate;
        if ((bb_ring_peek(&modem->task_queues[queue_index], &candidate) == 0) &&
            bb_tick_due(tick, candidate.tick) &&
            (!found || bb_tick_before(candidate.tick, event->tick))) {
            *event = candidate;
            *selected_queue = queue_index;
            found = true;
        }
    }
    return found ? 0 : -1;
}

static bool task_account_stack(struct bb_modem *modem,
                               const struct bb_event *event)
{
    /* Each task owns a fixed downward-growing stack region.  These footprints
     * reserve the audited worst-case C call chain/workspace for the event;
     * fill-pattern scanning below records the actual touched high-water. */
    static const uint32_t frame_usage[BB_TASK_QUEUE_COUNT] = {
        14336U, 4096U, 4096U, 2048U, 4096U, 4096U, 3072U, 1024U
    };
    const uint32_t task_index = task_queue_index(event->type);
    struct bb_task_info *const info = &modem->task_info[task_index];
    struct bb_task_stack_storage *const stack =
        &modem->task_stacks[task_index];
#if !defined(__arm__) && !defined(BB_HOST_TASK_STACK_SWITCH)
    uint32_t byte_index;
#endif
    uint32_t high_water = 0U;
    if ((info->guard_low != BB_STACK_CANARY) ||
        (info->guard_high != BB_STACK_CANARY) ||
        (stack->guard_low != BB_STACK_CANARY) ||
        (stack->guard_high != BB_STACK_CANARY) ||
        (info->budget_bytes > BB_TASK_STACK_STORAGE_BYTES) ||
        (frame_usage[task_index] > info->budget_bytes)) {
        modem->supervisor_fault = true;
        bb_trace(&modem->cfg, event->tick, "SUPERVISOR_STACK_FAULT",
                 (int32_t)task_index);
        return false;
    }
#if !defined(__arm__) && !defined(BB_HOST_TASK_STACK_SWITCH)
    /* Portable host fallback for architectures without a stack-switch shim. */
    for (byte_index = 0U; byte_index < frame_usage[task_index]; ++byte_index) {
        stack->bytes[info->budget_bytes - 1U - byte_index] = 0x5AU;
    }
#endif
    while ((high_water < info->budget_bytes) &&
           (stack->bytes[info->budget_bytes - 1U - high_water] !=
            (uint8_t)BB_STACK_FILL)) {
        high_water++;
    }
    if (high_water > info->high_water_bytes) {
        info->high_water_bytes = high_water;
    }
    return true;
}

static bool task_measure_stack(struct bb_modem *modem,
                               const struct bb_event *event)
{
    const uint32_t task_index = task_queue_index(event->type);
    struct bb_task_info *const info = &modem->task_info[task_index];
    const struct bb_task_stack_storage *const stack =
        &modem->task_stacks[task_index];
    uint32_t high_water = 0U;
    if ((info->guard_low != BB_STACK_CANARY) ||
        (info->guard_high != BB_STACK_CANARY) ||
        (stack->guard_low != BB_STACK_CANARY) ||
        (stack->guard_high != BB_STACK_CANARY)) {
        modem->supervisor_fault = true;
        bb_trace(&modem->cfg, event->tick, "SUPERVISOR_STACK_FAULT",
                 (int32_t)task_index);
        return false;
    }
    while ((high_water < info->budget_bytes) &&
           (stack->bytes[info->budget_bytes - 1U - high_water] !=
            (uint8_t)BB_STACK_FILL)) {
        high_water++;
    }
    if (high_water > info->high_water_bytes) {
        info->high_water_bytes = high_water;
    }
    return true;
}

int bb_modem_post(struct bb_modem *modem, const struct bb_event *ev)
{
    int result;
    if ((modem == NULL) || (ev == NULL) ||
        bb_tick_before(ev->tick, modem->now) ||
        (modem->last_post_valid &&
         bb_tick_before(ev->tick, modem->last_post_tick))) {
        return -1;
    }
    result = task_push(modem, ev);
    if (result != 0) {
        modem->stats.dropped++;
        bb_trace(&modem->cfg, modem->now, "QUEUE_FULL", ev->type);
    }
    if (result == 0) {
        modem->last_post_tick = ev->tick;
        modem->last_post_valid = true;
    }
    return result;
}

int bb_modem_irq(struct bb_modem *modem, uint16_t irq)
{
    struct bb_event event;
    int result;

    if (modem == NULL) {
        return -1;
    }
    event.tick = (modem->plat.ticks != NULL) ?
                 modem->plat.ticks(modem->plat_ctx) : modem->now;
    event.source = irq;
    event.type = (uint16_t)BB_EVENT_EXTERNAL;
    event.payload.slot = 0U;
    event.payload.generation = 0U;
    event.payload.length = 0U;
    if ((irq == 8U) && modem->pending_rx_valid) {
        event.tick = modem->dma[modem->pending_rx.slot].t0;
        event.type = (uint16_t)BB_EVENT_RX_DEFERRED;
        event.payload = modem->pending_rx;
        modem->pending_rx_valid = false;
    } else if ((irq == 13U) && bb_crypto_has_completion(modem)) {
        event.type = (uint16_t)BB_EVENT_CRYPTO_COMPLETE;
    } else if ((irq == 16U) && bb_ipc_has_request(modem)) {
        event.type = (uint16_t)BB_EVENT_IPC_REQUEST;
    }
    if ((modem->plat.mmio_read32 != NULL) &&
        (modem->plat.mmio_write32 != NULL)) {
        uintptr_t base = 0x60000000U;
        if ((irq == 1U) || (irq == 2U)) {
            base = 0x60001000U;
        } else if ((irq == 8U) || (irq == 9U)) {
            base = 0x60004000U;
        } else if (irq == 10U) {
            base = 0x60002000U;
        } else if ((irq == 11U) || (irq == 12U)) {
            base = 0x60005000U;
        } else if (irq == 13U) {
            base = 0x60007000U;
        } else if (irq == 16U) {
            base = 0x60008000U;
        }
        {
            uint32_t status = 0U;
            (void)bb_mmio_read32(modem, base + 0x10U, &status);
            if (event.type == (uint16_t)BB_EVENT_EXTERNAL) {
                event.payload.length = status;
            }
            (void)bb_mmio_write32(modem, base + 0x14U, status);
        }
    }
    result = bb_ring_push(&modem->irq_ring, &event);
    if (result != 0) {
        modem->stats.dropped++;
        if (event.type == (uint16_t)BB_EVENT_RX_DEFERRED) {
            dma_release(&modem->dma[event.payload.slot]);
        }
    }
    return result;
}

static int find_free_dma(struct bb_modem *modem, uint32_t *slot_index)
{
    uint32_t index;
    for (index = 0U; index < BB_DMA_SLOTS; ++index) {
        if (modem->dma[index].state == (uint8_t)BB_DMA_FREE) {
            *slot_index = index;
            return 0;
        }
    }
    return -1;
}

static uint32_t dma_descriptor_address(uint32_t slot_index)
{
    return BB_RF_DMA_DESCRIPTOR_BASE +
           slot_index * BB_DMA_DESCRIPTOR_SIZE;
}

static void dma_buffer_addresses(uint32_t slot_index, uint8_t direction,
                                 uint32_t *source, uint32_t *destination)
{
    const uint32_t cpu = BB_RF_DMA_CPU_BUFFER_BASE +
                         slot_index * BB_RF_DMA_BUFFER_STRIDE;
    const uint32_t device = BB_RF_DMA_DEVICE_BUFFER_BASE +
                            slot_index * BB_RF_DMA_BUFFER_STRIDE;
    if (direction == (uint8_t)BB_DMA_TX) {
        *source = cpu;
        *destination = device;
    } else {
        *source = device;
        *destination = cpu;
    }
}

static int dma_validate_descriptor(const struct bb_dma_slot *slot,
                                   uint32_t slot_index, bool owned)
{
    struct bb_dma_descriptor descriptor;
    uint32_t source;
    uint32_t destination;
    uint32_t expected_flags = BB_DMA_FLAG_EOP | BB_DMA_FLAG_IRQ;
    dma_buffer_addresses(slot_index, slot->direction, &source, &destination);
    if (slot->direction == (uint8_t)BB_DMA_TX) {
        expected_flags |= BB_DMA_FLAG_TO_DEVICE;
    }
    if (owned) {
        expected_flags |= BB_DMA_FLAG_OWN;
    }
    if ((bb_dma_ring_validate(slot->descriptor, 1U,
                              dma_descriptor_address(slot_index)) != 0) ||
        (bb_dma_descriptor_decode(slot->descriptor, &descriptor) != 0) ||
        (descriptor.source != source) ||
        (descriptor.destination != destination) ||
        (descriptor.length != slot->count * sizeof(slot->samples[0])) ||
        (descriptor.flags != expected_flags) ||
        (descriptor.cookie != (uint16_t)(slot_index + 1U)) ||
        (descriptor.generation != slot->generation)) {
        return -1;
    }
    return 0;
}

static int dma_prepare_descriptor(struct bb_modem *modem,
                                  struct bb_dma_slot *slot,
                                  uint32_t slot_index)
{
    struct bb_dma_descriptor descriptor;
    dma_buffer_addresses(slot_index, slot->direction, &descriptor.source,
                         &descriptor.destination);
    descriptor.length = slot->count * sizeof(slot->samples[0]);
    descriptor.flags = BB_DMA_FLAG_EOP | BB_DMA_FLAG_IRQ;
    if (slot->direction == (uint8_t)BB_DMA_TX) {
        descriptor.flags |= BB_DMA_FLAG_TO_DEVICE;
    }
    descriptor.cookie = (uint16_t)(slot_index + 1U);
    descriptor.generation = slot->generation;
    descriptor.next = 0U;
    descriptor.reserved = 0U;
    if (bb_dma_descriptor_encode(&descriptor, slot->descriptor) != 0) {
        return -1;
    }
    modem->plat.cache_clean(modem->plat_ctx, slot->descriptor,
                            BB_DMA_DESCRIPTOR_SIZE);
    atomic_thread_fence(memory_order_release);
    descriptor.flags |= BB_DMA_FLAG_OWN;
    if (bb_dma_descriptor_encode(&descriptor, slot->descriptor) != 0) {
        return -1;
    }
    modem->plat.cache_clean(modem->plat_ctx, slot->descriptor,
                            BB_DMA_DESCRIPTOR_SIZE);
    atomic_thread_fence(memory_order_release);
    return dma_validate_descriptor(slot, slot_index, true);
}

static int dma_device_complete(struct bb_dma_slot *slot,
                               uint32_t slot_index)
{
    struct bb_dma_descriptor descriptor;
    if ((dma_validate_descriptor(slot, slot_index, true) != 0) ||
        (bb_dma_descriptor_decode(slot->descriptor, &descriptor) != 0)) {
        return -1;
    }
    descriptor.flags &= ~BB_DMA_FLAG_OWN;
    return bb_dma_descriptor_encode(&descriptor, slot->descriptor);
}

int bb_modem_send_tb(struct bb_modem *modem, const uint8_t *tb, size_t length,
                     uint64_t tick)
{
    struct bb_buf_handle handle;
    struct bb_event event;
    uint8_t *buffer;
    size_t index;
    if ((modem == NULL) || (tb == NULL) || (length == 0U) ||
        (length > BB_MAX_TB_BYTES) || bb_tick_before(tick, modem->now)) {
        return -1;
    }
    if (bb_pool_alloc(&modem->pool, (uint32_t)length, &handle, &buffer) != 0) {
        modem->stats.dropped++;
        return -2;
    }
    for (index = 0U; index < length; ++index) {
        buffer[index] = tb[index];
    }
    event.tick = tick;
    event.source = 9U;
    event.type = (uint16_t)BB_EVENT_TX_REQUEST;
    event.payload = handle;
    if (bb_modem_post(modem, &event) != 0) {
        (void)bb_pool_release(&modem->pool, handle);
        return -2;
    }
    return 0;
}

int bb_modem_rx_iq_submit(struct bb_modem *modem,
                          const struct bb_iq_block *block, uint64_t tick)
{
    uint32_t slot_index;
    uint32_t sample_index;
    struct bb_dma_slot *slot;
    if ((modem == NULL) || (block == NULL) || (block->samples == NULL) ||
        (block->count == 0U) || (block->count > BB_MAX_IQ_SAMPLES) ||
        (block->sample_rate_hz != modem->cfg.sample_rate_hz) ||
        modem->pending_rx_valid || bb_tick_before(tick, modem->now)) {
        return -1;
    }
    if (find_free_dma(modem, &slot_index) != 0) {
        modem->stats.dropped++;
        return -2;
    }
    slot = &modem->dma[slot_index];
    slot->state = (uint8_t)BB_DMA_CPU_OWNED;
    slot->direction = (uint8_t)BB_DMA_RX;
    slot->count = block->count;
    slot->sample_rate_hz = block->sample_rate_hz;
    slot->t0 = tick;
    /* RX is a device-write transfer: publish the destination to DMA, let the
     * simulated device fill it, then invalidate before CPU consumption. */
    if (dma_prepare_descriptor(modem, slot, slot_index) != 0) {
        dma_release(slot);
        modem->stats.dropped++;
        return -3;
    }
    slot->state = (uint8_t)BB_DMA_DMA_OWNED;
    for (sample_index = 0U; sample_index < block->count; ++sample_index) {
        slot->samples[sample_index] = block->samples[sample_index];
    }
    atomic_thread_fence(memory_order_release);
    if (dma_device_complete(slot, slot_index) != 0) {
        dma_release(slot);
        modem->stats.dropped++;
        return -3;
    }
    slot->state = (uint8_t)BB_DMA_DONE;
    modem->pending_rx.slot = (uint16_t)slot_index;
    modem->pending_rx.generation = slot->generation;
    modem->pending_rx.length = block->count * 4U;
    modem->pending_rx_valid = true;
    bb_trace(&modem->cfg, tick, "DMA_RX_DONE", (int32_t)block->count);
    return bb_modem_irq(modem, 8U);
}

static void dma_release(struct bb_dma_slot *slot)
{
    slot->state = (uint8_t)BB_DMA_FREE;
    slot->count = 0U;
    slot->descriptor[0] = 0U;
    slot->generation++;
    if (slot->generation == 0U) {
        slot->generation = 1U;
    }
}

static void task_transmit(struct bb_modem *modem, const struct bb_event *event)
{
    uint8_t *tb;
    uint32_t slot_index;
    struct bb_dma_slot *slot;
    struct bb_iq_block block;
    if (bb_pool_get(&modem->pool, event->payload, &tb) != 0) {
        modem->stats.dropped++;
        return;
    }
    if (modem->stack.bsr_bytes >= event->payload.length) {
        modem->stack.bsr_bytes -= event->payload.length;
        bb_trace(&modem->cfg, event->tick, "MAC_BSR",
                 (int32_t)modem->stack.bsr_bytes);
    }
    if (find_free_dma(modem, &slot_index) != 0) {
        modem->stats.dropped++;
        (void)bb_pool_release(&modem->pool, event->payload);
        return;
    }
    slot = &modem->dma[slot_index];
    slot->state = (uint8_t)BB_DMA_CPU_OWNED;
    slot->direction = (uint8_t)BB_DMA_TX;
    block.t0 = event->tick;
    block.sample_rate_hz = modem->cfg.sample_rate_hz;
    block.count = BB_MAX_IQ_SAMPLES;
    block.samples = slot->samples;
    if (bb_phy_transmit(&modem->cfg, tb, event->payload.length, &block) != 0) {
        modem->stats.dropped++;
        dma_release(slot);
    } else {
        slot->count = block.count;
        slot->sample_rate_hz = block.sample_rate_hz;
        slot->t0 = event->tick;
        if (modem->plat.cache_clean != NULL) {
            modem->plat.cache_clean(modem->plat_ctx, slot->samples,
                                    (size_t)slot->count * sizeof(slot->samples[0]));
        }
        if (dma_prepare_descriptor(modem, slot, slot_index) != 0) {
            modem->stats.dropped++;
            dma_release(slot);
        } else {
            slot->state = (uint8_t)BB_DMA_DMA_OWNED;
            (void)bb_mmio_write32(modem, 0x60004008U,
                                  dma_descriptor_address(slot_index));
            (void)bb_mmio_write32(modem, 0x6000400CU, 1U);
            (void)bb_mmio_write32(modem, 0x60004018U, 1U);
            modem->stats.tx_blocks++;
            bb_trace(&modem->cfg, event->tick, "DMA_TX_OWN",
                     (int32_t)slot_index);
        }
    }
    (void)bb_pool_release(&modem->pool, event->payload);
}

static bool harq_has_soft_state(const struct bb_modem *modem)
{
    uint32_t index;
    for (index = 0U; index < BB_HARQ_PROCESSES; ++index) {
        const struct bb_harq_process *const process =
            &modem->stack.harq[index];
        if ((process->state == (uint8_t)BB_HARQ_WAITING) &&
            (process->soft_length != 0U)) {
            return true;
        }
    }
    return false;
}

static struct bb_harq_process *harq_process_by_id(struct bb_modem *modem,
                                                   int32_t harq_id,
                                                   uint32_t *process_index)
{
    struct bb_harq_process *process;
    if ((harq_id < 0) || ((uint32_t)harq_id >= BB_HARQ_PROCESSES)) {
        return NULL;
    }
    *process_index = (uint32_t)harq_id;
    process = &modem->stack.harq[*process_index];
    return (process->state == (uint8_t)BB_HARQ_WAITING) ? process : NULL;
}

static void harq_store_soft(struct bb_modem *modem,
                            struct bb_harq_process *process,
                            uint32_t process_index, const int16_t *llr,
                            size_t llr_length, uint64_t tick)
{
    size_t index;
    const size_t limit = (llr_length < BB_MAX_FRAME_BITS) ?
                         llr_length : BB_MAX_FRAME_BITS;
    if (process->soft_length == 0U) {
        for (index = 0U; index < limit; ++index) {
            process->soft_llr[index] = llr[index];
        }
    } else {
        const size_t old_length = process->soft_length;
        for (index = 0U; index < limit; ++index) {
            const int32_t prior = (index < old_length) ?
                                  process->soft_llr[index] : 0;
            process->soft_llr[index] = bb_sat16(prior + llr[index]);
        }
    }
    process->soft_length = (uint16_t)limit;
    bb_trace(&modem->cfg, tick, "HARQ_LLR_CAPTURE", (int32_t)process_index);
}

static int32_t decoded_harq_id(const uint8_t *transport_block, size_t length)
{
    struct bb_mac_sdu sdus[BB_MAC_MAX_SDUS];
    size_t count = 0U;
    size_t index;
    if (bb_mac_demultiplex(transport_block, length, sdus,
                           BB_MAC_MAX_SDUS, &count) != 0) {
        return -1;
    }
    for (index = 0U; index < count; ++index) {
        struct bb_wire_pdu pdu;
        uint32_t budget = 1000U;
        if (bb_protocol_decode(sdus[index].data, sdus[index].length,
                               &pdu, false, &budget) == 0) {
            return (int32_t)pdu.harq_id;
        }
    }
    return -1;
}

static void task_receive(struct bb_modem *modem, const struct bb_event *event)
{
    struct bb_dma_slot *slot;
    struct bb_iq_block block;
    uint8_t decoded[BB_MAX_TB_BYTES];
    int16_t llr[BB_MAX_FRAME_BITS];
    size_t decoded_length = 0U;
    size_t llr_length = 0U;
    struct bb_harq_process *process;
    uint32_t process_index = 0U;
    int32_t physical_harq = -1;
    int result;
    if ((uint32_t)event->payload.slot >= BB_DMA_SLOTS) {
        modem->stats.dropped++;
        return;
    }
    slot = &modem->dma[event->payload.slot];
    if ((slot->generation != event->payload.generation) ||
        (slot->state != (uint8_t)BB_DMA_DONE) ||
        (slot->direction != (uint8_t)BB_DMA_RX)) {
        modem->stats.dropped++;
        return;
    }
    if (modem->plat.cache_invalidate != NULL) {
        modem->plat.cache_invalidate(modem->plat_ctx, slot->descriptor,
                                     BB_DMA_DESCRIPTOR_SIZE);
        modem->plat.cache_invalidate(modem->plat_ctx, slot->samples,
                                     (size_t)slot->count * sizeof(slot->samples[0]));
    }
    atomic_thread_fence(memory_order_acquire);
    if (dma_validate_descriptor(slot, event->payload.slot, false) != 0) {
        modem->stats.dropped++;
        bb_trace(&modem->cfg, event->tick, "DMA_DESCRIPTOR_DROP",
                 event->payload.slot);
        dma_release(slot);
        return;
    }
    slot->state = (uint8_t)BB_DMA_CPU_OWNED;
    block.t0 = slot->t0;
    block.sample_rate_hz = slot->sample_rate_hz;
    block.count = slot->count;
    block.samples = slot->samples;
    process = NULL;
    if (harq_has_soft_state(modem)) {
        /* Probe without trace to recover the PHY HARQ metadata, then combine
         * only with that process.  This prevents one pending process from
         * contaminating a different process or an intervening control TB. */
        result = bb_phy_receive_soft_meta(
            &modem->cfg, &block, decoded, sizeof(decoded), &decoded_length,
            NULL, 0U, llr, BB_MAX_FRAME_BITS, &llr_length, &physical_harq,
            false);
        process = harq_process_by_id(modem, physical_harq, &process_index);
        decoded_length = 0U;
        llr_length = 0U;
        if ((process != NULL) && (process->soft_length != 0U)) {
            result = bb_phy_receive_soft_meta(
                &modem->cfg, &block, decoded, sizeof(decoded), &decoded_length,
                process->soft_llr, process->soft_length, llr,
                BB_MAX_FRAME_BITS, &llr_length, &physical_harq, true);
            if (result == 0) {
                modem->stack.harq_combines++;
                bb_trace(&modem->cfg, event->tick, "HARQ_LLR_COMBINE",
                         (int32_t)process_index);
            }
        } else {
            result = bb_phy_receive_soft_meta(
                &modem->cfg, &block, decoded, sizeof(decoded), &decoded_length,
                NULL, 0U, llr, BB_MAX_FRAME_BITS, &llr_length,
                &physical_harq, true);
        }
    } else {
        result = bb_phy_receive_soft_meta(
            &modem->cfg, &block, decoded, sizeof(decoded), &decoded_length,
            NULL, 0U, llr, BB_MAX_FRAME_BITS, &llr_length, &physical_harq,
            true);
        process = harq_process_by_id(modem, physical_harq, &process_index);
    }
    if (result == 0) {
        const int32_t payload_harq = decoded_harq_id(decoded, decoded_length);
        if ((payload_harq >= 0) && (payload_harq != physical_harq)) {
            result = -6;
            bb_trace(&modem->cfg, event->tick, "HARQ_METADATA_DROP",
                     physical_harq);
        }
        bb_trace(&modem->cfg, event->tick, "TB_HARQ",
                 (payload_harq >= 0) ? payload_harq : physical_harq);
    } else {
        bb_trace(&modem->cfg, event->tick, "TB_HARQ",
                 physical_harq);
    }
    if (result == 0) {
        struct bb_buf_handle handle;
        struct bb_event stack_event;
        uint8_t *buffer;
        uint32_t index;
        modem->stats.rx_ok++;
        modem->last_tb_length = (uint32_t)decoded_length;
        modem->last_tb_ready = true;
        for (index = 0U; index < decoded_length; ++index) {
            modem->last_tb[index] = decoded[index];
        }
        if (bb_pool_alloc(&modem->pool, (uint32_t)decoded_length,
                          &handle, &buffer) == 0) {
            for (index = 0U; index < decoded_length; ++index) {
                buffer[index] = decoded[index];
            }
            stack_event.tick = event->tick;
            stack_event.source = 3U;
            stack_event.type = (uint16_t)BB_EVENT_MAC_RX;
            stack_event.payload = handle;
            if (task_push(modem, &stack_event) != 0) {
                (void)bb_pool_release(&modem->pool, handle);
                modem->stats.dropped++;
            }
        } else {
            modem->stats.dropped++;
        }
    } else if (result == -5) {
        modem->stats.rx_crc_fail++;
        if (process != NULL) {
            harq_store_soft(modem, process, process_index, llr, llr_length,
                            event->tick);
        }
    } else {
        modem->stats.dropped++;
        if ((process != NULL) && (llr_length != 0U)) {
            harq_store_soft(modem, process, process_index, llr, llr_length,
                            event->tick);
        }
    }
    dma_release(slot);
}

static void task_stack(struct bb_modem *modem, const struct bb_event *event)
{
    uint8_t *data;
    if (bb_pool_get(&modem->pool, event->payload, &data) == 0) {
        const int next_stage =
            bb_stack_process_stage(modem, data, event->payload.length,
                                   event->tick, event->type);
        if (next_stage > 0) {
            struct bb_event next = *event;
            next.type = (uint16_t)next_stage;
            if (task_push(modem, &next) != 0) {
                modem->stats.dropped++;
                (void)bb_pool_release(&modem->pool, event->payload);
            }
        } else {
            (void)bb_pool_release(&modem->pool, event->payload);
        }
    } else {
        modem->stats.dropped++;
    }
    /* Releasing any stage buffer may create room for another in-order RLC
     * PDU.  The drain is bounded by the fixed pool and reorder window. */
    bb_rlc_drain(modem, event->tick);
}

static void task_dispatch_body(struct bb_modem *modem,
                               const struct bb_event *event)
{
    if (event->type == (uint16_t)BB_EVENT_TX_REQUEST) {
        task_transmit(modem, event);
    } else if (event->type == (uint16_t)BB_EVENT_RX_DEFERRED) {
        task_receive(modem, event);
    } else if ((event->type >= (uint16_t)BB_EVENT_MAC_RX) &&
               (event->type <= (uint16_t)BB_EVENT_NAS_RX)) {
        task_stack(modem, event);
    } else if (event->type == (uint16_t)BB_EVENT_CRYPTO_COMPLETE) {
        bb_crypto_complete_irq(modem, event->tick);
    } else if (event->type == (uint16_t)BB_EVENT_IPC_REQUEST) {
        bb_ipc_task_process(modem, event->tick);
    }
}

int bb_modem_run_until(struct bb_modem *modem, uint64_t tick)
{
    struct bb_event event;
    uint32_t work = 0U;

    if ((modem == NULL) || bb_tick_before(tick, modem->now)) {
        return -1;
    }
    if (modem->supervisor_fault) {
        return -3;
    }
    while (bb_ring_pop(&modem->irq_ring, &event) == 0) {
        if (work++ >= BB_RUN_WORK_BUDGET) {
            return -2;
        }
        if (task_push(modem, &event) != 0) {
            modem->stats.dropped++;
        }
    }
    for (;;) {
        uint32_t queue_index = 0U;
        const bool have_event =
            task_find_due(modem, tick, &event, &queue_index) == 0;
        const bool have_slot = bb_tick_due(tick, modem->next_slot);
        if (!have_event && !have_slot) {
            break;
        }
        if (work++ >= BB_RUN_WORK_BUDGET) {
            bb_trace(&modem->cfg, modem->now, "RUN_BUDGET_EXHAUSTED", 0);
            return -2;
        }
        /* A slot boundary is a timer IRQ and therefore precedes task work at
         * the same timestamp.  Earlier events always precede later slots. */
        if (have_slot &&
            (!have_event || !bb_tick_before(event.tick, modem->next_slot))) {
            modem->now = modem->next_slot;
            bb_trace(&modem->cfg, modem->now, "SLOT", 0);
            bb_stack_check_timers(modem, modem->now);
            modem->next_slot += BB_SLOT_TICKS;
            continue;
        }
        (void)bb_ring_pop(&modem->task_queues[queue_index], &event);
        if (!task_account_stack(modem, &event)) {
            return -3;
        }
        modem->now = event.tick;
        bb_trace(&modem->cfg, modem->now, "EVENT", (int32_t)event.type);
#if defined(__arm__)
        {
            const uint32_t task_index = task_queue_index(event.type);
            void *const stack_top =
                &modem->task_stacks[task_index].bytes[
                    modem->task_info[task_index].budget_bytes];
            bb_arch_run_task(task_dispatch_body, modem, &event, stack_top);
        }
#elif defined(BB_HOST_TASK_STACK_SWITCH)
        {
            const uint32_t task_index = task_queue_index(event.type);
            void *const stack_top =
                &modem->task_stacks[task_index].bytes[
                    modem->task_info[task_index].budget_bytes];
            bb_arch_run_task_host(task_dispatch_body, modem, &event,
                                  stack_top);
        }
#else
        task_dispatch_body(modem, &event);
#endif
        if (!task_measure_stack(modem, &event)) {
            return -3;
        }
    }
    modem->now = tick;
    return 0;
}

void bb_modem_get_stats(const struct bb_modem *modem, struct bb_stats *out)
{
    if ((modem != NULL) && (out != NULL)) {
        *out = modem->stats;
    }
}

int bb_modem_tx_acquire(struct bb_modem *modem, struct bb_iq_block *block,
                        struct bb_buf_handle *handle)
{
    uint32_t slot_index;
    if ((modem == NULL) || (block == NULL) || (handle == NULL)) {
        return -1;
    }
    for (slot_index = 0U; slot_index < BB_DMA_SLOTS; ++slot_index) {
        struct bb_dma_slot *const slot = &modem->dma[slot_index];
        if ((slot->state == (uint8_t)BB_DMA_DMA_OWNED) &&
            (slot->direction == (uint8_t)BB_DMA_TX)) {
            modem->plat.cache_invalidate(modem->plat_ctx, slot->descriptor,
                                         BB_DMA_DESCRIPTOR_SIZE);
            atomic_thread_fence(memory_order_acquire);
            if (dma_validate_descriptor(slot, slot_index, true) != 0) {
                modem->stats.dropped++;
                bb_trace(&modem->cfg, modem->now, "DMA_DESCRIPTOR_DROP",
                         (int32_t)slot_index);
                dma_release(slot);
                continue;
            }
            block->t0 = slot->t0;
            block->sample_rate_hz = slot->sample_rate_hz;
            block->count = slot->count;
            block->samples = slot->samples;
            handle->slot = (uint16_t)slot_index;
            handle->generation = slot->generation;
            handle->length = slot->count * 4U;
            return 0;
        }
    }
    return -2;
}

int bb_modem_tx_complete(struct bb_modem *modem, struct bb_buf_handle handle)
{
    struct bb_dma_slot *slot;
    if ((modem == NULL) || ((uint32_t)handle.slot >= BB_DMA_SLOTS)) {
        return -1;
    }
    slot = &modem->dma[handle.slot];
    if ((slot->generation != handle.generation) ||
        (slot->state != (uint8_t)BB_DMA_DMA_OWNED) ||
        (slot->direction != (uint8_t)BB_DMA_TX) ||
        (handle.length != slot->count * 4U)) {
        return -2;
    }
    modem->plat.cache_invalidate(modem->plat_ctx, slot->descriptor,
                                 BB_DMA_DESCRIPTOR_SIZE);
    atomic_thread_fence(memory_order_release);
    if (dma_device_complete(slot, handle.slot) != 0) {
        modem->stats.dropped++;
        dma_release(slot);
        return -3;
    }
    slot->state = (uint8_t)BB_DMA_DONE;
    bb_trace(&modem->cfg, modem->now, "DMA_TX_DONE", (int32_t)handle.slot);
    dma_release(slot);
    return 0;
}

int bb_modem_read_tb(struct bb_modem *modem, uint8_t *tb, size_t capacity,
                     size_t *length)
{
    uint32_t index;
    if ((modem == NULL) || (tb == NULL) || (length == NULL)) {
        return -1;
    }
    if (!modem->last_tb_ready) {
        return -2;
    }
    if (capacity < modem->last_tb_length) {
        return -3;
    }
    for (index = 0U; index < modem->last_tb_length; ++index) {
        tb[index] = modem->last_tb[index];
    }
    *length = modem->last_tb_length;
    modem->last_tb_ready = false;
    return 0;
}

void bb_modem_get_session_status(const struct bb_modem *modem,
                                 struct bb_session_status *status)
{
    if ((modem != NULL) && (status != NULL)) {
        status->rrc = modem->stack.rrc;
        status->nas = modem->stack.nas;
        status->delivered = modem->stack.delivered;
        status->replay_drops = modem->stack.replay_drops;
        status->harq_retx = modem->stack.harq_retx;
        status->harq_combines = modem->stack.harq_combines;
        status->rlc_reorders = modem->stack.rlc_reorders;
        status->pdcp_reorders = modem->stack.pdcp_reorders;
        status->recoveries = modem->stack.recoveries;
        status->security_active = modem->stack.security_active;
    }
}

int bb_modem_copy_delivered_sdu(const struct bb_modem *modem, uint8_t *output,
                                size_t capacity, size_t *length)
{
    uint32_t index;
    if ((modem == NULL) || (output == NULL) || (length == NULL) ||
        (capacity < BB_MAX_SDU_BYTES)) {
        return -1;
    }
    if (modem->stack.delivered != BB_MAX_SDU_BYTES) {
        return -2;
    }
    for (index = 0U; index < BB_MAX_SDU_BYTES; ++index) {
        if (modem->stack.received_map[index] == 0U) {
            return -2;
        }
        output[index] = modem->stack.reassembly[index];
    }
    *length = BB_MAX_SDU_BYTES;
    return 0;
}

int bb_modem_restart_domain(struct bb_modem *modem, uint16_t domain,
                            uint64_t tick)
{
    uint32_t index;
    if ((modem == NULL) || bb_tick_before(tick, modem->now) ||
        ((domain != 1U) && (domain != 2U))) {
        return -1;
    }
    if ((tick != modem->now) && (bb_modem_run_until(modem, tick) != 0)) {
        return -2;
    }
    if (domain == 1U) {
        for (index = 0U; index <= 1U; ++index) {
            struct bb_event queued;
            while (bb_ring_pop(&modem->task_queues[index], &queued) == 0) {
                if ((queued.type == (uint16_t)BB_EVENT_TX_REQUEST) &&
                    (bb_pool_release(&modem->pool, queued.payload) != 0)) {
                    modem->stats.dropped++;
                }
            }
        }
        for (index = 0U; index < BB_DMA_SLOTS; ++index) {
            dma_release(&modem->dma[index]);
        }
        modem->pending_rx_valid = false;
        bb_trace(&modem->cfg, tick, "DOMAIN_RESTART_PHY", 1);
    } else {
        /* Drop queued L2/control work and release every referenced buffer
         * before resetting sequence spaces. */
        for (index = 2U; index <= 6U; ++index) {
            struct bb_event queued;
            while (bb_ring_pop(&modem->task_queues[index], &queued) == 0) {
                if ((queued.type !=
                     (uint16_t)BB_EVENT_CRYPTO_COMPLETE) &&
                    (bb_pool_release(&modem->pool, queued.payload) != 0)) {
                    modem->stats.dropped++;
                }
            }
        }
        for (index = 0U; index < BB_HARQ_PROCESSES; ++index) {
            modem->stack.harq[index].state = (uint8_t)BB_HARQ_IDLE;
            modem->stack.harq[index].timer_generation++;
            modem->stack.harq[index].soft_length = 0U;
        }
        modem->stack.active_harq_valid = false;
        modem->stack.rlc_rx_next = 0U;
        modem->stack.rlc_bitmap = 0U;
        modem->stack.rlc_reorder_active = false;
        modem->stack.rlc_timer_generation++;
        modem->stack.pdcp_rx_next = 0U;
        modem->stack.pdcp_bitmap = 0U;
        modem->stack.pdcp_initialized = false;
        modem->stack.pdcp_reorder_active = false;
        modem->stack.pdcp_timer_generation++;
        for (index = 0U; index < BB_REORDER_WINDOW; ++index) {
            modem->stack.rlc_slots[index].valid = false;
            modem->stack.pdcp_slots[index].valid = false;
        }
        modem->stack.bsr_bytes = 0U;
        bb_crypto_reset(modem);
        bb_trace(&modem->cfg, tick, "DOMAIN_RESTART_L2", 2);
    }
    modem->stack.recoveries++;
    modem->now = tick;
    return 0;
}

int bb_modem_get_task_stack_status(const struct bb_modem *modem,
                                   uint32_t task_index,
                                   struct bb_task_stack_status *status)
{
    if ((modem == NULL) || (status == NULL) ||
        (task_index >= BB_SCHEDULED_TASKS)) {
        return -1;
    }
    status->budget_bytes = modem->task_info[task_index].budget_bytes;
    status->high_water_bytes = modem->task_info[task_index].high_water_bytes;
    status->guards_ok =
        (modem->task_info[task_index].guard_low == BB_STACK_CANARY) &&
        (modem->task_info[task_index].guard_high == BB_STACK_CANARY) &&
        (modem->task_stacks[task_index].guard_low == BB_STACK_CANARY) &&
        (modem->task_stacks[task_index].guard_high == BB_STACK_CANARY);
    return 0;
}
