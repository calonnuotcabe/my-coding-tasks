#include "internal.h"

static void rlc_advance(struct bb_stack_context *stack, uint32_t count)
{
    stack->rlc_rx_next =
        (uint16_t)((stack->rlc_rx_next + count) & 0x0FFFU);
    stack->rlc_bitmap >>= count;
}

int bb_rlc_process(struct bb_modem *modem, const uint8_t *data, size_t length,
                   uint16_t sequence, uint64_t tick)
{
    const uint16_t normalized = (uint16_t)(sequence & 0x0FFFU);
    const uint16_t delta =
        (uint16_t)((normalized - modem->stack.rlc_rx_next) & 0x0FFFU);
    struct bb_rlc_reorder_slot *slot;
    size_t index;
    if ((data == NULL) || (length == 0U) || (length > BB_MAX_TB_BYTES)) {
        modem->stats.dropped++;
        return 0;
    }
    if (delta >= BB_REORDER_WINDOW) {
        bb_trace(&modem->cfg, tick, "RLC_DROP_WINDOW", normalized);
        modem->stats.dropped++;
        return 0;
    }
    if (delta == 0U) {
        bb_trace(&modem->cfg, tick, "RLC_UM_IN_ORDER", normalized);
        rlc_advance(&modem->stack, 1U);
        if (modem->stack.rlc_bitmap == 0U) {
            modem->stack.rlc_reorder_active = false;
            modem->stack.rlc_timer_generation++;
        }
        return (int)BB_EVENT_PDCP_RX;
    }
    if ((modem->stack.rlc_bitmap & (UINT64_C(1) << delta)) != 0U) {
        bb_trace(&modem->cfg, tick, "RLC_DROP_DUPLICATE", normalized);
        modem->stats.dropped++;
        return 0;
    }
    slot = &modem->stack.rlc_slots[normalized & (BB_REORDER_WINDOW - 1U)];
    if (slot->valid) {
        bb_trace(&modem->cfg, tick, "RLC_SLOT_COLLISION", normalized);
        modem->stats.dropped++;
        return 0;
    }
    for (index = 0U; index < length; ++index) {
        slot->data[index] = data[index];
    }
    slot->valid = true;
    slot->sequence = normalized;
    slot->length = (uint16_t)length;
    modem->stack.rlc_bitmap |= UINT64_C(1) << delta;
    modem->stack.rlc_reorders++;
    if (!modem->stack.rlc_reorder_active) {
        modem->stack.rlc_reorder_active = true;
        modem->stack.rlc_reorder_expiry = tick + BB_REORDER_TIMEOUT_TICKS;
        modem->stack.rlc_timer_generation++;
    }
    bb_trace(&modem->cfg, tick, "RLC_REORDER_BUFFER", normalized);
    return 0;
}

void bb_rlc_drain(struct bb_modem *modem, uint64_t tick)
{
    uint32_t work = 0U;
    if (modem == NULL) {
        return;
    }
    while (((modem->stack.rlc_bitmap & UINT64_C(1)) != 0U) &&
           (work < BB_REORDER_WINDOW)) {
        const uint16_t sequence = modem->stack.rlc_rx_next;
        struct bb_rlc_reorder_slot *const slot =
            &modem->stack.rlc_slots[sequence & (BB_REORDER_WINDOW - 1U)];
        if (!slot->valid || (slot->sequence != sequence) ||
            (slot->length == 0U) || (slot->length > BB_MAX_TB_BYTES)) {
            slot->valid = false;
            modem->stats.dropped++;
            bb_trace(&modem->cfg, tick, "RLC_REORDER_CORRUPT", sequence);
            rlc_advance(&modem->stack, 1U);
            work++;
            continue;
        }
        if (bb_modem_enqueue_stage_copy(modem, slot->data, slot->length,
                                        tick,
                                        (uint16_t)BB_EVENT_PDCP_RX) != 0) {
            break;
        }
        slot->valid = false;
        bb_trace(&modem->cfg, tick, "RLC_REORDER_RELEASE", sequence);
        rlc_advance(&modem->stack, 1U);
        work++;
    }
    if (modem->stack.rlc_bitmap == 0U) {
        modem->stack.rlc_reorder_active = false;
        modem->stack.rlc_timer_generation++;
    }
}

void bb_rlc_check_timer(struct bb_modem *modem, uint64_t tick)
{
    uint32_t skip = 0U;
    if ((modem == NULL) || !modem->stack.rlc_reorder_active ||
        !bb_tick_due(tick, modem->stack.rlc_reorder_expiry)) {
        return;
    }
    if (tick != modem->stack.rlc_reorder_expiry) {
        modem->stats.deadline_miss++;
    }
    bb_rlc_drain(modem, tick);
    while ((skip < BB_REORDER_WINDOW) &&
           ((modem->stack.rlc_bitmap & (UINT64_C(1) << skip)) == 0U)) {
        skip++;
    }
    if (skip == BB_REORDER_WINDOW) {
        modem->stack.rlc_reorder_active = false;
        modem->stack.rlc_timer_generation++;
        return;
    }
    if (skip != 0U) {
        bb_trace(&modem->cfg, tick, "RLC_REORDER_TIMEOUT", (int32_t)skip);
        rlc_advance(&modem->stack, skip);
        modem->stack.rlc_timer_generation++;
        bb_rlc_drain(modem, tick);
    }
    if (modem->stack.rlc_bitmap != 0U) {
        modem->stack.rlc_reorder_active = true;
        modem->stack.rlc_reorder_expiry = tick + BB_REORDER_TIMEOUT_TICKS;
    }
}
