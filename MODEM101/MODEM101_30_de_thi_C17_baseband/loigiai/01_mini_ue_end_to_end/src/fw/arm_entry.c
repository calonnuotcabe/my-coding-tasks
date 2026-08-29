#include "bb/bb.h"

#include <stddef.h>
#include <stdint.h>

#define ARM_MODEM_ARENA_BYTES (1024U * 1024U)

_Alignas(max_align_t) static uint8_t firmware_arena[ARM_MODEM_ARENA_BYTES];
static uint64_t firmware_tick;
static struct bb_modem *firmware_modem;

_Noreturn void bb_firmware_main(void);
void bb_arch_irq_dispatch(void);
void bb_arch_enable_irqs(void);
void bb_arch_disable_irqs(void);
void bb_arch_cache_clean_range(const void *address, size_t length);
void bb_arch_cache_invalidate_range(void *address, size_t length);

static uint64_t arm_ticks(void *context)
{
    (void)context;
    return firmware_tick;
}

static uint32_t arm_mmio_read32(void *context, uintptr_t address)
{
    volatile const uint32_t *const reg = (volatile const uint32_t *)address;
    (void)context;
    return *reg;
}

static void arm_mmio_write32(void *context, uintptr_t address, uint32_t value)
{
    volatile uint32_t *const reg = (volatile uint32_t *)address;
    (void)context;
    *reg = value;
}

static void arm_cache_clean(void *context, const void *address, size_t length)
{
    (void)context;
    bb_arch_cache_clean_range(address, length);
}

static void arm_cache_invalidate(void *context, void *address, size_t length)
{
    (void)context;
    bb_arch_cache_invalidate_range(address, length);
}

void bb_arch_irq_dispatch(void)
{
    static const uint8_t irq_priority_order[] = {
        8U, 9U, 10U, 11U, 12U, 13U, 16U, 1U, 2U
    };
    const uint32_t pending = arm_mmio_read32(NULL, 0x60000010U);
    uint32_t index;
    if (firmware_modem == NULL) {
        return;
    }
    for (index = 0U;
         index < sizeof(irq_priority_order) / sizeof(irq_priority_order[0]);
         ++index) {
        const uint32_t mask = 1U << irq_priority_order[index];
        if ((pending & mask) != 0U) {
            arm_mmio_write32(NULL, 0x60000014U, mask);
            (void)bb_modem_irq(firmware_modem, irq_priority_order[index]);
        }
    }
}

_Noreturn void bb_firmware_main(void)
{
    const struct bb_config config = {
        7680000U, 1U, 0, 0U, 42U, 0U,
        (uint8_t)BB_MOD_QPSK, 0U, NULL, NULL
    };
    const struct bb_platform_ops platform = {
        arm_ticks, arm_mmio_read32, arm_mmio_write32,
        arm_cache_clean, arm_cache_invalidate
    };
    struct bb_modem *modem = NULL;
    const int initialized =
        bb_modem_init(firmware_arena, sizeof(firmware_arena), &config,
                      &platform, NULL, &modem);
    if (initialized == 0) {
        firmware_modem = modem;
        bb_arch_enable_irqs();
    }
    for (;;) {
        if (initialized == 0) {
            uint64_t dispatch_tick;
            /* arm_ticks() is also called from IRQ context.  Protect the
             * 64-bit update on this 32-bit core from a torn ISR read. */
            bb_arch_disable_irqs();
            firmware_tick += BB_SLOT_TICKS;
            dispatch_tick = firmware_tick;
            bb_arch_enable_irqs();
            (void)bb_modem_run_until(modem, dispatch_tick);
        }
    }
}
