#ifndef RPNCE_SYS_STORAGE_INTERNAL_H
#define RPNCE_SYS_STORAGE_INTERNAL_H

#include <stddef.h>
#include "storage.h"

#define STATE_APPVAR "RPNCEdat"
#define STATE_MAGIC 0x52
#define STATE_VERSION 6
#define PRECISION_SIZE (2 + 2 * sizeof(real_t) + 1 + 4)
#define LEGACY_UNITS_MAX (REG_BASE_COUNT + REG_EXTRA_MAX)
#define STORAGE_HEADER_SIZE 2   /* magic and version, in every format */

/* Frozen disk prefixes. Adding runtime statistics must not move these. */
typedef struct {
    real_t sum[SUM_COUNT];
    bool linear_only, lnx_ok, lny_ok;
    uint8_t model;
} storage_stats_t;
_Static_assert(offsetof(stats_t, first) == sizeof(storage_stats_t), "statistics prefix changed");

typedef struct {
    uint8_t magic, version, depth;
    value_t stack[STACK_MAX], lastx, regs[REG_BASE_COUNT];
    uint8_t extra_count, lift, degrees, complex_results, complex_mode, disp_style;
    int8_t disp_digits;
    tvm_settings_t tvm;
    storage_stats_t stats;
    base_state_t base;
    value_t extra[REG_EXTRA_MAX];
} storage_state_t;

typedef struct {
    uint8_t magic, version, depth;
    real_t stack[STACK_MAX];
    uint8_t unit[STACK_MAX];
    real_t lastx;
    uint8_t unit_lastx;
    real_t regs[REG_BASE_COUNT];
    uint8_t extra_count, lift, degrees, disp_style;
    int8_t disp_digits;
    tvm_settings_t tvm;
    storage_stats_t stats;
    base_state_t base;
    real_t extra[REG_EXTRA_MAX];
} storage_legacy_state_t;

/* Save/load/import never overlap. Both decoders stage their complete input
 * here, without consuming the CE's small stack or altering live values on
 * a failed read. Only one version's union member is active at a time. */
typedef union {
    struct { storage_state_t state; uint8_t precision[PRECISION_SIZE]; } current;
    struct {
        storage_legacy_state_t state;
        uint8_t units[8 + LEGACY_UNITS_MAX + 4];
        uint8_t precision[PRECISION_SIZE];
    } legacy;
} storage_workspace_t;
extern storage_workspace_t storage_workspace;

#ifdef __ez80__
_Static_assert(sizeof(storage_state_t) == 2958, "version-6 disk layout changed");
_Static_assert(sizeof(storage_legacy_state_t) == 1428, "version-5 disk layout changed");
#endif
/* Both formats start with the header, so reading it into the workspace
 * serves whichever reader the version selects. */
_Static_assert(offsetof(storage_state_t, version) == 1 &&
               offsetof(storage_legacy_state_t, version) == 1 &&
               offsetof(storage_state_t, depth) == STORAGE_HEADER_SIZE &&
               offsetof(storage_legacy_state_t, depth) == STORAGE_HEADER_SIZE,
               "saved formats must share their header");

uint32_t storage_hash(uint32_t h, const void *data, size_t n);
uint32_t storage_get32(const uint8_t *p);
uint32_t storage_precision_hash(const storage_stats_t *stats, const uint8_t *precision);
bool storage_precision_valid(const storage_stats_t *stats, const uint8_t *precision);
void storage_restore_math(calc_t *c, const tvm_settings_t *tvm,
                          const storage_stats_t *stats, const base_state_t *base,
                          const uint8_t *precision);
/* Reads a version-5 save from h, positioned after its header. */
bool storage_import_v5(calc_t *c, uint8_t h);

#endif
