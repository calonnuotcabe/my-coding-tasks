#include "internal.h"

#include <stdbool.h>

static void put_u16(uint8_t *output, uint16_t value)
{
    output[0] = (uint8_t)value;
    output[1] = (uint8_t)(value >> 8U);
}

static void put_u32(uint8_t *output, uint32_t value)
{
    output[0] = (uint8_t)value;
    output[1] = (uint8_t)(value >> 8U);
    output[2] = (uint8_t)(value >> 16U);
    output[3] = (uint8_t)(value >> 24U);
}

static uint16_t get_u16(const uint8_t *input)
{
    return (uint16_t)((uint16_t)input[0] | ((uint16_t)input[1] << 8U));
}

static uint32_t get_u32(const uint8_t *input)
{
    return (uint32_t)input[0] | ((uint32_t)input[1] << 8U) |
           ((uint32_t)input[2] << 16U) | ((uint32_t)input[3] << 24U);
}

uint32_t bb_crc32c(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFU;
    size_t index;
    if ((data == NULL) && (length != 0U)) {
        return 0U;
    }
    for (index = 0U; index < length; ++index) {
        uint32_t bit;
        crc ^= data[index];
        for (bit = 0U; bit < 8U; ++bit) {
            const uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0x82F63B78U & mask);
        }
    }
    return ~crc;
}

static int validate_tlvs(const uint8_t *tlvs, uint32_t length)
{
    uint32_t offset = 0U;
    uint32_t count = 0U;
    while (offset < length) {
        uint16_t value_length;
        uint32_t padded;
        uint32_t padding_index;
        if ((length - offset < 4U) || (count >= 64U)) {
            return -1;
        }
        value_length = get_u16(&tlvs[offset + 2U]);
        padded = ((uint32_t)value_length + 3U) & ~3U;
        if ((padded < value_length) ||
            (padded > length - offset - 4U)) {
            return -2;
        }
        for (padding_index = (uint32_t)value_length;
             padding_index < padded; ++padding_index) {
            if (tlvs[offset + 4U + padding_index] != 0U) {
                return -4;
            }
        }
        offset += 4U + padded;
        count++;
    }
    return (offset == length) ? 0 : -3;
}

int bb_ipc_encode(const struct bb_ipc_frame *frame, uint8_t *output,
                  size_t capacity, size_t *length)
{
    size_t total;
    size_t index;
    uint32_t crc;
    if ((frame == NULL) || (output == NULL) || (length == NULL) ||
        ((frame->tlvs == NULL) && (frame->tlv_length != 0U)) ||
        (frame->version != 1U) ||
        (frame->tlv_length > 4U * 1024U * 1024U - BB_IPC_HEADER_SIZE) ||
        (validate_tlvs(frame->tlvs, frame->tlv_length) != 0)) {
        return -1;
    }
    total = BB_IPC_HEADER_SIZE + (size_t)frame->tlv_length;
    if ((total > UINT32_MAX) || (capacity < total)) {
        return -2;
    }
    output[0] = (uint8_t)'B'; output[1] = (uint8_t)'B';
    output[2] = (uint8_t)'I'; output[3] = (uint8_t)'P';
    put_u16(&output[4], frame->version);
    put_u16(&output[6], frame->type);
    put_u32(&output[8], (uint32_t)total);
    put_u32(&output[12], frame->sequence);
    put_u32(&output[16], frame->epoch);
    put_u32(&output[20], frame->flags);
    put_u32(&output[24], 0U);
    for (index = 0U; index < frame->tlv_length; ++index) {
        output[BB_IPC_HEADER_SIZE + index] = frame->tlvs[index];
    }
    crc = bb_crc32c(output, total);
    put_u32(&output[24], crc);
    *length = total;
    return 0;
}

