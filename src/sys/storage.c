#include <string.h>
#include <stddef.h>
#include <fileioc.h>
#include "storage.h"
#include "../core/format.h"

#define APPVAR_NAME "RPNCEdat"
#define STATE_MAGIC 0x52
#define STATE_VERSION 6

/* Values, including units, have one representation in every current save.
 * The optional precision footer is independently checked: damaged witnesses
 * must not discard otherwise usable historical statistics sums. */
typedef struct {
    real_t sum[SUM_COUNT];
    bool linear_only, lnx_ok, lny_ok;
    uint8_t model;
} saved_stats_t;
_Static_assert(offsetof(stats_t, first) == sizeof(saved_stats_t), "statistics prefix changed");
typedef struct {
    uint8_t magic, version, depth;
    value_t stack[STACK_MAX], lastx, regs[REG_BASE_COUNT];
    uint8_t extra_count, lift, degrees, complex_results, complex_mode, disp_style;
    int8_t disp_digits;
    tvm_settings_t tvm;
    saved_stats_t stats;
    base_state_t base;
    value_t extra[REG_EXTRA_MAX];
} saved_state_t;
#define SAVED_FIXED_SIZE offsetof(saved_state_t, extra)
#define PRECISION_SIZE (2 + 2 * sizeof(real_t) + 1 + 4)
static saved_state_t s;
static uint8_t precision[PRECISION_SIZE];

bool storage_import_v5(calc_t *c);

static uint32_t precision_hash(void)
{
    uint32_t h = 2166136261UL;
    const uint8_t *p = (const uint8_t *)&s.stats;
    for (size_t i = 0; i < sizeof s.stats; i++) { h ^= p[i]; h *= 16777619UL; }
    for (size_t i = 0; i < PRECISION_SIZE - 4; i++) { h ^= precision[i]; h *= 16777619UL; }
    return h;
}
static void precision_checksum(uint8_t out[4])
{
    uint32_t h = precision_hash();
    for (int i = 0; i < 4; i++) out[i] = (uint8_t)(h >> (8 * i));
}

void storage_save(calc_t *c)
{
    calc_finish_entry(c);
    if (c->editing) return;
    memset(&s, 0, sizeof s);
    s.magic = STATE_MAGIC; s.version = STATE_VERSION;
    s.depth = c->depth; s.lastx = c->lastx;
    memcpy(s.stack, c->stack, sizeof s.stack);
    memcpy(s.regs, c->regs, sizeof s.regs);
    s.extra_count = c->extra_count;
    memcpy(s.extra, c->extra, sizeof(value_t) * c->extra_count);
    s.lift = c->lift; s.degrees = c->degrees;
    s.complex_results = c->complex_results; s.complex_mode = c->complex_mode;
    s.disp_style = c->disp_style; s.disp_digits = c->disp_digits;
    s.tvm = c->tvm; s.base = c->base;
    memcpy(&s.stats, &c->stats, sizeof s.stats);
    precision[0] = 'P'; precision[1] = 1;
    memcpy(precision + 2, c->stats.first, sizeof c->stats.first);
    precision[2 + sizeof c->stats.first] = c->stats.constant;
    precision_checksum(precision + PRECISION_SIZE - 4);
    uint8_t h = ti_Open(APPVAR_NAME, "w");
    if (!h) return;
    size_t n = SAVED_FIXED_SIZE + sizeof(value_t) * c->extra_count;
    bool saved = ti_Write(&s, 1, n, h) == n && ti_Write(precision, 1, PRECISION_SIZE, h) == PRECISION_SIZE;
    ti_SetArchiveStatus(true, h);
    ti_Close(h);
    if (saved) ti_Delete("RPNCEunt"); /* superseded by each value's own unit */
}

