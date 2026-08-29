#ifndef BB_BB_H
#define BB_BB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BB_NFFT 256U
#define BB_CP_LEN 18U
#define BB_MAX_IQ_SAMPLES 8192U
#define BB_MAX_TB_BYTES 512U
#define BB_MAX_SDU_BYTES 4096U
#define BB_SLOT_TICKS 500U

enum bb_modulation {
    BB_MOD_QPSK = 2,
    BB_MOD_16QAM = 4
};

struct bb_cpx16 { int16_t i, q; };

struct bb_iq_block {
    uint64_t t0;
    uint32_t sample_rate_hz;
    uint32_t count;
    struct bb_cpx16 *samples;
};

struct bb_buf_handle { uint16_t slot, generation; uint32_t length; };

struct bb_event {
    uint64_t tick;
    uint16_t source, type;
    struct bb_buf_handle payload;
};

struct bb_stats {
    uint64_t rx_ok, rx_crc_fail, tx_blocks, dropped, deadline_miss;
};

struct bb_rf_backend {
    int  (*tx_submit)(void *ctx, const struct bb_iq_block *block);
    int  (*rx_acquire)(void *ctx, uint64_t deadline, struct bb_iq_block *out);
    void (*rx_release)(void *ctx, struct bb_iq_block *block);
};

struct bb_platform_ops {
    uint64_t (*ticks)(void *ctx);
    uint32_t (*mmio_read32)(void *ctx, uintptr_t addr);
    void (*mmio_write32)(void *ctx, uintptr_t addr, uint32_t value);
    void (*cache_clean)(void *ctx, const void *addr, size_t len);
    void (*cache_invalidate)(void *ctx, void *addr, size_t len);
};

typedef void (*bb_trace_fn)(void *ctx, uint64_t tick,
                            const char *checkpoint, int32_t value);

struct bb_config {
    uint32_t sample_rate_hz;
    uint32_t seed;
    int32_t cfo_hz;
    uint16_t noise_amplitude;
    uint16_t pci;
    uint16_t timing_offset_samples;
    uint8_t modulation;
    uint8_t reserved;
    bb_trace_fn trace;
    void *trace_ctx;
};

enum bb_rrc_state {
    BB_RRC_OFF = 0,
    BB_RRC_SEARCHING,
    BB_RRC_CAMPED,
    BB_RRC_CONNECTING,
    BB_RRC_CONNECTED
};

enum bb_nas_state {
    BB_NAS_DEREGISTERED = 0,
    BB_NAS_REGISTERING,
    BB_NAS_REGISTERED,
    BB_NAS_SESSION_ACTIVE
};

struct bb_session_status {
    enum bb_rrc_state rrc;
    enum bb_nas_state nas;
    uint32_t delivered;
    uint32_t replay_drops;
    uint32_t harq_retx;
    uint32_t harq_combines;
    uint32_t rlc_reorders;
    uint32_t pdcp_reorders;
    uint32_t recoveries;
    bool security_active;
};

struct bb_task_stack_status {
    uint32_t budget_bytes;
    uint32_t high_water_bytes;
    bool guards_ok;
};

struct bb_modem;
struct bb_soc;

size_t bb_modem_required_memory(const struct bb_config *cfg);
int bb_modem_init(void *arena, size_t arena_len, const struct bb_config *cfg,
                  const struct bb_platform_ops *plat, void *plat_ctx,
                  struct bb_modem **out);
int bb_modem_post(struct bb_modem *m, const struct bb_event *ev);
int bb_modem_irq(struct bb_modem *m, uint16_t irq);
int bb_modem_run_until(struct bb_modem *m, uint64_t tick);
void bb_modem_get_stats(const struct bb_modem *m, struct bb_stats *out);
int bb_modem_send_tb(struct bb_modem *m, const uint8_t *tb, size_t length,
                     uint64_t tick);
int bb_modem_rx_iq_submit(struct bb_modem *m,
                          const struct bb_iq_block *block, uint64_t tick);
int bb_modem_tx_acquire(struct bb_modem *m, struct bb_iq_block *block,
                        struct bb_buf_handle *handle);
int bb_modem_tx_complete(struct bb_modem *m, struct bb_buf_handle handle);
int bb_modem_read_tb(struct bb_modem *m, uint8_t *tb, size_t capacity,
                     size_t *length);
void bb_modem_get_session_status(const struct bb_modem *m,
                                 struct bb_session_status *status);
int bb_modem_copy_delivered_sdu(const struct bb_modem *m, uint8_t *output,
                                size_t capacity, size_t *length);
int bb_modem_restart_domain(struct bb_modem *m, uint16_t domain,
                            uint64_t tick);
int bb_modem_get_task_stack_status(const struct bb_modem *m,
                                   uint32_t task_index,
                                   struct bb_task_stack_status *status);

size_t bb_soc_required_memory(const struct bb_config *cfg);
int bb_soc_init(void *arena, size_t arena_len, const struct bb_config *cfg,
                const struct bb_rf_backend *rf, void *rf_ctx,
                struct bb_soc **out);
int bb_soc_step(struct bb_soc *soc, struct bb_modem *fw, uint64_t tick);
int bb_soc_mmio_read32(struct bb_soc *soc, uintptr_t address, uint32_t *value);
int bb_soc_mmio_write32(struct bb_soc *soc, uintptr_t address, uint32_t value);

uint32_t bb_crc24a(const uint8_t *data, size_t len);
int bb_phy_transmit(const struct bb_config *cfg, const uint8_t *tb,
                    size_t tb_len, struct bb_iq_block *out);
int bb_phy_receive(const struct bb_config *cfg, const struct bb_iq_block *in,
                   uint8_t *tb, size_t capacity, size_t *tb_len);
int bb_channel_apply(const struct bb_config *cfg,
                     const struct bb_iq_block *in, struct bb_iq_block *out);
void bb_fft_fixed(struct bb_cpx16 *samples, uint32_t count, bool inverse);
void bb_qpsk_map(uint8_t first, uint8_t second, struct bb_cpx16 *symbol);
void bb_qpsk_demap(struct bb_cpx16 symbol, uint8_t *first, uint8_t *second);
void bb_qam16_map(uint8_t bits, struct bb_cpx16 *symbol);
uint8_t bb_qam16_demap(struct bb_cpx16 symbol);

#ifdef __cplusplus
}
#endif
#endif
