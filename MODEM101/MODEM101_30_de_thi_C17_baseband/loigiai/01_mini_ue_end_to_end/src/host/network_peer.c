#include "network_peer.h"

#include <stddef.h>

static void copy_samples(struct bb_cpx16 *destination,
                         const struct bb_cpx16 *source, uint32_t count)
{
    uint32_t index;
    for (index = 0U; index < count; ++index) {
        destination[index] = source[index];
    }
}

static int erase_codeword_tail(const struct network_peer *peer,
                               struct bb_iq_block *block)
{
    const uint32_t symbol_samples = BB_NFFT + BB_CP_LEN;
    const uint32_t first_erased =
        peer->channel_config.timing_offset_samples + 3U * symbol_samples;
    uint32_t index;
    if ((peer == NULL) || (block == NULL) || (block->samples == NULL) ||
        (first_erased >= block->count)) {
        return -1;
    }
    /* Preserve the first data symbol so the TB length remains recoverable,
     * then erase the remaining codeword samples.  The demapper assigns zero
     * reliability to these carriers and Chase-combines the clean RETX. */
    for (index = first_erased; index < block->count; ++index) {
        block->samples[index].i = 0;
        block->samples[index].q = 0;
    }
    return 0;
}

static bool protected_type(uint8_t type)
{
    return (type == (uint8_t)BB_PDU_REG_ACCEPT) ||
           (type == (uint8_t)BB_PDU_SESSION_ACCEPT) ||
           (type == (uint8_t)BB_PDU_DATA_GRANT) ||
           (type == (uint8_t)BB_PDU_DATA) ||
           (type == (uint8_t)BB_PDU_LINK_LOSS) ||
           (type == (uint8_t)BB_PDU_SECURITY_COMPLETE) ||
           (type == (uint8_t)BB_PDU_NACK) ||
           (type == (uint8_t)BB_PDU_DATA_ACK);
}

static void peer_handle_pdu(struct network_peer *peer,
                            const uint8_t *encoded, size_t encoded_length);

static void peer_handle_transport_block(struct network_peer *peer,
                                        const uint8_t *data, size_t length)
{
    struct bb_mac_sdu sdus[BB_MAC_MAX_SDUS];
    size_t count = 0U;
    size_t index;
    if (bb_mac_demultiplex(data, length, sdus, BB_MAC_MAX_SDUS, &count) != 0) {
        return;
    }
    for (index = 0U; index < count; ++index) {
        peer_handle_pdu(peer, sdus[index].data, sdus[index].length);
    }
}

static int peer_rf_rx_acquire(void *context, uint64_t deadline,
                              struct bb_iq_block *out)
{
    struct network_peer *const peer = (struct network_peer *)context;
    if ((peer == NULL) || (out == NULL) || !peer->pending_downlink ||
        peer->pending_acquired || (peer->pending_downlink_tick > deadline)) {
        return -1;
    }
    out->t0 = peer->pending_downlink_tick;
    out->sample_rate_hz = peer->channel_config.sample_rate_hz;
    out->count = peer->pending_downlink_count;
    out->samples = peer->downlink_rx;
    peer->pending_acquired = true;
    return 0;
}

static void peer_rf_rx_release(void *context, struct bb_iq_block *block)
{
    struct network_peer *const peer = (struct network_peer *)context;
    if ((peer != NULL) && (block != NULL) && peer->pending_acquired &&
        (block->samples == peer->downlink_rx)) {
        peer->pending_downlink = false;
        peer->pending_acquired = false;
        peer->pending_downlink_count = 0U;
    }
}

static int peer_rf_tx_submit(void *context, const struct bb_iq_block *block)
{
    struct network_peer *const peer = (struct network_peer *)context;
    struct bb_iq_block receive;
    uint8_t decoded[BB_MAX_TB_BYTES];
    size_t decoded_length = 0U;
    if ((peer == NULL) || (block == NULL) || (block->samples == NULL)) {
        return -1;
    }
    receive.t0 = block->t0;
    receive.sample_rate_hz = block->sample_rate_hz;
    receive.count = BB_MAX_IQ_SAMPLES;
    receive.samples = peer->uplink_rx;
    if ((bb_channel_apply(&peer->channel_config, block, &receive) != 0) ||
        (bb_phy_receive(&peer->channel_config, &receive, decoded,
                        sizeof(decoded), &decoded_length) != 0)) {
        return -1;
    }
    peer_handle_transport_block(peer, decoded, decoded_length);
    copy_samples(peer->last_uplink, receive.samples, receive.count);
    peer->last_uplink_count = receive.count;
    return 0;
}

const struct bb_rf_backend *network_peer_rf_backend(void)
{
    static const struct bb_rf_backend backend = {
        peer_rf_tx_submit, peer_rf_rx_acquire, peer_rf_rx_release
    };
    return &backend;
}

