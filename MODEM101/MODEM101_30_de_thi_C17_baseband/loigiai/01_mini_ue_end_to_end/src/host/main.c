#include "bb/bb.h"
#include "bb/protocol.h"
#include "network_peer.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOST_MODEM_ARENA_BYTES (1024U * 1024U)
#define HOST_MAX_EVENTS 256U

_Alignas(max_align_t) static uint8_t modem_arena[HOST_MODEM_ARENA_BYTES];
_Alignas(max_align_t) static uint8_t soc_arena[4096U];
static struct bb_cpx16 input_iq_samples[BB_MAX_IQ_SAMPLES];
static uint8_t source_sdu[BB_MAX_SDU_BYTES];
static uint8_t delivered_sdu[BB_MAX_SDU_BYTES];
static struct network_peer peer;

struct trace_record {
    uint64_t tick;
    const char *checkpoint;
    int32_t value;
};
struct trace_context {
    FILE *stream;
    int32_t last_decode_bytes;
    uint32_t count;
    uint32_t dropped;
    struct trace_record records[4096];
};
struct host_platform {
    uint64_t tick;
    uint32_t irq_status, irq_acks, cache_cleans, cache_invalidates;
    struct bb_soc *soc;
};
struct parsed_event {
    uint64_t tick;
    uint16_t source;
    uint8_t payload[64];
    uint8_t payload_length;
};
struct event_file {
    struct parsed_event events[HOST_MAX_EVENTS];
    uint32_t count;
};

static uint64_t platform_ticks(void *context)
{
    return ((const struct host_platform *)context)->tick;
}

static uint32_t platform_mmio_read32(void *context, uintptr_t address)
{
    struct host_platform *const platform = (struct host_platform *)context;
    uint32_t value = 0U;
    if ((platform->soc != NULL) &&
        (bb_soc_mmio_read32(platform->soc, address, &value) == 0)) {
        return value;
    }
    return ((address & 3U) == 0U) ? platform->irq_status : 0U;
}

static void platform_mmio_write32(void *context, uintptr_t address,
                                  uint32_t value)
{
    struct host_platform *const platform = (struct host_platform *)context;
    if (platform->soc != NULL) {
        (void)bb_soc_mmio_write32(platform->soc, address, value);
    }
    if (((address & 3U) == 0U) && (value != 0U)) {
        platform->irq_status &= ~value;
        platform->irq_acks++;
    }
}

static void platform_cache_clean(void *context, const void *address,
                                 size_t length)
{
    struct host_platform *const platform = (struct host_platform *)context;
    if ((address != NULL) && (length != 0U)) {
        platform->cache_cleans++;
    }
}

static void platform_cache_invalidate(void *context, void *address,
                                      size_t length)
{
    struct host_platform *const platform = (struct host_platform *)context;
    if ((address != NULL) && (length != 0U)) {
        platform->cache_invalidates++;
    }
}

static void trace_state(FILE *stream, uint64_t tick, const char *domain,
                        const char *from, const char *to, int32_t identifier)
{
    (void)fprintf(stream,
                  "{\"tick\":%" PRIu64 ",\"domain\":\"%s\","
                  "\"event\":\"STATE\",\"from\":\"%s\","
                  "\"to\":\"%s\",\"id\":%" PRId32 "}\n",
                  tick, domain, from, to, identifier);
}