int bb_ipc_decode(const uint8_t *input, size_t length,
                  struct bb_ipc_frame *frame)
{
    uint32_t expected_crc;
    uint32_t actual_crc = 0xFFFFFFFFU;
    size_t index;
    if ((input == NULL) || (frame == NULL) ||
        (length < BB_IPC_HEADER_SIZE) ||
        (length > 4U * 1024U * 1024U) || (length > UINT32_MAX) ||
        (input[0] != (uint8_t)'B') || (input[1] != (uint8_t)'B') ||
        (input[2] != (uint8_t)'I') || (input[3] != (uint8_t)'P') ||
        (get_u16(&input[4]) != 1U) ||
        (get_u32(&input[8]) != length)) {
        return -1;
    }
    expected_crc = get_u32(&input[24]);
    for (index = 0U; index < length; ++index) {
        uint32_t bit;
        const uint8_t value = ((index >= 24U) && (index < 28U)) ?
                              0U : input[index];
        actual_crc ^= value;
        for (bit = 0U; bit < 8U; ++bit) {
            const uint32_t mask = 0U - (actual_crc & 1U);
            actual_crc = (actual_crc >> 1U) ^ (0x82F63B78U & mask);
        }
    }
    actual_crc = ~actual_crc;
    if (actual_crc != expected_crc) {
        return -2;
    }
    if (validate_tlvs(&input[BB_IPC_HEADER_SIZE],
                      (uint32_t)(length - BB_IPC_HEADER_SIZE)) != 0) {
        return -3;
    }
    frame->version = get_u16(&input[4]);
    frame->type = get_u16(&input[6]);
    frame->sequence = get_u32(&input[12]);
    frame->epoch = get_u32(&input[16]);
    frame->flags = get_u32(&input[20]);
    frame->tlvs = &input[BB_IPC_HEADER_SIZE];
    frame->tlv_length = (uint32_t)(length - BB_IPC_HEADER_SIZE);
    return 0;
}

static int ipc_ring_push(struct bb_ipc_storage_ring *ring,
                         const uint8_t *bytes, size_t length)
{
    uint32_t write_index;
    uint32_t read_index;
    struct bb_ipc_storage_slot *slot;
    size_t index;
    if ((ring == NULL) || (bytes == NULL) || (length < BB_IPC_HEADER_SIZE) ||
        (length > BB_IPC_MAX_FRAME_SIZE)) {
        return -1;
    }
    write_index = atomic_load_explicit(&ring->write_index,
                                       memory_order_relaxed);
    read_index = atomic_load_explicit(&ring->read_index,
                                      memory_order_acquire);
    if ((write_index - read_index) >= BB_IPC_RING_SLOTS) {
        return -2;
    }
    slot = &ring->slots[write_index & (BB_IPC_RING_SLOTS - 1U)];
    slot->length = (uint16_t)length;
    for (index = 0U; index < length; ++index) {
        slot->bytes[index] = bytes[index];
    }
    atomic_store_explicit(&ring->write_index, write_index + 1U,
                          memory_order_release);
    return 0;
}

static int ipc_ring_pop(struct bb_ipc_storage_ring *ring, uint8_t *bytes,
                        size_t capacity, size_t *length)
{
    uint32_t write_index;
    uint32_t read_index;
    const struct bb_ipc_storage_slot *slot;
    size_t index;
    if ((ring == NULL) || (bytes == NULL) || (length == NULL)) {
        return -1;
    }
    read_index = atomic_load_explicit(&ring->read_index,
                                      memory_order_relaxed);
    write_index = atomic_load_explicit(&ring->write_index,
                                       memory_order_acquire);
    if (read_index == write_index) {
        return -2;
    }
    slot = &ring->slots[read_index & (BB_IPC_RING_SLOTS - 1U)];
    if ((slot->length < BB_IPC_HEADER_SIZE) ||
        (slot->length > BB_IPC_MAX_FRAME_SIZE) ||
        (capacity < slot->length)) {
        return -3;
    }
    for (index = 0U; index < slot->length; ++index) {
        bytes[index] = slot->bytes[index];
    }
    *length = slot->length;
    atomic_store_explicit(&ring->read_index, read_index + 1U,
                          memory_order_release);
    return 0;
}

