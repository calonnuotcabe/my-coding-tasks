#include "internal.h"

struct rrc_transition_spec {
    enum bb_rrc_state from;
    uint8_t event;
    enum bb_control_action action;
    enum bb_rrc_state to;
};

struct nas_transition_spec {
    enum bb_nas_state from;
    uint8_t event;
    enum bb_control_action action;
    enum bb_nas_state to;
};

static const struct rrc_transition_spec rrc_transitions[] = {
    { BB_RRC_OFF, (uint8_t)BB_PDU_CELL_BEACON,
      BB_ACTION_CELL_SEARCH, BB_RRC_SEARCHING },
    { BB_RRC_SEARCHING, (uint8_t)BB_PDU_CELL_BEACON,
      BB_ACTION_CELL_CAMP, BB_RRC_CAMPED },
    { BB_RRC_CAMPED, (uint8_t)BB_PDU_RRC_SETUP,
      BB_ACTION_RRC_CONNECT_START, BB_RRC_CONNECTING },
    { BB_RRC_CONNECTING, (uint8_t)BB_PDU_RRC_SETUP,
      BB_ACTION_RRC_CONNECT_COMPLETE, BB_RRC_CONNECTED },
    { BB_RRC_CONNECTED, (uint8_t)BB_PDU_LINK_LOSS,
      BB_ACTION_LINK_RECOVERY_START, BB_RRC_SEARCHING }
};

static const struct nas_transition_spec nas_transitions[] = {
    { BB_NAS_DEREGISTERED, (uint8_t)BB_PDU_RRC_SETUP,
      BB_ACTION_NAS_REGISTRATION_START, BB_NAS_REGISTERING },
    { BB_NAS_REGISTERING, (uint8_t)BB_PDU_REG_ACCEPT,
      BB_ACTION_NAS_REGISTRATION_ACCEPT, BB_NAS_REGISTERED },
    { BB_NAS_REGISTERED, (uint8_t)BB_PDU_SESSION_ACCEPT,
      BB_ACTION_NAS_SESSION_ACTIVATE, BB_NAS_SESSION_ACTIVE }
};

bool bb_control_rrc_allowed(enum bb_rrc_state from, uint8_t event,
                            bool guard, enum bb_control_action action,
                            enum bb_rrc_state to)
{
    size_t index;
    if (!guard) {
        return false;
    }
    for (index = 0U; index < sizeof(rrc_transitions) / sizeof(rrc_transitions[0]);
         ++index) {
        if ((rrc_transitions[index].from == from) &&
            (rrc_transitions[index].event == event) &&
            (rrc_transitions[index].action == action) &&
            (rrc_transitions[index].to == to)) {
            return true;
        }
    }
    return false;
}

bool bb_control_nas_allowed(enum bb_nas_state from, uint8_t event,
                            bool guard, enum bb_control_action action,
                            enum bb_nas_state to)
{
    size_t index;
    if (!guard) {
        return false;
    }
    for (index = 0U; index < sizeof(nas_transitions) / sizeof(nas_transitions[0]);
         ++index) {
        if ((nas_transitions[index].from == from) &&
            (nas_transitions[index].event == event) &&
            (nas_transitions[index].action == action) &&
            (nas_transitions[index].to == to)) {
            return true;
        }
    }
    return false;
}