static void trace_emit_json(void *context, uint64_t tick,
                            const char *checkpoint, int32_t value,
                            int32_t harq_id)
{
    FILE *const stream = ((struct trace_context *)context)->stream;
    if (stream == NULL) {
        return;
    }
    if (strcmp(checkpoint, "DECODE") == 0) {
        ((struct trace_context *)context)->last_decode_bytes = value;
    }
    if (strcmp(checkpoint, "SYNC") == 0) {
        ((struct trace_context *)context)->last_decode_bytes = 0;
    }
    if (strcmp(checkpoint, "RRC_OFF_SEARCHING") == 0) {
        trace_state(stream, tick, "RRC", "OFF", "SEARCHING", value);
    } else if (strcmp(checkpoint, "RRC_SEARCHING_CAMPED") == 0) {
        trace_state(stream, tick, "RRC", "SEARCHING", "CAMPED", value);
    } else if (strcmp(checkpoint, "RRC_CAMPED_CONNECTING") == 0) {
        trace_state(stream, tick, "RRC", "CAMPED", "CONNECTING", value);
    } else if (strcmp(checkpoint, "RRC_CONNECTING_CONNECTED") == 0) {
        trace_state(stream, tick, "RRC", "CONNECTING", "CONNECTED", value);
    } else if (strcmp(checkpoint, "RRC_CONNECTED_SEARCHING_LINK_LOSS") == 0) {
        trace_state(stream, tick, "RRC", "CONNECTED", "SEARCHING", value);
    } else if (strcmp(checkpoint, "NAS_DEREGISTERED_REGISTERING") == 0) {
        trace_state(stream, tick, "NAS", "DEREGISTERED", "REGISTERING", value);
    } else if (strcmp(checkpoint, "NAS_REGISTERING_REGISTERED") == 0) {
        trace_state(stream, tick, "NAS", "REGISTERING", "REGISTERED", value);
    } else if (strcmp(checkpoint, "NAS_REGISTERED_SESSION_ACTIVE") == 0) {
        trace_state(stream, tick, "NAS", "REGISTERED", "SESSION_ACTIVE", value);
    } else if (strcmp(checkpoint, "TB_CRC") == 0) {
        const struct trace_context *const trace =
            (const struct trace_context *)context;
        (void)fprintf(stream,
                      "{\"tick\":%" PRIu64 ",\"domain\":\"PHY\","
                      "\"event\":\"TB\",\"checkpoint\":\"TB_CRC\","
                      "\"dir\":\"DL\",\"harq\":%" PRId32 ","
                      "\"crc\":%s,\"bytes\":%" PRId32 "}\n",
                      tick, harq_id, (value != 0) ? "true" : "false",
                      trace->last_decode_bytes);
    } else {
        const bool phy_domain =
            (strcmp(checkpoint, "SYNC") == 0) ||
            (strcmp(checkpoint, "TIMING_OFFSET") == 0) ||
            (strcmp(checkpoint, "CFO_HZ") == 0) ||
            (strcmp(checkpoint, "FFT") == 0) ||
            (strcmp(checkpoint, "CHANNEL_EST") == 0) ||
            (strcmp(checkpoint, "DEMAP") == 0) ||
            (strcmp(checkpoint, "DECODE") == 0) ||
            (strcmp(checkpoint, "TX_WAVEFORM") == 0);
        (void)fprintf(stream,
                      "{\"tick\":%" PRIu64 ",\"domain\":\"%s\","
                      "\"event\":\"%s\",\"value\":%" PRId32 "}\n",
                      tick, phy_domain ? "PHY" : "MODEM", checkpoint, value);
    }
}

static void trace_json(void *context, uint64_t tick, const char *checkpoint,
                       int32_t value)
{
    struct trace_context *const trace = (struct trace_context *)context;
    if (trace->count < 4096U) {
        trace->records[trace->count].tick = tick;
        trace->records[trace->count].checkpoint = checkpoint;
        trace->records[trace->count].value = value;
        trace->count++;
    } else {
        trace->dropped++;
    }
}

static void trace_flush(struct trace_context *trace)
{
    uint32_t index;
    for (index = 0U; index < trace->count; ++index) {
        int32_t harq_id = -1;
        if (strcmp(trace->records[index].checkpoint, "TB_HARQ") == 0) {
            continue;
        }
        if (strcmp(trace->records[index].checkpoint, "TB_CRC") == 0) {
            uint32_t lookahead = index + 1U;
            while ((lookahead < trace->count) &&
                   (trace->records[lookahead].tick ==
                    trace->records[index].tick)) {
                if (strcmp(trace->records[lookahead].checkpoint,
                           "TB_HARQ") == 0) {
                    harq_id = trace->records[lookahead].value;
                    break;
                }
                lookahead++;
            }
        }
        trace_emit_json(trace, trace->records[index].tick,
                        trace->records[index].checkpoint,
                        trace->records[index].value, harq_id);
    }
    if ((trace->stream != NULL) && (trace->dropped != 0U)) {
        (void)fprintf(trace->stream,
                      "{\"tick\":0,\"domain\":\"TRACE\","
                      "\"event\":\"DROPPED\",\"value\":%" PRIu32 "}\n",
                      trace->dropped);
    }
    trace->count = 0U;
}

static void put_u32_le(uint8_t bytes[4], uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8U);
    bytes[2] = (uint8_t)(value >> 16U);
    bytes[3] = (uint8_t)(value >> 24U);
}

static void put_u64_le(uint8_t bytes[8], uint64_t value)
{
    uint32_t index;
    for (index = 0U; index < 8U; ++index) {
        bytes[index] = (uint8_t)(value >> (index * 8U));
    }
}

