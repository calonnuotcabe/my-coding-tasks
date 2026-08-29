#include "internal.h"

uint32_t bb_pdcp_reconstruct_count(const struct bb_stack_context *stack,
                                   uint32_t sequence_number)
{
    uint32_t hfn;
    uint32_t expected_sn;
    const uint32_t sn = sequence_number & BB_PDCP_SN_MASK;
    const uint32_t half_window = (BB_PDCP_SN_MASK + 1U) / 2U;
    if ((stack == NULL) || !stack->pdcp_initialized) {
        return sn;
    }
    hfn = stack->pdcp_rx_next >> BB_PDCP_SN_BITS;
    expected_sn = stack->pdcp_rx_next & BB_PDCP_SN_MASK;
    if ((sn < expected_sn) && ((expected_sn - sn) > half_window)) {
        hfn = (hfn + 1U) & 0x3FFFU;
    } else if ((sn > expected_sn) && ((sn - expected_sn) > half_window)) {
        hfn = (hfn - 1U) & 0x3FFFU;
    }
    return (hfn << BB_PDCP_SN_BITS) | sn;
}

static void pdcp_replay_drop(struct bb_modem *modem, uint32_t count,
                             uint64_t tick)
{
    modem->stack.replay_drops++;
    modem->stats.dropped++;
    bb_trace(&modem->cfg, tick, "PDCP_REPLAY_DROP", (int32_t)count);
}

static bool pdcp_fragment_valid(struct bb_modem *modem,
                                const struct bb_wire_pdu *pdu,
                                uint64_t tick)
{
    uint32_t index;
    uint32_t slot_index;
    if ((pdu->type != (uint8_t)BB_PDU_DATA) ||
        (pdu->payload_length == 0U) ||
        ((uint32_t)pdu->offset + pdu->payload_length > pdu->total_length) ||
        (pdu->total_length != BB_MAX_SDU_BYTES)) {
        bb_trace(&modem->cfg, tick, "PDCP_LENGTH_DROP", pdu->payload_length);
        modem->stats.dropped++;
        return false;
    }
    for (index = 0U; index < pdu->payload_length; ++index) {
        const uint32_t destination = (uint32_t)pdu->offset + index;
        if ((modem->stack.received_map[destination] != 0U) &&
            (modem->stack.reassembly[destination] != pdu->payload[index])) {
            bb_trace(&modem->cfg, tick, "PDCP_OVERLAP_DROP",
                     (int32_t)destination);
            modem->stats.dropped++;
            return false;
        }
    }
    /* Detect conflicts between two not-yet-delivered reorder entries too. */
    for (slot_index = 0U; slot_index < BB_REORDER_WINDOW; ++slot_index) {
        const struct bb_pdcp_reorder_slot *const slot =
            &modem->stack.pdcp_slots[slot_index];
        const uint32_t left = (pdu->offset > slot->offset) ?
                              pdu->offset : slot->offset;
        const uint32_t pdu_end = (uint32_t)pdu->offset + pdu->payload_length;
        const uint32_t slot_end = (uint32_t)slot->offset + slot->payload_length;
        const uint32_t right = (pdu_end < slot_end) ? pdu_end : slot_end;
        uint32_t position;
        if (!slot->valid || (left >= right)) {
            continue;
        }
        for (position = left; position < right; ++position) {
            if (pdu->payload[position - pdu->offset] !=
                slot->payload[position - slot->offset]) {
                bb_trace(&modem->cfg, tick, "PDCP_OVERLAP_DROP",
                         (int32_t)position);
                modem->stats.dropped++;
                return false;
            }
        }
    }
    return true;
}

static void pdcp_deliver(struct bb_modem *modem, uint8_t harq_id,
                         uint32_t count, uint16_t offset,
                         const uint8_t *payload, uint16_t payload_length,
                         uint64_t tick)
{
    uint32_t index;
    for (index = 0U; index < payload_length; ++index) {
        const uint32_t destination = (uint32_t)offset + index;
        modem->stack.reassembly[destination] = payload[index];
        if (modem->stack.received_map[destination] == 0U) {
            modem->stack.received_map[destination] = 1U;
            modem->stack.delivered++;
        }
    }
    bb_trace(&modem->cfg, tick, "PDCP_DELIVER", (int32_t)modem->stack.delivered);
    (void)bb_mac_send_control(modem, (uint8_t)BB_PDU_DATA_ACK, harq_id,
                              count, tick);
}

static void pdcp_advance(struct bb_stack_context *stack)
{
    stack->pdcp_rx_next++;
    stack->pdcp_bitmap >>= 1U;
}

static void pdcp_release_contiguous(struct bb_modem *modem, uint64_t tick)
{
    uint32_t work = 0U;
    while (((modem->stack.pdcp_bitmap & UINT64_C(1)) != 0U) &&
           (work < BB_REORDER_WINDOW)) {
        const uint32_t count = modem->stack.pdcp_rx_next;
        struct bb_pdcp_reorder_slot *const slot =
            &modem->stack.pdcp_slots[count & (BB_REORDER_WINDOW - 1U)];
        if (!slot->valid || (slot->count != count)) {
            slot->valid = false;
            modem->stats.dropped++;
            bb_trace(&modem->cfg, tick, "PDCP_REORDER_CORRUPT",
                     (int32_t)count);
        } else {
            pdcp_deliver(modem, slot->harq_id, slot->count, slot->offset,
                         slot->payload, slot->payload_length, tick);
            slot->valid = false;
            bb_trace(&modem->cfg, tick, "PDCP_REORDER_RELEASE",
                     (int32_t)count);
        }
        pdcp_advance(&modem->stack);
        work++;
    }
    if (modem->stack.pdcp_bitmap == 0U) {
        modem->stack.pdcp_reorder_active = false;
        modem->stack.pdcp_timer_generation++;
    }
}

