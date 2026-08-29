#include "internal.h"

void bb_trace(const struct bb_config *cfg, uint64_t tick,
              const char *checkpoint, int32_t value)
{
    if ((cfg != NULL) && (cfg->trace != NULL)) {
        cfg->trace(cfg->trace_ctx, tick, checkpoint, value);
    }
}