static uint32_t get_u32_le(const uint8_t bytes[4])
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

static uint64_t get_u64_le(const uint8_t bytes[8])
{
    uint64_t value = 0U;
    uint32_t index;
    for (index = 0U; index < 8U; ++index) {
        value |= (uint64_t)bytes[index] << (index * 8U);
    }
    return value;
}

static int write_iq(const char *path, const struct bb_iq_block *block)
{
    FILE *stream;
    uint8_t header[16];
    uint32_t index;
    if ((path == NULL) || (block == NULL) || (block->samples == NULL) ||
        (block->count > BB_MAX_IQ_SAMPLES)) {
        return -1;
    }
    stream = fopen(path, "wb");
    if (stream == NULL) {
        return -1;
    }
    header[0] = (uint8_t)'I'; header[1] = (uint8_t)'Q';
    header[2] = (uint8_t)'1'; header[3] = (uint8_t)'6';
    put_u32_le(&header[4], block->sample_rate_hz);
    put_u64_le(&header[8], block->count);
    if (fwrite(header, 1U, sizeof(header), stream) != sizeof(header)) {
        (void)fclose(stream);
        return -1;
    }
    for (index = 0U; index < block->count; ++index) {
        uint8_t pair[4];
        const uint16_t in_phase = (uint16_t)block->samples[index].i;
        const uint16_t quadrature = (uint16_t)block->samples[index].q;
        pair[0] = (uint8_t)in_phase;
        pair[1] = (uint8_t)(in_phase >> 8U);
        pair[2] = (uint8_t)quadrature;
        pair[3] = (uint8_t)(quadrature >> 8U);
        if (fwrite(pair, 1U, sizeof(pair), stream) != sizeof(pair)) {
            (void)fclose(stream);
            return -1;
        }
    }
    return (fclose(stream) == 0) ? 0 : -1;
}

static int read_iq(const char *path, struct bb_iq_block *block)
{
    FILE *stream;
    uint8_t header[16];
    uint64_t count;
    uint32_t index;
    int trailing;
    if ((path == NULL) || (block == NULL) || (block->samples == NULL)) {
        return -1;
    }
    stream = fopen(path, "rb");
    if (stream == NULL) {
        return -1;
    }
    if (fread(header, 1U, sizeof(header), stream) != sizeof(header)) {
        (void)fclose(stream);
        return -2;
    }
    count = get_u64_le(&header[8]);
    if ((header[0] != (uint8_t)'I') || (header[1] != (uint8_t)'Q') ||
        (header[2] != (uint8_t)'1') || (header[3] != (uint8_t)'6') ||
        (get_u32_le(&header[4]) == 0U) || (count == 0U) ||
        (count > BB_MAX_IQ_SAMPLES) || (count > UINT32_MAX)) {
        (void)fclose(stream);
        return -3;
    }
    for (index = 0U; index < (uint32_t)count; ++index) {
        uint8_t pair[4];
        uint16_t in_phase;
        uint16_t quadrature;
        if (fread(pair, 1U, sizeof(pair), stream) != sizeof(pair)) {
            (void)fclose(stream);
            return -4;
        }
        in_phase = (uint16_t)((uint16_t)pair[0] | ((uint16_t)pair[1] << 8U));
        quadrature = (uint16_t)((uint16_t)pair[2] |
                                ((uint16_t)pair[3] << 8U));
        block->samples[index].i = (int16_t)in_phase;
        block->samples[index].q = (int16_t)quadrature;
    }
    trailing = fgetc(stream);
    if (trailing != EOF) {
        (void)fclose(stream);
        return -5;
    }
    block->t0 = 0U;
    block->sample_rate_hz = get_u32_le(&header[4]);
    block->count = (uint32_t)count;
    return (fclose(stream) == 0) ? 0 : -6;
}

static int parse_u32(const char *text, uint32_t *value)
{
    char *end = NULL;
    unsigned long parsed;
    errno = 0;
    parsed = strtoul(text, &end, 0);
    if ((errno != 0) || (end == text) || (*end != '\0') ||
        (parsed > UINT32_MAX)) {
        return -1;
    }
    *value = (uint32_t)parsed;
    return 0;
}

static int parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed;
    errno = 0;
    parsed = strtoull(text, &end, 0);
    if ((errno != 0) || (end == text) || (*end != '\0')) {
        return -1;
    }
    *value = (uint64_t)parsed;
    return 0;
}