bool bb_ipc_has_request(const struct bb_modem *modem)
{
    uint32_t write_index;
    uint32_t read_index;
    if (modem == NULL) {
        return false;
    }
    read_index = atomic_load_explicit(&modem->ipc_requests.read_index,
                                      memory_order_relaxed);
    write_index = atomic_load_explicit(&modem->ipc_requests.write_index,
                                       memory_order_acquire);
    return read_index != write_index;
}

int bb_modem_ipc_submit(struct bb_modem *modem, const uint8_t *request,
                        size_t length)
{
    struct bb_ipc_frame frame;
    if ((modem == NULL) ||
        (bb_ipc_decode(request, length, &frame) != 0) ||
        ((frame.flags & BB_IPC_FLAG_RESPONSE) != 0U) ||
        (length > BB_IPC_MAX_FRAME_SIZE)) {
        return -1;
    }
    if (ipc_ring_push(&modem->ipc_requests, request, length) != 0) {
        modem->stats.dropped++;
        return -2;
    }
    (void)bb_mmio_write32(modem, 0x60008018U, 1U);
    if (bb_modem_irq(modem, 16U) != 0) {
        modem->stats.dropped++;
        return -3;
    }
    return 0;
}

int bb_modem_ipc_receive(struct bb_modem *modem, uint8_t *response,
                         size_t capacity, size_t *length)
{
    if (modem == NULL) {
        return -1;
    }
    return ipc_ring_pop(&modem->ipc_responses, response, capacity, length);
}

void bb_ipc_task_process(struct bb_modem *modem, uint64_t tick)
{
    uint8_t request[BB_IPC_MAX_FRAME_SIZE];
    uint8_t response[BB_IPC_MAX_FRAME_SIZE];
    struct bb_ipc_frame decoded;
    struct bb_ipc_frame reply;
    size_t request_length = 0U;
    size_t response_length = 0U;
    if ((modem == NULL) ||
        (ipc_ring_pop(&modem->ipc_requests, request, sizeof(request),
                      &request_length) != 0) ||
        (bb_ipc_decode(request, request_length, &decoded) != 0)) {
        if (modem != NULL) {
            modem->stats.dropped++;
            bb_trace(&modem->cfg, tick, "IPC_REQUEST_DROP", -1);
        }
        return;
    }
    if (modem->ipc_sequence_initialized &&
        (decoded.epoch == modem->ipc_last_epoch) &&
        (decoded.sequence == modem->ipc_last_sequence)) {
        modem->stats.dropped++;
        bb_trace(&modem->cfg, tick, "IPC_REPLAY_DROP",
                 (int32_t)decoded.sequence);
        return;
    }
    modem->ipc_sequence_initialized = true;
    modem->ipc_last_epoch = decoded.epoch;
    modem->ipc_last_sequence = decoded.sequence;
    reply.version = 1U;
    reply.type = (uint16_t)(decoded.type | UINT16_C(0x8000));
    reply.sequence = decoded.sequence;
    reply.epoch = decoded.epoch;
    reply.flags = decoded.flags | BB_IPC_FLAG_RESPONSE;
    reply.tlvs = decoded.tlvs;
    reply.tlv_length = decoded.tlv_length;
    if ((bb_ipc_encode(&reply, response, sizeof(response),
                       &response_length) != 0) ||
        (ipc_ring_push(&modem->ipc_responses, response,
                       response_length) != 0)) {
        modem->stats.dropped++;
        bb_trace(&modem->cfg, tick, "IPC_RESPONSE_DROP", -1);
        return;
    }
    atomic_thread_fence(memory_order_release);
    (void)bb_mmio_write32(modem, 0x60008018U, 2U);
    bb_trace(&modem->cfg, tick, "IPC_RESPONSE",
             (int32_t)decoded.sequence);
}
