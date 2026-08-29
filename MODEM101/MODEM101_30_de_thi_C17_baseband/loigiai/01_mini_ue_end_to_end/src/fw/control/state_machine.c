#include "internal.h"

static void rrc_transition(struct bb_modem *modem, enum bb_rrc_state from,
                           uint8_t event, bool guard,
                           enum bb_control_action action,
                           enum bb_rrc_state to, const char *trace,
                           uint64_t tick)
{
    if ((modem->stack.rrc == from) &&
        bb_control_rrc_allowed(from, event, guard, action, to)) {
        modem->stack.rrc = to;
        bb_trace(&modem->cfg, tick, trace, (int32_t)to);
    }
}

static void nas_transition(struct bb_modem *modem, enum bb_nas_state from,
                           uint8_t event, bool guard,
                           enum bb_control_action action,
                           enum bb_nas_state to, const char *trace,
                           uint64_t tick)
{
    if ((modem->stack.nas == from) &&
        bb_control_nas_allowed(from, event, guard, action, to)) {
        modem->stack.nas = to;
        bb_trace(&modem->cfg, tick, trace, (int32_t)to);
    }
}

int bb_control_process(struct bb_modem *modem,
                       const struct bb_wire_pdu *pdu, uint64_t tick)
{
    switch (pdu->type) {
    case BB_PDU_CELL_BEACON:
        if ((pdu->payload_length != 5U) || (pdu->payload[0] != 1U) ||
            (pdu->payload[1] != 2U) || (pdu->payload[2] != 0U) ||
            (((uint16_t)pdu->payload[3] |
              ((uint16_t)pdu->payload[4] << 8U)) != modem->cfg.pci)) {
            break;
        }
        bb_trace(&modem->cfg, tick, "CELL_PCI", modem->cfg.pci);
        if (modem->stack.rrc == BB_RRC_OFF) {
            rrc_transition(modem, BB_RRC_OFF, pdu->type, true,
                           BB_ACTION_CELL_SEARCH, BB_RRC_SEARCHING,
                           "RRC_OFF_SEARCHING", tick);
            return 0;
        } else if (modem->stack.rrc == BB_RRC_SEARCHING) {
            rrc_transition(modem, BB_RRC_SEARCHING, pdu->type, true,
                           BB_ACTION_CELL_CAMP, BB_RRC_CAMPED,
                           "RRC_SEARCHING_CAMPED", tick);
            return 0;
        }
        break;
    case BB_PDU_RRC_SETUP:
        if (modem->stack.rrc == BB_RRC_CAMPED) {
            rrc_transition(modem, BB_RRC_CAMPED, pdu->type, true,
                           BB_ACTION_RRC_CONNECT_START, BB_RRC_CONNECTING,
                           "RRC_CAMPED_CONNECTING", tick);
            return 0;
        } else if (modem->stack.rrc == BB_RRC_CONNECTING) {
            rrc_transition(modem, BB_RRC_CONNECTING, pdu->type, true,
                           BB_ACTION_RRC_CONNECT_COMPLETE, BB_RRC_CONNECTED,
                           "RRC_CONNECTING_CONNECTED", tick);
            if (modem->stack.recovering) {
                modem->stack.recovering = false;
                modem->stack.recoveries++;
                bb_trace(&modem->cfg, tick, "RRC_RECOVERY_COMPLETE",
                         (int32_t)modem->stack.recoveries);
            }
            (void)bb_mac_send_control(modem, (uint8_t)BB_PDU_RRC_COMPLETE,
                                      0U, 0U, tick);
            if (modem->stack.nas == BB_NAS_DEREGISTERED) {
                nas_transition(modem, BB_NAS_DEREGISTERED, pdu->type, true,
                               BB_ACTION_NAS_REGISTRATION_START,
                               BB_NAS_REGISTERING,
                               "NAS_DEREGISTERED_REGISTERING", tick);
                (void)bb_mac_send_control(modem,
                                          (uint8_t)BB_PDU_REG_REQUEST,
                                          0U, 0U, tick);
            }
            return 0;
        }
        break;
    case BB_PDU_SECURITY_MODE:
        if ((modem->stack.rrc != BB_RRC_CONNECTED) ||
            (modem->stack.nas != BB_NAS_REGISTERING)) break;
        modem->stack.key_slot = 1U;
        modem->stack.security_active = true;
        bb_trace(&modem->cfg, tick, "SECURITY_ACTIVE", 1);
        (void)bb_mac_send_control(modem, (uint8_t)BB_PDU_SECURITY_COMPLETE,
                                  0U, 0U, tick);
        return 0;
    case BB_PDU_REG_ACCEPT:
        if (!modem->stack.security_active ||
            (modem->stack.nas != BB_NAS_REGISTERING)) break;
        nas_transition(modem, BB_NAS_REGISTERING, pdu->type,
                       modem->stack.security_active,
                       BB_ACTION_NAS_REGISTRATION_ACCEPT,
                       BB_NAS_REGISTERED,
                       "NAS_REGISTERING_REGISTERED", tick);
        return 0;
    case BB_PDU_SESSION_ACCEPT:
        if (modem->stack.nas != BB_NAS_REGISTERED) break;
        nas_transition(modem, BB_NAS_REGISTERED, pdu->type, true,
                       BB_ACTION_NAS_SESSION_ACTIVATE,
                       BB_NAS_SESSION_ACTIVE,
                       "NAS_REGISTERED_SESSION_ACTIVE", tick);
        return 0;
    case BB_PDU_DATA_GRANT:
        if (modem->stack.nas == BB_NAS_SESSION_ACTIVE) {
            struct bb_harq_process *const process =
                &modem->stack.harq[pdu->harq_id];
            const uint32_t full_count =
                bb_pdcp_reconstruct_count(&modem->stack, pdu->pdcp_count);
            process->state = (uint8_t)BB_HARQ_WAITING;
            process->pdcp_count = full_count;
            process->expiry = tick + UINT64_C(1000);
            process->timer_generation++;
            process->transmissions = 0U;
            process->soft_length = 0U;
            modem->stack.active_harq_id = pdu->harq_id;
            modem->stack.active_harq_valid = true;
            bb_trace(&modem->cfg, tick, "HARQ_WAIT", pdu->harq_id);
            return 0;
        }
        break;
    case BB_PDU_LINK_LOSS:
        if (modem->stack.rrc == BB_RRC_CONNECTED) {
            rrc_transition(modem, BB_RRC_CONNECTED, pdu->type, true,
                           BB_ACTION_LINK_RECOVERY_START,
                           BB_RRC_SEARCHING,
                           "RRC_CONNECTED_SEARCHING_LINK_LOSS", tick);
            modem->stack.recovering = true;
            return 0;
        }
        break;
    default:
        break;
    }
    bb_trace(&modem->cfg, tick, "STATE_EVENT_REJECT", pdu->type);
    return -1;
}