static int parse_i32(const char *text, int32_t *value)
{
    char *end = NULL;
    long parsed;
    errno = 0;
    parsed = strtol(text, &end, 0);
    if ((errno != 0) || (end == text) || (*end != '\0') ||
        (parsed < INT32_MIN) || (parsed > INT32_MAX)) {
        return -1;
    }
    *value = (int32_t)parsed;
    return 0;
}

static int parse_config(const char *path, struct bb_config *config)
{
    FILE *stream;
    char line[256];
    uint32_t seen = 0U;
    if (path == NULL) {
        return 0;
    }
    stream = fopen(path, "r");
    if (stream == NULL) {
        return -1;
    }
    while (fgets(line, (int)sizeof(line), stream) != NULL) {
        char *separator;
        char *newline = strchr(line, '\n');
        uint32_t value;
        if (newline != NULL) {
            *newline = '\0';
            if ((newline != line) && (newline[-1] == '\r')) {
                newline[-1] = '\0';
            }
        } else if (!feof(stream)) {
            (void)fclose(stream); return -1;
        }
        if ((line[0] == '\0') || (line[0] == '#')) {
            continue;
        }
        separator = strchr(line, '=');
        if (separator == NULL) {
            (void)fclose(stream); return -1;
        }
        *separator = '\0';
        if (strcmp(line, "cfo_hz") == 0) {
            if ((seen & 4U) != 0U) { (void)fclose(stream); return -1; }
            if (parse_i32(separator + 1, &config->cfo_hz) != 0) {
                (void)fclose(stream); return -1;
            }
            seen |= 4U;
            continue;
        }
        if (parse_u32(separator + 1, &value) != 0) {
            (void)fclose(stream); return -1;
        }
        if (strcmp(line, "sample_rate_hz") == 0) {
            if ((seen & 1U) != 0U) { (void)fclose(stream); return -1; }
            config->sample_rate_hz = value;
            seen |= 1U;
        } else if (strcmp(line, "seed") == 0) {
            if ((seen & 2U) != 0U) { (void)fclose(stream); return -1; }
            config->seed = value;
            seen |= 2U;
        } else if (strcmp(line, "noise_amplitude") == 0) {
            if ((value > UINT16_MAX) || ((seen & 8U) != 0U)) {
                (void)fclose(stream); return -1;
            }
            config->noise_amplitude = (uint16_t)value;
            seen |= 8U;
        } else if (strcmp(line, "pci") == 0) {
            if ((value > 1007U) || ((seen & 16U) != 0U)) {
                (void)fclose(stream); return -1;
            }
            config->pci = (uint16_t)value;
            seen |= 16U;
        } else if (strcmp(line, "timing_offset_samples") == 0) {
            if ((value > UINT16_MAX) || ((seen & 32U) != 0U)) {
                (void)fclose(stream); return -1;
            }
            config->timing_offset_samples = (uint16_t)value;
            seen |= 32U;
        } else if (strcmp(line, "modulation") == 0) {
            if (((value != (uint32_t)BB_MOD_QPSK) &&
                 (value != (uint32_t)BB_MOD_16QAM)) ||
                ((seen & 64U) != 0U)) {
                (void)fclose(stream); return -1;
            }
            config->modulation = (uint8_t)value;
            seen |= 64U;
        } else {
            (void)fclose(stream); return -1;
        }
    }
    if ((fclose(stream) != 0) || (seen != 127U)) {
        return -1;
    }
    return 0;
}

static int hex_value(char character)
{
    if ((character >= '0') && (character <= '9')) return character - '0';
    if ((character >= 'a') && (character <= 'f')) return character - 'a' + 10;
    if ((character >= 'A') && (character <= 'F')) return character - 'A' + 10;
    return -1;
}

