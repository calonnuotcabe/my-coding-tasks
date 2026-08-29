#include "bb/bb.h"
#include "bb/ipc.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Alignas(max_align_t) static uint8_t arena[1024U * 1024U];
static struct bb_cpx16 channel_samples[BB_MAX_IQ_SAMPLES];

struct mock_platform { uint64_t tick; uint32_t clean, invalidate, ack; };
static uint64_t dispatched_ticks[8];
static uint32_t dispatched_count;

static void capture_trace(void *context, uint64_t tick,
                          const char *checkpoint, int32_t value)
{
    (void)context;
    (void)value;
    if ((strcmp(checkpoint, "EVENT") == 0) && (dispatched_count < 8U)) {
        dispatched_ticks[dispatched_count++] = tick;
    }
}

static uint64_t ticks(void *context)
{
    return ((struct mock_platform *)context)->tick;
}

static uint32_t mmio_read(void *context, uintptr_t address)
{
    (void)context;
    return ((address & 3U) == 0U) ? 1U : 0U;
}

static void mmio_write(void *context, uintptr_t address, uint32_t value)
{
    struct mock_platform *const platform = (struct mock_platform *)context;
    if (((address & 3U) == 0U) && (value != 0U)) platform->ack++;
}

static void clean(void *context, const void *address, size_t length)
{
    struct mock_platform *const platform = (struct mock_platform *)context;
    if ((address != NULL) && (length != 0U)) platform->clean++;
}

static void invalidate(void *context, void *address, size_t length)
{
    struct mock_platform *const platform = (struct mock_platform *)context;
    if ((address != NULL) && (length != 0U)) platform->invalidate++;
}

int main(void)
{
    static const uint8_t payload[] = "runtime deferred IQ path";
    struct bb_config config = {
        7680000U, 1U, 1800, 4U, 42U, 11U,
        (uint8_t)BB_MOD_QPSK, 0U, capture_trace, NULL
    };
    struct mock_platform platform = { 0U, 0U, 0U, 0U };
    const struct bb_platform_ops ops = {
        ticks, mmio_read, mmio_write, clean, invalidate
    };
    struct bb_modem *modem;
    struct bb_iq_block transmit;
    struct bb_iq_block receive = {
        0U, 7680000U, BB_MAX_IQ_SAMPLES, channel_samples
    };
    struct bb_buf_handle handle;
    uint8_t decoded[BB_MAX_TB_BYTES];
    size_t decoded_length = 0U;
    uint32_t index;
    {
        const struct bb_platform_ops incomplete = {
            ticks, NULL, mmio_write, clean, invalidate
        };
        if (bb_modem_init(arena, sizeof(arena), &config, &incomplete,
                          &platform, &modem) == 0) {
            (void)fprintf(stderr, "incomplete platform accepted\n");
            return 1;
        }
    }
    if (bb_modem_init(arena, sizeof(arena), &config, &ops, &platform,
                      &modem) != 0) return 1;
    if ((bb_modem_send_tb(modem, payload, sizeof(payload), 0U) != 0) ||
        (bb_modem_run_until(modem, 0U) != 0) ||
        (bb_modem_tx_acquire(modem, &transmit, &handle) != 0)) return 1;
    if (bb_channel_apply(&config, &transmit, &receive) != 0) return 1;
    if ((bb_modem_tx_complete(modem, handle) != 0) ||
        (bb_modem_tx_complete(modem, handle) == 0)) return 1;
    platform.tick = 100U;
    if ((bb_modem_rx_iq_submit(modem, &receive, 100U) != 0) ||
        (bb_modem_run_until(modem, 100U) != 0) ||
        (bb_modem_read_tb(modem, decoded, sizeof(decoded),
                          &decoded_length) != 0)) return 1;
    if ((decoded_length != sizeof(payload)) ||
        (memcmp(decoded, payload, sizeof(payload)) != 0) ||
        (platform.clean == 0U) || (platform.invalidate == 0U) ||
        (platform.ack == 0U)) return 1;
    {
        static const uint8_t tlv[] = {
            1U, 0U, 1U, 0U, 0x5AU, 0U, 0U, 0U
        };
        const struct bb_ipc_frame request = {
            1U, 7U, 41U, 3U, 0U, tlv, (uint32_t)sizeof(tlv)
        };
        struct bb_ipc_frame response_frame;
        uint8_t request_wire[BB_IPC_MAX_FRAME_SIZE];
        uint8_t response_wire[BB_IPC_MAX_FRAME_SIZE];
        size_t request_length = 0U;
        size_t response_length = 0U;
        if ((bb_ipc_encode(&request, request_wire, sizeof(request_wire),
                           &request_length) != 0) ||
            (bb_modem_ipc_submit(modem, request_wire, request_length) != 0) ||
            (bb_modem_run_until(modem, 100U) != 0) ||
            (bb_modem_ipc_receive(modem, response_wire,
                                  sizeof(response_wire),
                                  &response_length) != 0) ||
            (bb_ipc_decode(response_wire, response_length,
                           &response_frame) != 0) ||
            (response_frame.sequence != request.sequence) ||
            (response_frame.epoch != request.epoch) ||
            ((response_frame.flags & BB_IPC_FLAG_RESPONSE) == 0U) ||
            ((response_frame.type & UINT16_C(0x8000)) == 0U)) {
            (void)fprintf(stderr, "IPC request/response ring failed\n");
            return 1;
        }
        if ((bb_modem_ipc_submit(modem, request_wire, request_length) != 0) ||
            (bb_modem_run_until(modem, 100U) != 0) ||
            (bb_modem_ipc_receive(modem, response_wire,
                                  sizeof(response_wire),
                                  &response_length) == 0)) {
            (void)fprintf(stderr, "IPC tuple replay guard failed\n");
            return 1;
        }
    }
    {
        struct bb_task_stack_status stack_status;
        if ((bb_modem_get_task_stack_status(modem, 0U, &stack_status) != 0) ||
            !stack_status.guards_ok || (stack_status.high_water_bytes == 0U) ||
            (stack_status.high_water_bytes > stack_status.budget_bytes)) {
            (void)fprintf(stderr, "task stack high-water failed\n");
            return 1;
        }
    }
    {
        const struct bb_event earlier = {
            150U, 1U, 1U, { 0U, 0U, 0U }
        };
        const struct bb_event later_high_priority = {
            180U, 8U, 3U, { UINT16_MAX, 1U, 4U }
        };
        dispatched_count = 0U;
        if ((bb_modem_post(modem, &earlier) != 0) ||
            (bb_modem_post(modem, &later_high_priority) != 0) ||
            (bb_modem_run_until(modem, 180U) != 0) ||
            (dispatched_count < 2U) ||
            (dispatched_ticks[0] != 150U) ||
            (dispatched_ticks[1] != 180U)) {
            (void)fprintf(stderr, "virtual-time ordering failed\n");
            return 1;
        }
    }
    for (index = 0U; index < 64U; ++index) {
        struct bb_event event = { 200U, 1U, 1U, { 0U, 0U, 0U } };
        if (bb_modem_post(modem, &event) != 0) return 1;
    }
    {
        struct bb_event overflow = { 200U, 1U, 1U, { 0U, 0U, 0U } };
        if (bb_modem_post(modem, &overflow) == 0) return 1;
    }
    if ((bb_modem_run_until(modem, 200U) != 0) ||
        (bb_modem_restart_domain(modem, 2U, 300U) != 0)) return 1;
    (void)printf("runtime ownership/queue tests passed\n");
    return 0;
}