bool storage_load(calc_t *c)
{
    uint8_t h = ti_Open(APPVAR_NAME, "r");
    if (!h) return false;
    uint8_t header[2];
    bool header_ok = ti_Read(header, 1, 2, h) == 2 && header[0] == STATE_MAGIC;
    ti_Close(h);
    if (!header_ok) return false;
    if (header[1] == 5) return storage_import_v5(c);
    if (header[1] != STATE_VERSION) return false;
    h = ti_Open(APPVAR_NAME, "r");
    if (!h) return false;
    bool valid = ti_Read(&s, 1, SAVED_FIXED_SIZE, h) == SAVED_FIXED_SIZE &&
                 s.depth >= STACK_MIN && s.depth <= STACK_MAX && s.extra_count <= REG_EXTRA_MAX;
    if (valid) {
        size_t tail = sizeof(value_t) * s.extra_count;
        valid = ti_Read(s.extra, 1, tail, h) == tail;
    }
    bool have_precision = false;
    if (valid && ti_Read(precision, 1, PRECISION_SIZE, h) == PRECISION_SIZE) {
        uint8_t checksum[4];
        precision_checksum(checksum);
        have_precision = precision[0] == 'P' && precision[1] == 1 &&
            precision[PRECISION_SIZE - 5] <= 3 && !memcmp(checksum, precision + PRECISION_SIZE - 4, 4);
    }
    ti_Close(h);
    if (!valid) return false;
    for (int i = 0; i < STACK_MAX; i++) if (!value_valid(&s.stack[i])) return false;
    if (!value_valid(&s.lastx)) return false;
    for (int i = 0; i < REG_BASE_COUNT; i++) if (!value_valid(&s.regs[i])) return false;
    for (int i = 0; i < s.extra_count; i++) if (!value_valid(&s.extra[i])) return false;
    c->depth = s.depth; c->lastx = s.lastx;
    memcpy(c->stack, s.stack, sizeof c->stack);
    memcpy(c->regs, s.regs, sizeof c->regs);
    c->extra_count = s.extra_count;
    for (int i = 0; i < REG_EXTRA_MAX; i++)
        c->extra[i] = i < s.extra_count ? s.extra[i] : value_real(os_Int24ToReal(0));
    c->lift = s.lift != 0; c->degrees = s.degrees != 0;
    c->complex_results = s.complex_results != 0;
    c->complex_mode = s.complex_mode <= COMPLEX_DEG ? s.complex_mode : COMPLEX_RECT;
    if (s.disp_style >= DISP_FIX && s.disp_style <= DISP_ENG &&
        s.disp_digits >= DISP_FLOAT && s.disp_digits <= DISP_MAX_DIGITS) {
        c->disp_style = s.disp_style; c->disp_digits = s.disp_digits;
    }
    c->tvm = s.tvm;
    if (!c->tvm.iter_max) tvm_settings_reset(&c->tvm);
    memcpy(&c->stats, &s.stats, sizeof s.stats);
    memset(c->stats.first, 0, sizeof c->stats.first);
    c->stats.constant = 0;
    if (have_precision) {
        memcpy(c->stats.first, precision + 2, sizeof c->stats.first);
        c->stats.constant = precision[PRECISION_SIZE - 5];
    }
    if (c->stats.model >= FIT_COUNT) c->stats.model = FIT_LIN;
    c->base = s.base; base_validate(&c->base);
    return true;
}

/* ---------- settings ---------- */


#define SETTINGS_NAME "RPNCEcfg"

typedef struct {
    uint8_t magic;
    uint8_t version;        /* 1: tint; 2: DATE; 3: appended decimal preference */
    uint8_t tint;
    uint8_t date_as_string;
    uint8_t epoch;
    dt_obj_t custom_epoch;
    int8_t app_tz, clock_tz;
} settings_t;

void storage_save_settings(uint8_t tint, const calc_t *c)
{
    settings_t s;
    memset(&s, 0, sizeof s);
    s.magic = STATE_MAGIC;
    s.version = 3;
    s.tint = tint;
    s.date_as_string = c->date_as_string;
    s.epoch = c->epoch;
    s.custom_epoch = c->custom_epoch;
    s.app_tz = c->app_tz;
    s.clock_tz = c->clock_tz;
    uint8_t h = ti_Open(SETTINGS_NAME, "w");
    if (!h)
        return;
    ti_Write(&s, sizeof s, 1, h);
    /* The decimal preference follows the fixed settings prefix. */
    uint8_t decimal_comma = c->decimal_comma;
    ti_Write(&decimal_comma, 1, 1, h);
    ti_SetArchiveStatus(true, h);
    ti_Close(h);
}

uint8_t storage_load_settings(calc_t *c)
{
    settings_t s;
    uint8_t h = ti_Open(SETTINGS_NAME, "r");
    if (!h)
        return 0;
    size_t n = ti_Read(&s, 1, sizeof s, h);
    uint8_t decimal_comma = 0;
    if (n == sizeof s && s.version >= 3)
        ti_Read(&decimal_comma, 1, 1, h);
    ti_Close(h);
    if (n < 3 || s.magic != STATE_MAGIC)
        return 0;
    c->decimal_comma = decimal_comma == 1;
    if (s.version >= 2 && n >= sizeof s) {
        c->date_as_string = s.date_as_string != 0;
        if (s.epoch <= EPOCH_CUSTOM)
            c->epoch = s.epoch;
        if (s.custom_epoch.type == OBJ_DATE && dt_validate(&s.custom_epoch) == DT_OK)
            c->custom_epoch = s.custom_epoch;
        if (s.app_tz >= -95 && s.app_tz <= 95)
            c->app_tz = s.app_tz;
        if (s.clock_tz >= -95 && s.clock_tz <= 95)
            c->clock_tz = s.clock_tz;
    }
    return s.tint;
}
