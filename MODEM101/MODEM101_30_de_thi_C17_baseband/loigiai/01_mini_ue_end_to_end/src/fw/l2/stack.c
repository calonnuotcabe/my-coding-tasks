#include "internal.h"

static void clear_bytes(uint8_t *data, size_t length)
{
    size_t index;
    for (index = 0U; index < length; ++index) data[index] = 0U;
}

void bb_stack_init(struct bb_stack_context *stack)
{
    uint32_t process_index;
    uint32_t reorder_index;
    if (stack == NULL) return;
    stack->rrc = BB_RRC_OFF;
    stack->nas = BB_NAS_DEREGISTERED;
    stack->security_active = false;
    stack->recovering = false;
    stack->key_slot = 0U;
    stack->rlc_rx_next = 0U;
    stack->rlc_bitmap = 0U;
    stack->rlc_reorder_expiry = 0U;
    stack->rlc_timer_generation = 1U;
    stack->rlc_reorder_active = false;
    stack->pdcp_rx_next = 0U;
    stack->pdcp_bitmap = 0U;
    stack->pdcp_reorder_expiry = 0U;
    stack->pdcp_timer_generation = 1U;
    stack->pdcp_initialized = false;
    stack->pdcp_reorder_active = false;
    stack->delivered = 0U;
    stack->replay_drops = 0U;
    stack->harq_retx = 0U;
    stack->harq_combines = 0U;
    stack->rlc_reorders = 0U;
    stack->pdcp_reorders = 0U;
    stack->recoveries = 0U;
    stack->bsr_bytes = 0U;
    stack->active_harq_id = 0U;
    stack->active_harq_valid = false;
    clear_bytes(stack->reassembly, sizeof(stack->reassembly));
    clear_bytes(stack->received_map, sizeof(stack->received_map));
    for (reorder_index = 0U; reorder_index < BB_REORDER_WINDOW;
         ++reorder_index) {
        stack->rlc_slots[reorder_index].valid = false;
        stack->pdcp_slots[reorder_index].valid = false;
    }
    for (process_index = 0U; process_index < BB_HARQ_PROCESSES;
         ++process_index) {
        stack->harq[process_index].state = (uint8_t)BB_HARQ_IDLE;
        stack->harq[process_index].transmissions = 0U;
        stack->harq[process_index].timer_generation = 1U;
        stack->harq[process_index].expiry = 0U;
        stack->harq[process_index].pdcp_count = 0U;
        stack->harq[process_index].soft_length = 0U;
    }
}

int bb_stack_process_pdu(struct bb_modem *modem, const uint8_t *data,
                         size_t length, uint64_t tick)
{
    int stage = (int)BB_EVENT_MAC_RX;
    uint32_t stages = 0U;
    if ((modem == NULL) || (data == NULL) || (length > 65535U)) return -1;
    while ((stage > 0) && (stages < 4U)) {
        stage = bb_stack_process_stage(modem, data, length, tick,
                                       (uint16_t)stage);
        stages++;
    }
    bb_rlc_drain(modem, tick);
    return (stage == 0) ? 0 : stage;
}

int bb_stack_process_stage(struct bb_modem *modem, const uint8_t *data,
                           size_t length, uint64_t tick, uint16_t stage)
{
    struct bb_wire_pdu pdu;
    uint32_t budget = 1000U;
    int result;
    if ((modem == NULL) || (data == NULL) || (length > 65535U)) return -1;
    if (stage == (uint16_t)BB_EVENT_MAC_RX) {
        return bb_mac_process_tb(modem, data, length, tick);
    }
    /* Only MAC publishes child stage messages; protected PDUs have already
     * passed the descriptor-backed crypto completion continuation. */
    result = bb_protocol_decode(data, length, &pdu, false, &budget);
    if (result != 0) {
        bb_trace(&modem->cfg, tick, "PDU_MALFORMED_DROP", result);
        modem->stats.dropped++;
        return 0;
    }
    if (stage == (uint16_t)BB_EVENT_RLC_RX) {
        return bb_rlc_process(modem, data, length, pdu.rlc_sn, tick);
    }
    if (stage == (uint16_t)BB_EVENT_PDCP_RX) {
        (void)bb_pdcp_decode_and_process(modem, data, length, tick);
        return 0;
    }
    if (stage == (uint16_t)BB_EVENT_CONTROL_RX) {
        if (bb_protocol_validate_ies(pdu.payload, pdu.payload_length,
                                     &budget) != 0) {
            bb_trace(&modem->cfg, tick, "IE_PARSE_DROP", pdu.type);
            modem->stats.dropped++;
            return 0;
        }
        if ((pdu.type == (uint8_t)BB_PDU_REG_ACCEPT) ||
            (pdu.type == (uint8_t)BB_PDU_SESSION_ACCEPT)) {
            return (int)BB_EVENT_NAS_RX;
        }
        (void)bb_control_process(modem, &pdu, tick);
        return 0;
    }
    if (stage == (uint16_t)BB_EVENT_NAS_RX) {
        (void)bb_control_process(modem, &pdu, tick);
        return 0;
    }
    modem->stats.dropped++;
    return 0;
}

void bb_stack_check_timers(struct bb_modem *modem, uint64_t tick)
{
    uint32_t process_index;
    if (modem == NULL) return;
    for (process_index = 0U; process_index < BB_HARQ_PROCESSES;
         ++process_index) {
        struct bb_harq_process *const process =
            &modem->stack.harq[process_index];
        if ((process->state == (uint8_t)BB_HARQ_WAITING) &&
            bb_tick_due(tick, process->expiry)) {
            if (tick != process->expiry) {
                modem->stats.deadline_miss++;
                bb_trace(&modem->cfg, tick, "TIMER_LATE",
                         (int32_t)(tick - process->expiry));
            }
            if (process->transmissions >= BB_HARQ_MAX_TRANSMISSIONS) {
                process->state = (uint8_t)BB_HARQ_IDLE;
                process->soft_length = 0U;
                process->timer_generation++;
                if (modem->stack.active_harq_valid &&
                    (modem->stack.active_harq_id == process_index)) {
                    modem->stack.active_harq_valid = false;
                }
                modem->stats.dropped++;
                bb_trace(&modem->cfg, tick, "HARQ_TIMEOUT",
                         (int32_t)process_index);
            } else {
                process->transmissions++;
                process->expiry = tick + 1000U;
                process->timer_generation++;
                bb_trace(&modem->cfg, tick, "HARQ_NACK",
                         (int32_t)process_index);
                (void)bb_mac_send_control(modem, (uint8_t)BB_PDU_NACK,
                                          (uint8_t)process_index,
                                          process->pdcp_count, tick);
            }
        }
    }
    bb_rlc_check_timer(modem, tick);
    bb_pdcp_check_timer(modem, tick);
}
