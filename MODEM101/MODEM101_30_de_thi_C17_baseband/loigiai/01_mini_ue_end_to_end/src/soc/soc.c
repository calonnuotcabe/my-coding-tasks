#include "internal.h"

#include <stdint.h>

size_t bb_soc_required_memory(const struct bb_config *cfg)
{
    (void)cfg;
    return sizeof(struct bb_soc) + _Alignof(struct bb_soc) - 1U;
}

int bb_soc_init(void *arena, size_t arena_len, const struct bb_config *cfg,
                const struct bb_rf_backend *rf, void *rf_ctx,
                struct bb_soc **out)
{
    uintptr_t base;
    uintptr_t aligned;
    size_t skipped;
    struct bb_soc *soc;

    if ((arena == NULL) || (cfg == NULL) || (rf == NULL) || (out == NULL) ||
        (rf->tx_submit == NULL) || (rf->rx_acquire == NULL) ||
        (rf->rx_release == NULL) ||
        (arena_len < bb_soc_required_memory(cfg))) {
        return -1;
    }
    base = (uintptr_t)arena;
    aligned = (base + _Alignof(struct bb_soc) - 1U) &
              ~((uintptr_t)_Alignof(struct bb_soc) - 1U);
    skipped = (size_t)(aligned - base);
    if ((skipped > arena_len) ||
        ((arena_len - skipped) < sizeof(struct bb_soc))) {
        return -1;
    }
    soc = (struct bb_soc *)aligned;
    soc->cfg = *cfg;
    soc->rf = rf;
    soc->rf_ctx = rf_ctx;
    soc->last_tick = 0U;
    {
        uint32_t device;
        uint32_t reg;
        for (device = 0U; device < 7U; ++device) {
            for (reg = 0U; reg < 8U; ++reg) {
                soc->device_registers[device][reg] = 0U;
            }
        }
    }
    *out = soc;
    return 0;
}

static int mmio_location(uintptr_t address, uint32_t *device,
                         uint32_t *reg)
{
    static const uintptr_t bases[7] = {
        0x60000000U, 0x60001000U, 0x60002000U, 0x60004000U,
        0x60005000U, 0x60007000U, 0x60008000U
    };
    uint32_t index;
    if (((address & 3U) != 0U) || (device == NULL) || (reg == NULL)) {
        return -1;
    }
    for (index = 0U; index < 7U; ++index) {
        if ((address >= bases[index]) && (address < bases[index] + 0x20U)) {
            *device = index;
            *reg = (uint32_t)((address - bases[index]) / 4U);
            return 0;
        }
    }
    return -1;
}

int bb_soc_mmio_read32(struct bb_soc *soc, uintptr_t address, uint32_t *value)
{
    uint32_t device;
    uint32_t reg;
    if ((soc == NULL) || (value == NULL) ||
        (mmio_location(address, &device, &reg) != 0)) {
        return -1;
    }
    *value = soc->device_registers[device][reg];
    return 0;
}

int bb_soc_mmio_write32(struct bb_soc *soc, uintptr_t address, uint32_t value)
{
    uint32_t device;
    uint32_t reg;
    if ((soc == NULL) || (mmio_location(address, &device, &reg) != 0)) {
        return -1;
    }
    if (reg == 0U) {
        if ((value & 2U) != 0U) {
            uint32_t index;
            for (index = 0U; index < 8U; ++index) {
                soc->device_registers[device][index] = 0U;
            }
            return 0;
        }
        soc->device_registers[device][0] = value & 7U;
        if (((value & 1U) != 0U) && ((value & 4U) != 0U)) {
            if (device == 5U) {
                /* Crypto stays BUSY until the descriptor engine step has
                 * produced a result and raised IRQ 13. */
                soc->device_registers[device][1] = 1U;
            } else {
                soc->device_registers[device][1] = 2U;
                soc->device_registers[device][4] |= 1U;
            }
        }
    } else if (reg == 5U) {
        soc->device_registers[device][4] &= ~value;
    } else if (reg < 8U) {
        soc->device_registers[device][reg] = value;
    }
    return 0;
}

int bb_soc_step(struct bb_soc *soc, struct bb_modem *fw, uint64_t tick)
{
    struct bb_iq_block block;
    int acquired;
    if ((soc == NULL) || (fw == NULL) || bb_tick_before(tick, soc->last_tick)) {
        return -1;
    }
    soc->last_tick = tick;
    /* Advance timer/task state first so an RF arrival at tick cannot appear in
     * the trace before an earlier slot boundary. */
    if (bb_modem_run_until(fw, tick) != 0) {
        return -1;
    }
    acquired = soc->rf->rx_acquire(soc->rf_ctx, tick, &block);
    if (acquired == 0) {
        soc->device_registers[3][4] |= 1U;
        const int submitted = bb_modem_rx_iq_submit(fw, &block, tick);
        soc->rf->rx_release(soc->rf_ctx, &block);
        if (submitted != 0) {
            return -2;
        }
    } else if (acquired < -1) {
        return -2;
    }
    if (bb_modem_run_until(fw, tick) != 0) {
        return -1;
    }
    {
        const int crypto_completed = bb_crypto_accelerator_step(fw);
        if (crypto_completed < 0) {
            return -5;
        }
        if (crypto_completed > 0) {
            soc->device_registers[5][1] = 2U;
            soc->device_registers[5][4] |= 1U;
            if ((bb_modem_irq(fw, 13U) != 0) ||
                (bb_modem_run_until(fw, tick) != 0)) {
                return -5;
            }
        }
    }
    {
        struct bb_iq_block transmit;
        struct bb_buf_handle handle;
        while (bb_modem_tx_acquire(fw, &transmit, &handle) == 0) {
            if (soc->rf->tx_submit(soc->rf_ctx, &transmit) != 0) {
                return -3;
            }
            if (bb_modem_tx_complete(fw, handle) != 0) {
                return -4;
            }
        }
    }
    return 0;
}
