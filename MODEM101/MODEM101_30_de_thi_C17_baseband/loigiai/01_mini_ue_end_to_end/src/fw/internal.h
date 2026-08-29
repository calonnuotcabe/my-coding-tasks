#ifndef BB_INTERNAL_H
#define BB_INTERNAL_H

#include "bb/bb.h"
#include "bb/ipc.h"
#include "bb/mac.h"
#include "bb/protocol.h"
#include "bb/soc.h"

#include <stdatomic.h>

#define BB_EVENT_RING_STORAGE 128U
#define BB_TASK_QUEUE_COUNT 8U
#define BB_POOL_SLOTS 16U
#define BB_POOL_BYTES 1024U
#define BB_DMA_SLOTS 4U
#define BB_HARQ_PROCESSES 8U
#define BB_SCHEDULED_TASKS 10U
#define BB_STACK_CANARY 0xBADC0FFEU
#define BB_RUN_WORK_BUDGET 100000U
#define BB_HARQ_MAX_TRANSMISSIONS 4U
#define BB_MAX_FRAME_BITS ((BB_MAX_TB_BYTES + 6U) * 8U)
#define BB_REORDER_WINDOW 64U
#define BB_REORDER_TIMEOUT_TICKS UINT64_C(1500)
#define BB_CRYPTO_JOB_SLOTS 4U
#define BB_TASK_STACK_STORAGE_BYTES 16384U
#define BB_STACK_FILL 0xA5U
#define BB_IPC_RING_SLOTS 4U

enum bb_internal_event_type {
    BB_EVENT_EXTERNAL = 1,
    BB_EVENT_TX_REQUEST = 2,
    BB_EVENT_RX_DEFERRED = 3,
    BB_EVENT_MAC_RX = 4,
    BB_EVENT_RLC_RX = 5,
    BB_EVENT_PDCP_RX = 6,
    BB_EVENT_CONTROL_RX = 7,
    BB_EVENT_NAS_RX = 8,
    BB_EVENT_CRYPTO_COMPLETE = 9,
    BB_EVENT_IPC_REQUEST = 10
};

enum bb_control_action {
    BB_ACTION_CELL_SEARCH = 1,
    BB_ACTION_CELL_CAMP,
    BB_ACTION_RRC_CONNECT_START,
    BB_ACTION_RRC_CONNECT_COMPLETE,
    BB_ACTION_LINK_RECOVERY_START,
    BB_ACTION_NAS_REGISTRATION_START,
    BB_ACTION_NAS_REGISTRATION_ACCEPT,
    BB_ACTION_NAS_SESSION_ACTIVATE
};

enum bb_dma_state {
    BB_DMA_FREE = 0,
    BB_DMA_CPU_OWNED,
    BB_DMA_DMA_OWNED,
    BB_DMA_DONE
};

struct bb_event_ring {
    _Atomic uint32_t write_index;
    _Atomic uint32_t read_index;
    uint32_t capacity;
    struct bb_event entries[BB_EVENT_RING_STORAGE];
};

struct bb_ipc_storage_slot {
    uint16_t length;
    uint8_t bytes[BB_IPC_MAX_FRAME_SIZE];
};

struct bb_ipc_storage_ring {
    _Atomic uint32_t write_index;
    _Atomic uint32_t read_index;
    struct bb_ipc_storage_slot slots[BB_IPC_RING_SLOTS];
};

struct bb_pool_slot {
    uint16_t generation;
    uint16_t state;
    uint32_t length;
    uint8_t data[BB_POOL_BYTES];
};

struct bb_pool { struct bb_pool_slot slots[BB_POOL_SLOTS]; };

enum bb_dma_direction { BB_DMA_RX = 0, BB_DMA_TX = 1 };

struct bb_dma_slot {
    uint16_t generation;
    uint8_t state;
    uint8_t direction;
    uint32_t count;
    uint64_t t0;
    uint32_t sample_rate_hz;
    _Alignas(64) uint8_t descriptor[BB_DMA_DESCRIPTOR_SIZE];
    _Alignas(64) struct bb_cpx16 samples[BB_MAX_IQ_SAMPLES];
};