static int parse_events(const char *path, struct event_file *event_file)
{
    FILE *stream;
    char line[512];
    if (path == NULL) return 0;
    stream = fopen(path, "r");
    if (stream == NULL) return -1;
    while (fgets(line, (int)sizeof(line), stream) != NULL) {
        char *first;
        char *second;
        char *newline = strchr(line, '\n');
        struct parsed_event *event;
        uint32_t source;
        size_t hex_length;
        size_t index;
        if (newline != NULL) *newline = '\0';
        else if (!feof(stream)) { (void)fclose(stream); return -2; }
        if ((line[0] == '\0') || (line[0] == '#')) continue;
        if (event_file->count >= HOST_MAX_EVENTS) {
            (void)fclose(stream); return -3;
        }
        first = strchr(line, '|');
        if (first == NULL) { (void)fclose(stream); return -4; }
        *first = '\0';
        second = strchr(first + 1, '|');
        if (second == NULL) { (void)fclose(stream); return -4; }
        *second = '\0';
        event = &event_file->events[event_file->count];
        if ((parse_u64(line, &event->tick) != 0) ||
            (parse_u32(first + 1, &source) != 0) ||
            (source > UINT16_MAX)) {
            (void)fclose(stream); return -5;
        }
        hex_length = strlen(second + 1);
        if (((hex_length & 1U) != 0U) || (hex_length > 128U)) {
            (void)fclose(stream); return -6;
        }
        event->source = (uint16_t)source;
        event->payload_length = (uint8_t)(hex_length / 2U);
        for (index = 0U; index < hex_length / 2U; ++index) {
            const int high = hex_value(second[1U + index * 2U]);
            const int low = hex_value(second[2U + index * 2U]);
            if ((high < 0) || (low < 0)) {
                (void)fclose(stream); return -7;
            }
            event->payload[index] = (uint8_t)((uint32_t)high * 16U +
                                               (uint32_t)low);
        }
        event_file->count++;
    }
    if (fclose(stream) != 0) return -8;
    /* Stable insertion sort: timestamp controls processing order, while equal
     * timestamps retain their original file order. */
    {
        uint32_t outer;
        for (outer = 1U; outer < event_file->count; ++outer) {
            const struct parsed_event current = event_file->events[outer];
            uint32_t inner = outer;
            while ((inner > 0U) &&
                   (event_file->events[inner - 1U].tick > current.tick)) {
                event_file->events[inner] = event_file->events[inner - 1U];
                inner--;
            }
            event_file->events[inner] = current;
        }
    }
    return 0;
}

static int argument_value(int argc, char **argv, int *index,
                          const char **destination)
{
    if ((*index + 1) >= argc) return -1;
    *index += 1;
    *destination = argv[*index];
    return 0;
}

static struct bb_wire_pdu control_pdu(uint8_t type)
{
    struct bb_wire_pdu pdu;
    pdu.type = type; pdu.harq_id = 0U; pdu.flags = 0U; pdu.rlc_sn = 0U;
    pdu.pdcp_count = 0U; pdu.offset = 0U; pdu.total_length = 0U;
    pdu.payload = NULL; pdu.payload_length = 0U;
    return pdu;
}

static int send_and_drain(struct bb_soc *soc, struct bb_modem *modem,
                          struct host_platform *platform,
                          const struct bb_wire_pdu *pdu, uint64_t tick,
                          bool corrupt)
{
    platform->tick = tick;
    platform->irq_status = 1U;
    if (network_peer_send(&peer, soc, modem, pdu, tick, corrupt) != 0) return -1;
    return (network_peer_drain_uplink(&peer, soc, modem, tick) < 0) ? -2 : 0;
}

static int run_mandatory_scenario(struct bb_soc *soc, struct bb_modem *modem,
                                  struct host_platform *platform)
{
    struct bb_wire_pdu pdu;
    uint64_t tick = 100U;
    uint16_t rlc_sequence = 0U;
    uint32_t count = 0U;
    uint32_t offset;
    uint32_t index;
    const uint8_t cell_identity_ie[5] = {
        1U, 2U, 0U, (uint8_t)peer.channel_config.pci,
        (uint8_t)(peer.channel_config.pci >> 8U)
    };
    for (index = 0U; index < BB_MAX_SDU_BYTES; ++index) {
        source_sdu[index] = (uint8_t)((index * 29U + 7U) & 0xFFU);
    }
    pdu = control_pdu((uint8_t)BB_PDU_CELL_BEACON);
    pdu.payload = cell_identity_ie;
    pdu.payload_length = (uint16_t)sizeof(cell_identity_ie);
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -1;
    tick += 500U;
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -1;
    tick += 500U; pdu = control_pdu((uint8_t)BB_PDU_RRC_SETUP);
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -2;
    tick += 500U;
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -2;
    if (!peer.rrc_complete || !peer.registration_request) return -3;
    tick += 500U; pdu = control_pdu((uint8_t)BB_PDU_SECURITY_MODE);
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -4;
    if (!peer.security_complete) return -5;
    tick += 500U; pdu = control_pdu((uint8_t)BB_PDU_REG_ACCEPT);
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -6;
    tick += 500U; pdu = control_pdu((uint8_t)BB_PDU_SESSION_ACCEPT);
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -7;