int bb_pdcp_decode_and_process(struct bb_modem *modem, const uint8_t *data,
                               size_t length, uint64_t tick)
{
    struct bb_wire_pdu pdu;
    uint32_t budget = 1000U;
    uint32_t delta;
    /* Integrity was completed by the MAC crypto continuation before this
     * owned message was published to RLC/PDCP. */
    const int result = bb_protocol_decode(data, length, &pdu, false, &budget);
    if (result != 0) {
        bb_trace(&modem->cfg, tick, "INTEGRITY_DROP", result);
        modem->stats.dropped++;
        return result;
    }
    pdu.pdcp_count = bb_pdcp_reconstruct_count(&modem->stack,
                                                pdu.pdcp_count);
    bb_trace(&modem->cfg, tick, "PDCP_COUNT_SN",
             (int32_t)(pdu.pdcp_count & BB_PDCP_SN_MASK));
    bb_trace(&modem->cfg, tick, "PDCP_COUNT_HFN",
             (int32_t)(pdu.pdcp_count >> BB_PDCP_SN_BITS));
    if ((modem->stack.nas != BB_NAS_SESSION_ACTIVE) ||
        !modem->stack.security_active) {
        bb_trace(&modem->cfg, tick, "DATA_WRONG_STATE", (int32_t)pdu.type);
        return -1;
    }
    if (!pdcp_fragment_valid(modem, &pdu, tick)) {
        return -3;
    }
    if (!modem->stack.pdcp_initialized) {
        modem->stack.pdcp_initialized = true;
        modem->stack.pdcp_rx_next = pdu.pdcp_count;
    }
    delta = pdu.pdcp_count - modem->stack.pdcp_rx_next;
    if ((delta & UINT32_C(0x80000000)) != 0U) {
        pdcp_replay_drop(modem, pdu.pdcp_count, tick);
        return -2;
    }
    if (delta >= BB_REORDER_WINDOW) {
        modem->stats.dropped++;
        bb_trace(&modem->cfg, tick, "PDCP_DROP_WINDOW",
                 (int32_t)pdu.pdcp_count);
        return -2;
    }
    if (delta != 0U) {
        struct bb_pdcp_reorder_slot *const slot =
            &modem->stack.pdcp_slots[pdu.pdcp_count &
                                     (BB_REORDER_WINDOW - 1U)];
        uint32_t index;
        if (((modem->stack.pdcp_bitmap & (UINT64_C(1) << delta)) != 0U) ||
            slot->valid) {
            pdcp_replay_drop(modem, pdu.pdcp_count, tick);
            return -2;
        }
        slot->valid = true;
        slot->harq_id = pdu.harq_id;
        slot->offset = pdu.offset;
        slot->total_length = pdu.total_length;
        slot->payload_length = pdu.payload_length;
        slot->count = pdu.pdcp_count;
        for (index = 0U; index < pdu.payload_length; ++index) {
            slot->payload[index] = pdu.payload[index];
        }
        modem->stack.pdcp_bitmap |= UINT64_C(1) << delta;
        modem->stack.pdcp_reorders++;
        if (!modem->stack.pdcp_reorder_active) {
            modem->stack.pdcp_reorder_active = true;
            modem->stack.pdcp_reorder_expiry =
                tick + BB_REORDER_TIMEOUT_TICKS;
            modem->stack.pdcp_timer_generation++;
        }
        bb_trace(&modem->cfg, tick, "PDCP_REORDER_BUFFER",
                 (int32_t)pdu.pdcp_count);
        (void)bb_mac_send_control(modem, (uint8_t)BB_PDU_DATA_ACK,
                                  pdu.harq_id, pdu.pdcp_count, tick);
        return 0;
    }
    pdcp_deliver(modem, pdu.harq_id, pdu.pdcp_count, pdu.offset,
                 pdu.payload, pdu.payload_length, tick);
    pdcp_advance(&modem->stack);
    pdcp_release_contiguous(modem, tick);
    return 0;
}

void bb_pdcp_check_timer(struct bb_modem *modem, uint64_t tick)
{
    uint32_t skip = 0U;
    if ((modem == NULL) || !modem->stack.pdcp_reorder_active ||
        !bb_tick_due(tick, modem->stack.pdcp_reorder_expiry)) {
        return;
    }
    if (tick != modem->stack.pdcp_reorder_expiry) {
        modem->stats.deadline_miss++;
    }
    while ((skip < BB_REORDER_WINDOW) &&
           ((modem->stack.pdcp_bitmap & (UINT64_C(1) << skip)) == 0U)) {
        skip++;
    }
    if (skip == BB_REORDER_WINDOW) {
        modem->stack.pdcp_reorder_active = false;
        modem->stack.pdcp_timer_generation++;
        return;
    }
    if (skip != 0U) {
        bb_trace(&modem->cfg, tick, "PDCP_REORDER_TIMEOUT", (int32_t)skip);
        modem->stack.pdcp_rx_next += skip;
        modem->stack.pdcp_bitmap >>= skip;
        modem->stack.pdcp_timer_generation++;
    }
    pdcp_release_contiguous(modem, tick);
    if (modem->stack.pdcp_bitmap != 0U) {
        modem->stack.pdcp_reorder_active = true;
        modem->stack.pdcp_reorder_expiry = tick + BB_REORDER_TIMEOUT_TICKS;
    }
}
