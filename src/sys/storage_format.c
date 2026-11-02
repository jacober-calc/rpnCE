#include <string.h>
#include "storage_internal.h"

storage_workspace_t storage_workspace;

uint32_t storage_hash(uint32_t h, const void *data, size_t n)
{
    const uint8_t *p = data;
    while (n--) { h ^= *p++; h *= 16777619UL; }
    return h;
}

uint32_t storage_get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

uint32_t storage_precision_hash(const storage_stats_t *stats, const uint8_t *precision)
{
    return storage_hash(storage_hash(2166136261UL, stats, sizeof *stats),
                        precision, PRECISION_SIZE - 4);
}

bool storage_precision_valid(const storage_stats_t *stats, const uint8_t *precision)
{
    return precision[0] == 'P' && precision[1] == 1 &&
        precision[PRECISION_SIZE - 5] <= 3 &&
        storage_get32(precision + PRECISION_SIZE - 4) == storage_precision_hash(stats, precision);
}

void storage_restore_math(calc_t *c, const tvm_settings_t *tvm,
                          const storage_stats_t *stats, const base_state_t *base,
                          const uint8_t *precision)
{
    c->tvm = *tvm;
    if (!c->tvm.iter_max) tvm_settings_reset(&c->tvm);
    memcpy(&c->stats, stats, sizeof *stats);
    memset(c->stats.first, 0, sizeof c->stats.first);
    c->stats.constant = 0;
    if (precision) {
        memcpy(c->stats.first, precision + 2, sizeof c->stats.first);
        c->stats.constant = precision[PRECISION_SIZE - 5];
    }
    if (c->stats.model >= FIT_COUNT) c->stats.model = FIT_LIN;
    c->base = *base;
    base_validate(&c->base);
}