    tick += 500U; pdu = control_pdu((uint8_t)BB_PDU_DATA_GRANT);
    pdu.harq_id = 0U; pdu.pdcp_count = 0U;
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -8;
    tick += 200U; pdu = control_pdu((uint8_t)BB_PDU_DATA);
    pdu.harq_id = 0U; pdu.rlc_sn = 0U; pdu.pdcp_count = 0U;
    pdu.offset = 0U; pdu.total_length = BB_MAX_SDU_BYTES;
    pdu.payload = source_sdu; pdu.payload_length = 224U;
    if (send_and_drain(soc, modem, platform, &pdu, tick, true) != 0) return -9;
    tick += 1200U; platform->tick = tick;
    if (network_peer_drain_uplink(&peer, soc, modem, tick) < 0) return -10;
    if (!peer.nack_received || (peer.nack_harq != 0U) ||
        (peer.nack_count != 0U)) return -11;
    /* Retransmit the same transport block.  RETX is derived from the HARQ
     * process/timer state rather than from an artificial on-wire marker. */
    tick += 100U;
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -12;
    /* Send RLC SN=2 before SN=1.  Once RLC restores sequence order, COUNT=2
     * deliberately precedes COUNT=1 so PDCP must independently reorder too. */
    tick += 300U; pdu = control_pdu((uint8_t)BB_PDU_DATA);
    pdu.harq_id = 1U; pdu.rlc_sn = 2U; pdu.pdcp_count = 1U;
    pdu.offset = 224U; pdu.total_length = BB_MAX_SDU_BYTES;
    pdu.payload = &source_sdu[224U]; pdu.payload_length = 224U;
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -13;
    tick += 300U; pdu = control_pdu((uint8_t)BB_PDU_DATA);
    pdu.harq_id = 2U; pdu.rlc_sn = 1U; pdu.pdcp_count = 2U;
    pdu.offset = 448U; pdu.total_length = BB_MAX_SDU_BYTES;
    pdu.payload = &source_sdu[448U]; pdu.payload_length = 224U;
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -13;
    rlc_sequence = 3U; count = 3U;

    for (offset = 672U; offset < BB_MAX_SDU_BYTES; offset += 224U) {
        const uint32_t remaining = BB_MAX_SDU_BYTES - offset;
        const uint16_t segment_length =
            (uint16_t)((remaining < 224U) ? remaining : 224U);
        tick += 300U; pdu = control_pdu((uint8_t)BB_PDU_DATA);
        pdu.harq_id = (uint8_t)(count & 7U); pdu.rlc_sn = rlc_sequence;
        pdu.pdcp_count = count; pdu.offset = (uint16_t)offset;
        pdu.total_length = BB_MAX_SDU_BYTES; pdu.payload = &source_sdu[offset];
        pdu.payload_length = segment_length;
        if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -13;
        rlc_sequence = (uint16_t)((rlc_sequence + 1U) & 0x0FFFU);
        count = (count + 1U) & 0x3FFFFU;
    }

    tick += 500U; pdu = control_pdu((uint8_t)BB_PDU_DATA);
    pdu.harq_id = 0U; pdu.rlc_sn = rlc_sequence; pdu.pdcp_count = 0U;
    pdu.offset = 0U; pdu.total_length = BB_MAX_SDU_BYTES;
    pdu.payload = source_sdu; pdu.payload_length = 224U;
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -14;
    tick += 500U; pdu = control_pdu((uint8_t)BB_PDU_LINK_LOSS);
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -15;
    tick += 500U; pdu = control_pdu((uint8_t)BB_PDU_CELL_BEACON);
    pdu.payload = cell_identity_ie;
    pdu.payload_length = (uint16_t)sizeof(cell_identity_ie);
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -16;
    tick += 500U; pdu = control_pdu((uint8_t)BB_PDU_RRC_SETUP);
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -17;
    tick += 500U;
    if (send_and_drain(soc, modem, platform, &pdu, tick, false) != 0) return -18;
    return 0;
}

