#ifndef NETWORK_PEER_H
#define NETWORK_PEER_H

#include "bb/bb.h"
#include "bb/mac.h"
#include "bb/protocol.h"

struct network_peer {
    struct bb_config channel_config;
    struct bb_cpx16 downlink_tx[BB_MAX_IQ_SAMPLES];
    struct bb_cpx16 downlink_rx[BB_MAX_IQ_SAMPLES];
    struct bb_cpx16 uplink_rx[BB_MAX_IQ_SAMPLES];
    struct bb_cpx16 last_uplink[BB_MAX_IQ_SAMPLES];
    uint32_t last_uplink_count;
    bool rrc_complete;
    bool registration_request;
    bool security_complete;
    bool security_active;
    bool nack_received;
    uint8_t nack_harq;
    uint32_t nack_count;
    uint32_t data_acks;
    uint32_t uplink_rx_next;
    bool uplink_count_initialized;
    uint32_t downlink_unique_bytes;
    uint8_t downlink_map[BB_MAX_SDU_BYTES];
    uint64_t pending_downlink_tick;
    uint32_t pending_downlink_count;
    bool pending_downlink;
    bool pending_acquired;
};

void network_peer_init(struct network_peer *peer,
                       const struct bb_config *config);
const struct bb_rf_backend *network_peer_rf_backend(void);
int network_peer_send(struct network_peer *peer, struct bb_soc *soc,
                      struct bb_modem *modem,
                      const struct bb_wire_pdu *pdu, uint64_t tick,
                      bool corrupt_codeword);
int network_peer_send_raw(struct network_peer *peer, struct bb_soc *soc,
                          struct bb_modem *modem,
                          const uint8_t *transport_block, size_t length,
                          uint64_t tick);
int network_peer_submit_iq(struct network_peer *peer, struct bb_soc *soc,
                           struct bb_modem *modem,
                           const struct bb_iq_block *block, uint64_t tick);
int network_peer_drain_uplink(struct network_peer *peer,
                              struct bb_soc *soc, struct bb_modem *modem,
                              uint64_t tick);

#endif
