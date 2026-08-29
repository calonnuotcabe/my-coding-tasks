#include "internal.h"

int bb_mmio_read32(struct bb_modem *modem, uintptr_t address,
                   uint32_t *value)
{
    if ((modem == NULL) || (value == NULL) || ((address & 3U) != 0U) ||
        (modem->plat.mmio_read32 == NULL)) {
        return -1;
    }
    *value = modem->plat.mmio_read32(modem->plat_ctx, address);
    return 0;
}

int bb_mmio_write32(struct bb_modem *modem, uintptr_t address,
                    uint32_t value)
{
    if ((modem == NULL) || ((address & 3U) != 0U) ||
        (modem->plat.mmio_write32 == NULL)) {
        return -1;
    }
    modem->plat.mmio_write32(modem->plat_ctx, address, value);
    return 0;
}