enum bb_harq_state { BB_HARQ_IDLE = 0, BB_HARQ_WAITING };

struct bb_harq_process {
    uint8_t state;
    uint8_t transmissions;
    uint16_t timer_generation;
    uint64_t expiry;
    uint32_t pdcp_count;
    int16_t soft_llr[BB_MAX_FRAME_BITS];
    uint16_t soft_length;
};

struct bb_rlc_reorder_slot {
    bool valid;
    uint16_t sequence;
    uint16_t length;
    uint8_t data[BB_MAX_TB_BYTES];
};

struct bb_pdcp_reorder_slot {
    bool valid;
    uint8_t harq_id;
    uint16_t offset;
    uint16_t total_length;
    uint16_t payload_length;
    uint32_t count;
    uint8_t payload[BB_WIRE_MAX_PAYLOAD];
};

enum bb_crypto_job_state {
    BB_CRYPTO_JOB_IDLE = 0,
    BB_CRYPTO_JOB_SUBMITTED,
    BB_CRYPTO_JOB_COMPLETED
};

enum bb_crypto_operation {
    BB_CRYPTO_VERIFY_MAC = 1,
    BB_CRYPTO_SIGN_MAC = 2
};

struct bb_crypto_job {
    uint8_t state;
    uint8_t require_mask;
    uint8_t operation;
    uint8_t reserved;
    uint16_t generation;
    uint16_t cookie;
    uint16_t key_slot;
    uint16_t length;
    int32_t result;
    uint64_t transmit_tick;
    _Alignas(64) uint8_t descriptor[BB_DMA_DESCRIPTOR_SIZE];
    _Alignas(64) uint8_t input[BB_MAX_TB_BYTES];
    _Alignas(64) uint8_t output[BB_MAX_TB_BYTES];
};

struct bb_stack_context {
    enum bb_rrc_state rrc;
    enum bb_nas_state nas;
    bool security_active;
    bool recovering;
    uint16_t key_slot;
    uint16_t rlc_rx_next;
    uint64_t rlc_bitmap;
    uint64_t rlc_reorder_expiry;
    uint16_t rlc_timer_generation;
    bool rlc_reorder_active;
    struct bb_rlc_reorder_slot rlc_slots[BB_REORDER_WINDOW];
    uint32_t pdcp_rx_next;
    uint64_t pdcp_bitmap;
    uint64_t pdcp_reorder_expiry;
    uint16_t pdcp_timer_generation;
    bool pdcp_initialized;
    bool pdcp_reorder_active;
    struct bb_pdcp_reorder_slot pdcp_slots[BB_REORDER_WINDOW];
    uint32_t delivered;
    uint32_t replay_drops;
    uint32_t harq_retx;
    uint32_t harq_combines;
    uint32_t rlc_reorders;
    uint32_t pdcp_reorders;
    uint32_t recoveries;
    uint32_t bsr_bytes;
    uint8_t active_harq_id;
    bool active_harq_valid;
    uint8_t reassembly[BB_MAX_SDU_BYTES];
    uint8_t received_map[BB_MAX_SDU_BYTES];
    struct bb_harq_process harq[BB_HARQ_PROCESSES];
};

struct bb_task_info {
    uint16_t priority;
    uint16_t reserved;
    uint32_t budget_bytes;
    uint32_t high_water_bytes;
    uint32_t guard_low;
    uint32_t guard_high;
};

struct bb_task_stack_storage {
    _Alignas(16) uint8_t alignment_padding[12];
    uint32_t guard_low;
    uint8_t bytes[BB_TASK_STACK_STORAGE_BYTES];
    uint32_t guard_high;
};