void network_peer_init(struct network_peer *peer,
                       const struct bb_config *config)
{
    if ((peer != NULL) && (config != NULL)) {
        peer->channel_config = *config;
        /* Peer-side DSP is not UE firmware telemetry.  Keeping the callbacks
         * separate also prevents uplink receive checkpoints being mislabeled
         * as UE downlink checkpoints. */
        peer->channel_config.trace = NULL;
        peer->channel_config.trace_ctx = NULL;
        peer->last_uplink_count = 0U;
        peer->rrc_complete = false;
        peer->registration_request = false;
        peer->security_complete = false;
        peer->security_active = false;
        peer->nack_received = false;
        peer->nack_harq = 0U;
        peer->nack_count = 0U;
        peer->data_acks = 0U;
        peer->uplink_rx_next = 0U;
        peer->uplink_count_initialized = false;
        peer->downlink_unique_bytes = 0U;
        peer->pending_downlink_tick = 0U;
        peer->pending_downlink_count = 0U;
        peer->pending_downlink = false;
        peer->pending_acquired = false;
        {
            uint32_t index;
            for (index = 0U; index < BB_MAX_SDU_BYTES; ++index) {
                peer->downlink_map[index] = 0U;
            }
        }
    }
}

int network_peer_send(struct network_peer *peer, struct bb_soc *soc,
                      struct bb_modem *modem,
                      const struct bb_wire_pdu *pdu, uint64_t tick,
                      bool corrupt_codeword)
{
    uint8_t encoded[BB_MAX_TB_BYTES];
    uint8_t transport_block[BB_MAX_TB_BYTES];
    struct bb_mac_sdu sdu;
    size_t encoded_length = 0U;
    size_t transport_length = 0U;
    struct bb_iq_block transmit;
    struct bb_iq_block receive;
    uint32_t index;
    bool integrity;
    int result;
    if ((peer == NULL) || (soc == NULL) || (modem == NULL) || (pdu == NULL) ||
        peer->pending_downlink) {
        return -1;
    }
    integrity = (pdu->type == (uint8_t)BB_PDU_DATA) ||
                (peer->security_active && protected_type(pdu->type));
    if (bb_protocol_encode(pdu, encoded, sizeof(encoded), &encoded_length,
                           integrity) != 0) {
        return -2;
    }
    sdu.lcid = (pdu->type == (uint8_t)BB_PDU_DATA) ?
               (uint8_t)BB_MAC_LCID_DATA :
               (uint8_t)BB_MAC_LCID_CONTROL;
    sdu.data = encoded;
    sdu.length = (uint16_t)encoded_length;
    if (bb_mac_multiplex(&sdu, 1U, transport_block,
                         sizeof(transport_block), &transport_length) != 0) {
        return -2;
    }
    transmit.t0 = tick;
    transmit.sample_rate_hz = peer->channel_config.sample_rate_hz;
    transmit.count = BB_MAX_IQ_SAMPLES;
    transmit.samples = peer->downlink_tx;
    if (bb_phy_transmit(&peer->channel_config, transport_block,
                        transport_length,
                        &transmit) != 0) {
        return -3;
    }
    receive.t0 = tick;
    receive.sample_rate_hz = transmit.sample_rate_hz;
    receive.count = BB_MAX_IQ_SAMPLES;
    receive.samples = peer->downlink_rx;
    if (bb_channel_apply(&peer->channel_config, &transmit, &receive) != 0) {
        return -4;
    }
    if (corrupt_codeword && (erase_codeword_tail(peer, &receive) != 0)) {
        return -4;
    }
    peer->pending_downlink_tick = tick;
    peer->pending_downlink_count = receive.count;
    peer->pending_downlink = true;
    result = bb_soc_step(soc, modem, tick);
    if ((result == 0) && (pdu->type == (uint8_t)BB_PDU_DATA) &&
        ((uint32_t)pdu->offset + pdu->payload_length <= BB_MAX_SDU_BYTES)) {
        for (index = 0U; index < pdu->payload_length; ++index) {
            const uint32_t position = (uint32_t)pdu->offset + index;
            if (peer->downlink_map[position] == 0U) {
                peer->downlink_map[position] = 1U;
                peer->downlink_unique_bytes++;
            }
        }
    }
    return result;
}