static int apply_external_events(struct bb_soc *soc, struct bb_modem *modem,
                                 struct host_platform *platform,
                                 const struct event_file *event_file)
{
    uint32_t event_index;
    for (event_index = 0U; event_index < event_file->count; ++event_index) {
        const struct parsed_event *const input = &event_file->events[event_index];
        if (input->tick < platform->tick) {
            return -1;
        }
        platform->tick = input->tick;
        platform->irq_status = 1U;
        if (input->source == 100U) {
            uint16_t irq;
            if (input->payload_length != 2U) return -2;
            irq = (uint16_t)((uint16_t)input->payload[0] |
                             ((uint16_t)input->payload[1] << 8U));
            if ((bb_modem_irq(modem, irq) != 0) ||
                (bb_modem_run_until(modem, input->tick) != 0)) return -3;
        } else if (input->source == 101U) {
            if ((input->payload_length == 0U) ||
                (network_peer_send_raw(&peer, soc, modem, input->payload,
                                       input->payload_length,
                                       input->tick) != 0)) return -4;
        } else if (input->source == 102U) {
            uint16_t delay;
            if (input->payload_length != 2U) return -5;
            delay = (uint16_t)((uint16_t)input->payload[0] |
                               ((uint16_t)input->payload[1] << 8U));
            if (UINT64_MAX - delay < input->tick) return -5;
            platform->tick = input->tick + delay;
            if (network_peer_drain_uplink(&peer, soc, modem,
                                          platform->tick) < 0) return -5;
        } else if (input->source == 103U) {
            uint32_t index;
            uint32_t rejected = 0U;
            const uint32_t count = (input->payload_length == 1U) ?
                                   input->payload[0] : 65U;
            for (index = 0U; index < count; ++index) {
                struct bb_event event = {
                    input->tick, 103U, 1U, { 0U, 0U, 0U }
                };
                if (bb_modem_post(modem, &event) != 0) rejected++;
            }
            if ((count > 64U) && (rejected == 0U)) return -6;
            if (bb_modem_run_until(modem, input->tick) != 0) return -6;
        } else if (input->source == 104U) {
            if ((input->payload_length != 1U) ||
                (bb_modem_restart_domain(modem, input->payload[0],
                                         input->tick) != 0)) return -7;
        } else {
            return -8;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct bb_config config = {
        .sample_rate_hz = 7680000U, .seed = 0x101U, .cfo_hz = 1800,
        .noise_amplitude = 12U, .pci = 42U, .timing_offset_samples = 37U,
        .modulation = (uint8_t)BB_MOD_16QAM, .reserved = 0U,
        .trace = NULL, .trace_ctx = NULL
    };
    struct host_platform platform = { 0U, 1U, 0U, 0U, 0U, NULL };
    const struct bb_platform_ops platform_ops = {
        platform_ticks, platform_mmio_read32, platform_mmio_write32,
        platform_cache_clean, platform_cache_invalidate
    };
    struct trace_context trace = { NULL, 0, 0U, 0U, { { 0U, NULL, 0 } } };
    struct event_file event_file;
    struct bb_modem *modem = NULL;
    struct bb_soc *soc = NULL;
    struct bb_session_status status = { BB_RRC_OFF, BB_NAS_DEREGISTERED,
                                        0U, 0U, 0U, 0U, 0U, 0U, 0U, false };
    struct bb_stats stats = { 0U, 0U, 0U, 0U, 0U };
    const char *config_path = NULL, *events_path = NULL, *iq_in_path = NULL;
    const char *iq_out_path = "uplink.iq", *trace_path = NULL;
    FILE *trace_file = NULL;
    size_t delivered_length = 0U;
    int scenario_result = 0;
    int index;
    bool pass;
    event_file.count = 0U;

    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--config") == 0) {
            if (argument_value(argc, argv, &index, &config_path) != 0) return EXIT_FAILURE;
        } else if (strcmp(argv[index], "--events") == 0) {
            if (argument_value(argc, argv, &index, &events_path) != 0) return EXIT_FAILURE;
        } else if (strcmp(argv[index], "--iq-in") == 0) {
            if (argument_value(argc, argv, &index, &iq_in_path) != 0) return EXIT_FAILURE;
        } else if (strcmp(argv[index], "--iq-out") == 0) {
            if (argument_value(argc, argv, &index, &iq_out_path) != 0) return EXIT_FAILURE;
        } else if (strcmp(argv[index], "--trace") == 0) {
            if (argument_value(argc, argv, &index, &trace_path) != 0) return EXIT_FAILURE;
        } else {
            (void)fprintf(stderr, "unknown argument: %s\n", argv[index]);
            return EXIT_FAILURE;
        }
    }
    if ((parse_config(config_path, &config) != 0) ||
        (parse_events(events_path, &event_file) != 0)) {
        (void)fprintf(stderr, "invalid config or events file\n");
        return EXIT_FAILURE;
    }
    if (trace_path != NULL) {
        trace_file = fopen(trace_path, "w");
        if (trace_file == NULL) return EXIT_FAILURE;
        trace.stream = trace_file;
    }
    config.trace = trace_json; config.trace_ctx = &trace;
    network_peer_init(&peer, &config);
    if ((bb_soc_required_memory(&config) > sizeof(soc_arena)) ||
        (bb_soc_init(soc_arena, sizeof(soc_arena), &config,
                     network_peer_rf_backend(), &peer, &soc) != 0)) {
        scenario_result = -99;
    }
    platform.soc = soc;
    if ((scenario_result == 0) &&
        ((bb_modem_required_memory(&config) > sizeof(modem_arena)) ||
        (bb_modem_init(modem_arena, sizeof(modem_arena), &config,
                       &platform_ops, &platform, &modem) != 0))) {
        scenario_result = -100;
    }
    if ((scenario_result == 0) && (iq_in_path != NULL)) {
        struct bb_iq_block input = { 0U, 0U, BB_MAX_IQ_SAMPLES, input_iq_samples };
        if ((read_iq(iq_in_path, &input) != 0) ||
            (network_peer_submit_iq(&peer, soc, modem, &input, 0U) != 0)) {
            trace_json(&trace, 0U, "IQ_INPUT_REJECT", -101);
            scenario_result = -101;
        }
    }
    if (scenario_result == 0) {
        scenario_result = run_mandatory_scenario(soc, modem, &platform);
    }
    if (scenario_result == 0) {
        scenario_result = apply_external_events(soc, modem, &platform,
                                                &event_file);
    }
    if (modem != NULL) {
        bb_modem_get_session_status(modem, &status);
        bb_modem_get_stats(modem, &stats);
        if (bb_modem_copy_delivered_sdu(modem, delivered_sdu,
                                        sizeof(delivered_sdu),
                                        &delivered_length) != 0) delivered_length = 0U;
    }
    pass = (scenario_result == 0) && (status.rrc == BB_RRC_CONNECTED) &&
           (status.nas == BB_NAS_SESSION_ACTIVE) && status.security_active &&
           (delivered_length == BB_MAX_SDU_BYTES) &&
           (memcmp(source_sdu, delivered_sdu, BB_MAX_SDU_BYTES) == 0) &&
           (status.harq_retx >= 1U) && (status.harq_combines >= 1U) &&
           (status.rlc_reorders >= 1U) && (status.pdcp_reorders >= 1U) &&
           (status.replay_drops >= 1U) &&
           (status.recoveries >= 1U) && (stats.rx_crc_fail >= 1U) &&
           (platform.irq_acks > 0U) && (platform.cache_cleans > 0U) &&
           (platform.cache_invalidates > 0U);
    trace_flush(&trace);
    if (trace.stream != NULL) {
        (void)fprintf(trace.stream,
                      "{\"tick\":%" PRIu64 ",\"domain\":\"MODEM\","
                      "\"event\":\"FINAL\",\"registered\":%s,"
                      "\"session\":%s,\"delivered\":%" PRIu32 ","
                      "\"drops\":%" PRIu64 "}\n", platform.tick,
                      (status.nas >= BB_NAS_REGISTERED) ? "true" : "false",
                      (status.nas == BB_NAS_SESSION_ACTIVE) ? "true" : "false",
                      status.delivered, stats.dropped);
    }
    if ((iq_out_path != NULL) && (peer.last_uplink_count != 0U)) {
        const struct bb_iq_block last_uplink = {
            platform.tick, config.sample_rate_hz, peer.last_uplink_count,
            peer.last_uplink
        };
        if (write_iq(iq_out_path, &last_uplink) != 0) pass = false;
    }
    if (trace_file != NULL) (void)fclose(trace_file);
    (void)printf("MODEM_RESULT status=%s state=%s tx_bytes=%u rx_bytes=%zu "
                 "crc_fail=%" PRIu64 " recoveries=%" PRIu32 "\n",
                 pass ? "PASS" : "FAIL",
                 (status.nas == BB_NAS_SESSION_ACTIVE) ? "DATA" : "NOT_DATA",
                 peer.downlink_unique_bytes, delivered_length, stats.rx_crc_fail,
                 status.recoveries);
    return pass ? EXIT_SUCCESS : EXIT_FAILURE;
}