struct bb_modem {
    struct bb_config cfg;
    struct bb_platform_ops plat;
    void *plat_ctx;
    uint64_t now;
    uint64_t next_slot;
    uint64_t last_post_tick;
    bool last_post_valid;
    struct bb_stats stats;
    struct bb_event_ring irq_ring;
    struct bb_event_ring task_queues[BB_TASK_QUEUE_COUNT];
    struct bb_pool pool;
    struct bb_dma_slot dma[BB_DMA_SLOTS];
    struct bb_buf_handle pending_rx;
    bool pending_rx_valid;
    uint8_t last_tb[BB_MAX_TB_BYTES];
    uint32_t last_tb_length;
    bool last_tb_ready;
    struct bb_stack_context stack;
    struct bb_crypto_job crypto_jobs[BB_CRYPTO_JOB_SLOTS];
    uint16_t crypto_cookie;
    struct bb_ipc_storage_ring ipc_requests;
    struct bb_ipc_storage_ring ipc_responses;
    uint32_t ipc_last_sequence;
    uint32_t ipc_last_epoch;
    bool ipc_sequence_initialized;
    struct bb_task_info task_info[BB_SCHEDULED_TASKS];
    struct bb_task_stack_storage task_stacks[BB_SCHEDULED_TASKS];
    bool supervisor_fault;
};

struct bb_soc {
    struct bb_config cfg;
    const struct bb_rf_backend *rf;
    void *rf_ctx;
    uint64_t last_tick;
    uint32_t device_registers[7][8];
};

/* Modular clock comparisons are valid while deadlines are less than 2^63
 * ticks apart.  Keeping the operation unsigned avoids implementation-defined
 * uint64_t-to-int64_t conversions on the freestanding target. */
static inline bool bb_tick_before(uint64_t left, uint64_t right)
{
    return (left != right) &&
           (((left - right) & (UINT64_C(1) << 63U)) != 0U);
}

static inline bool bb_tick_due(uint64_t now, uint64_t deadline)
{
    return !bb_tick_before(now, deadline);
}

int bb_ring_push(struct bb_event_ring *ring, const struct bb_event *ev);
int bb_ring_pop(struct bb_event_ring *ring, struct bb_event *ev);
int bb_ring_peek(struct bb_event_ring *ring, struct bb_event *ev);
void bb_pool_init(struct bb_pool *pool);
int bb_pool_alloc(struct bb_pool *pool, uint32_t length,
                  struct bb_buf_handle *handle, uint8_t **data);
int bb_pool_get(struct bb_pool *pool, struct bb_buf_handle handle,
                uint8_t **data);
int bb_pool_release(struct bb_pool *pool, struct bb_buf_handle handle);
void bb_trace(const struct bb_config *cfg, uint64_t tick,
              const char *checkpoint, int32_t value);
int16_t bb_sat16(int32_t value);
int bb_phy_receive_soft(const struct bb_config *cfg,
                        const struct bb_iq_block *input,
                        uint8_t *tb, size_t capacity, size_t *tb_len,
                        const int16_t *prior_llr, size_t prior_length,
                        int16_t *output_llr, size_t output_capacity,
                        size_t *output_length);
int bb_phy_receive_soft_meta(const struct bb_config *cfg,
                             const struct bb_iq_block *input,
                             uint8_t *tb, size_t capacity, size_t *tb_len,
                             const int16_t *prior_llr, size_t prior_length,
                             int16_t *output_llr, size_t output_capacity,
                             size_t *output_length, int32_t *harq_id,
                             bool emit_trace);
void bb_qpsk_map(uint8_t first, uint8_t second, struct bb_cpx16 *symbol);
void bb_qpsk_demap(struct bb_cpx16 symbol, uint8_t *first, uint8_t *second);
void bb_qam16_map(uint8_t bits, struct bb_cpx16 *symbol);
uint8_t bb_qam16_demap(struct bb_cpx16 symbol);
void bb_oscillator_init(int32_t frequency_hz, uint32_t sample_rate_hz,
                        int16_t *step_i, int16_t *step_q);