int network_peer_send_raw(struct network_peer *peer, struct bb_soc *soc,
                          struct bb_modem *modem,
                          const uint8_t *transport_block, size_t length,
                          uint64_t tick)
{
    struct bb_iq_block transmit;
    struct bb_iq_block receive;
    struct bb_mac_sdu sdu;
    uint8_t mac_transport_block[BB_MAX_TB_BYTES];
    size_t mac_length = 0U;
    if ((peer == NULL) || (soc == NULL) || (modem == NULL) ||
        (transport_block == NULL) || (length == 0U) ||
        (length > BB_MAX_TB_BYTES) || peer->pending_downlink) {
        return -1;
    }
    sdu.lcid = ((length > 5U) &&
                (transport_block[5] == (uint8_t)BB_PDU_DATA)) ?
               (uint8_t)BB_MAC_LCID_DATA :
               (uint8_t)BB_MAC_LCID_CONTROL;
    sdu.data = transport_block;
    sdu.length = (uint16_t)length;
    if (bb_mac_multiplex(&sdu, 1U, mac_transport_block,
                         sizeof(mac_transport_block), &mac_length) != 0) {
        return -2;
    }
    transmit.t0 = tick;
    transmit.sample_rate_hz = peer->channel_config.sample_rate_hz;
    transmit.count = BB_MAX_IQ_SAMPLES;
    transmit.samples = peer->downlink_tx;
    if (bb_phy_transmit(&peer->channel_config, mac_transport_block, mac_length,
                        &transmit) != 0) {
        return -2;
    }
    receive.t0 = tick;
    receive.sample_rate_hz = transmit.sample_rate_hz;
    receive.count = BB_MAX_IQ_SAMPLES;
    receive.samples = peer->downlink_rx;
    if (bb_channel_apply(&peer->channel_config, &transmit, &receive) != 0) {
        return -3;
    }
    peer->pending_downlink_tick = tick;
    peer->pending_downlink_count = receive.count;
    peer->pending_downlink = true;
    return bb_soc_step(soc, modem, tick);
}

static void peer_handle_pdu(struct network_peer *peer,
                            const uint8_t *encoded, size_t encoded_length)
{
    struct bb_wire_pdu pdu;
    uint32_t budget = 1000U;
    uint32_t full_count;
    bool require_integrity;
    if (encoded_length < BB_WIRE_HEADER_SIZE) {
        return;
    }
    require_integrity = (encoded[5] == (uint8_t)BB_PDU_SECURITY_COMPLETE) ||
                        (peer->security_active && protected_type(encoded[5]));
    if (bb_protocol_decode(encoded, encoded_length, &pdu, false,
                           &budget) != 0) {
        return;
    }
    full_count = pdu.pdcp_count & BB_PDCP_SN_MASK;
    if (peer->uplink_count_initialized) {
        uint32_t hfn = peer->uplink_rx_next >> BB_PDCP_SN_BITS;
        const uint32_t expected_sn =
            peer->uplink_rx_next & BB_PDCP_SN_MASK;
        const uint32_t half = (BB_PDCP_SN_MASK + 1U) / 2U;
        if ((full_count < expected_sn) &&
            ((expected_sn - full_count) > half)) {
            hfn = (hfn + 1U) & 0x3FFFU;
        } else if ((full_count > expected_sn) &&
                   ((full_count - expected_sn) > half)) {
            hfn = (hfn - 1U) & 0x3FFFU;
        }
        full_count |= hfn << BB_PDCP_SN_BITS;
    }
    if (require_integrity) {
        budget = 1000U;
        if (bb_protocol_decode_with_count(encoded, encoded_length, &pdu,
                                          true, full_count, &budget) != 0) {
            return;
        }
    } else {
        pdu.pdcp_count = full_count;
    }
    if (pdu.type == (uint8_t)BB_PDU_RRC_COMPLETE) {
        peer->rrc_complete = true;
    } else if (pdu.type == (uint8_t)BB_PDU_REG_REQUEST) {
        peer->registration_request = true;
    } else if (pdu.type == (uint8_t)BB_PDU_SECURITY_COMPLETE) {
        peer->security_complete = true;
        peer->security_active = true;
    } else if (pdu.type == (uint8_t)BB_PDU_NACK) {
        peer->nack_received = true;
        peer->nack_harq = pdu.harq_id;
        peer->nack_count = pdu.pdcp_count;
    } else if (pdu.type == (uint8_t)BB_PDU_DATA_ACK) {
        peer->data_acks++;
    }
    if (!peer->uplink_count_initialized ||
        ((pdu.pdcp_count - peer->uplink_rx_next) < UINT32_C(0x80000000))) {
        peer->uplink_rx_next = pdu.pdcp_count + 1U;
        peer->uplink_count_initialized = true;
    }
}

int network_peer_drain_uplink(struct network_peer *peer,
                              struct bb_soc *soc, struct bb_modem *modem,
                              uint64_t tick)
{
    if ((peer == NULL) || (soc == NULL) || (modem == NULL)) {
        return -1;
    }
    return bb_soc_step(soc, modem, tick);
}

int network_peer_submit_iq(struct network_peer *peer, struct bb_soc *soc,
                           struct bb_modem *modem,
                           const struct bb_iq_block *block, uint64_t tick)
{
    if ((peer == NULL) || (soc == NULL) || (modem == NULL) ||
        (block == NULL) || (block->samples == NULL) ||
        (block->count == 0U) || (block->count > BB_MAX_IQ_SAMPLES) ||
        (block->sample_rate_hz != peer->channel_config.sample_rate_hz) ||
        peer->pending_downlink) {
        return -1;
    }
    copy_samples(peer->downlink_rx, block->samples, block->count);
    peer->pending_downlink_tick = tick;
    peer->pending_downlink_count = block->count;
    peer->pending_downlink = true;
    return bb_soc_step(soc, modem, tick);
}