struct bb_cpx16 bb_oscillator_step(struct bb_cpx16 current,
                                   int16_t step_i, int16_t step_q);
int bb_stack_process_pdu(struct bb_modem *modem, const uint8_t *data,
                         size_t length, uint64_t tick);
int bb_stack_process_stage(struct bb_modem *modem, const uint8_t *data,
                           size_t length, uint64_t tick, uint16_t stage);
void bb_stack_check_timers(struct bb_modem *modem, uint64_t tick);
void bb_stack_init(struct bb_stack_context *stack);
int bb_mac_send_control(struct bb_modem *modem, uint8_t type,
                        uint8_t harq_id, uint32_t count, uint64_t tick);
int bb_mac_process_tb(struct bb_modem *modem, const uint8_t *data,
                      size_t length, uint64_t tick);
int bb_mac_process_tb_verified(struct bb_modem *modem, const uint8_t *data,
                               size_t length, uint64_t tick);
void bb_mac_harq_accept(struct bb_modem *modem,
                        const struct bb_wire_pdu *pdu, uint64_t tick);
int bb_rlc_process(struct bb_modem *modem, const uint8_t *data, size_t length,
                   uint16_t sequence, uint64_t tick);
void bb_rlc_drain(struct bb_modem *modem, uint64_t tick);
void bb_rlc_check_timer(struct bb_modem *modem, uint64_t tick);
int bb_pdcp_decode_and_process(struct bb_modem *modem, const uint8_t *data,
                               size_t length, uint64_t tick);
uint32_t bb_pdcp_reconstruct_count(const struct bb_stack_context *stack,
                                   uint32_t sequence_number);
void bb_pdcp_check_timer(struct bb_modem *modem, uint64_t tick);
int bb_modem_enqueue_stage_copy(struct bb_modem *modem, const uint8_t *data,
                                size_t length, uint64_t tick, uint16_t stage);
int bb_control_process(struct bb_modem *modem,
                       const struct bb_wire_pdu *pdu, uint64_t tick);
int bb_security_cmac(uint16_t key_slot, const uint8_t *data, size_t length,
                     uint8_t output[16]);
bool bb_security_verify(uint16_t key_slot, const uint8_t *data, size_t length,
                        const uint8_t tag[8]);
int bb_security_nia2(uint16_t key_slot, uint32_t count, uint8_t bearer,
                     uint8_t direction, const uint8_t *message, size_t length,
                     uint8_t output[16]);
int bb_crypto_submit_mac(struct bb_modem *modem, const uint8_t *data,
                         size_t length, uint8_t require_mask, uint64_t tick);
int bb_crypto_submit_mac_sign(struct bb_modem *modem, const uint8_t *data,
                              size_t length, uint8_t require_mask,
                              uint64_t tick);
int bb_crypto_accelerator_step(struct bb_modem *modem);
bool bb_crypto_has_completion(const struct bb_modem *modem);
void bb_crypto_complete_irq(struct bb_modem *modem, uint64_t tick);
void bb_crypto_reset(struct bb_modem *modem);
bool bb_ipc_has_request(const struct bb_modem *modem);
void bb_ipc_task_process(struct bb_modem *modem, uint64_t tick);
int bb_protocol_validate_ies(const uint8_t *data, size_t length,
                             uint32_t *work_budget);
bool bb_protocol_type_requires_integrity(uint8_t type);
bool bb_control_rrc_allowed(enum bb_rrc_state from, uint8_t event,
                            bool guard, enum bb_control_action action,
                            enum bb_rrc_state to);
bool bb_control_nas_allowed(enum bb_nas_state from, uint8_t event,
                            bool guard, enum bb_control_action action,
                            enum bb_nas_state to);
int bb_mmio_read32(struct bb_modem *modem, uintptr_t address,
                   uint32_t *value);
int bb_mmio_write32(struct bb_modem *modem, uintptr_t address,
                    uint32_t value);

#endif
