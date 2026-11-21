/* Saved state (RPNCEdat) and settings (RPNCEcfg), both archived appvars.
 * Each value is stored complete, with its type, payload and unit. Saves
 * from rpnCE 0.7.0 and earlier (format 5) are not read. */
#include <string.h>
#include <stddef.h>
#include <fileioc.h>
#include "storage.h"
#include "../core/format.h"

#define STATE_APPVAR "RPNCEdat"
#define STATE_MAGIC 0x52
#define STATE_VERSION 6

/* The statistics as saved: the sums and modes, a fixed prefix of stats_t.
 * Its constancy witnesses follow the state in a checksummed footer. */
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
    value_t extra[REG_EXTRA_MAX];   /* only extra_count of these are saved */
} storage_state_t;
#define SAVED_FIXED_SIZE offsetof(storage_state_t, extra)

/* Footer: 'P', 1, the two witnesses, the constancy bits, then an FNV-1a
 * hash of the saved statistics and the footer so far. */
#define PRECISION_SIZE (2 + 2 * sizeof(real_t) + 1 + 4)
#define PRECISION_CONSTANT (PRECISION_SIZE - 5)
#define PRECISION_HASH (PRECISION_SIZE - 4)

#ifdef __ez80__
_Static_assert(sizeof(storage_state_t) == 2958, "version-6 disk layout changed");
#endif

/* Staging for a save or load, kept off the CE's small stack. A load reads
 * everything here first, so a damaged file never alters live values. */
static struct {
    storage_state_t state;
    uint8_t precision[PRECISION_SIZE];
} work;

static uint32_t fnv1a(uint32_t h, const void *data, size_t n)
{
    const uint8_t *p = data;
    while (n--) { h ^= *p++; h *= 16777619UL; }
    return h;
}

static uint32_t precision_hash(const storage_stats_t *stats, const uint8_t *precision)
{
    return fnv1a(fnv1a(2166136261UL, stats, sizeof *stats), precision, PRECISION_HASH);
}

static bool precision_valid(const storage_stats_t *stats, const uint8_t *p)
{
    const uint8_t *h = p + PRECISION_HASH;
    uint32_t stored = (uint32_t)h[0] | (uint32_t)h[1] << 8 |
                      (uint32_t)h[2] << 16 | (uint32_t)h[3] << 24;
    return p[0] == 'P' && p[1] == 1 && p[PRECISION_CONSTANT] <= 3 &&
           stored == precision_hash(stats, p);
}

void storage_save(calc_t *c)
{
    storage_state_t *s = &work.state;
    uint8_t *precision = work.precision;
    calc_finish_entry(c);
    if (c->editing) return;
    memset(s, 0, sizeof *s);
    s->magic = STATE_MAGIC; s->version = STATE_VERSION;
    s->depth = c->depth; s->lastx = c->lastx;
    memcpy(s->stack, c->stack, sizeof s->stack);
    memcpy(s->regs, c->regs, sizeof s->regs);
    s->extra_count = c->extra_count;
    memcpy(s->extra, c->extra, sizeof(value_t) * c->extra_count);
    s->lift = c->lift; s->degrees = c->degrees;
    s->complex_results = c->complex_results; s->complex_mode = c->complex_mode;
    s->disp_style = c->disp_style; s->disp_digits = c->disp_digits;
    s->tvm = c->tvm; s->base = c->base;
    memcpy(&s->stats, &c->stats, sizeof s->stats);

    precision[0] = 'P'; precision[1] = 1;
    memcpy(precision + 2, c->stats.first, sizeof c->stats.first);
    precision[PRECISION_CONSTANT] = c->stats.constant;
    uint32_t h = precision_hash(&s->stats, precision);
    for (int i = 0; i < 4; i++) precision[PRECISION_HASH + i] = (uint8_t)(h >> (8 * i));

    uint8_t f = ti_Open(STATE_APPVAR, "w");
    if (!f) return;
    size_t n = SAVED_FIXED_SIZE + sizeof(value_t) * c->extra_count;
    ti_Write(s, 1, n, f);
    ti_Write(precision, 1, PRECISION_SIZE, f);
    ti_SetArchiveStatus(true, f);
    ti_Close(f);
}

/* Reads the whole file into the workspace; true if its structure is sound.
 * *have_precision says whether the witness footer is present and intact. */
static bool read_state(bool *have_precision)
{
    storage_state_t *s = &work.state;
    uint8_t f = ti_Open(STATE_APPVAR, "r");
    if (!f) return false;
    bool ok = ti_Read(s, 1, SAVED_FIXED_SIZE, f) == SAVED_FIXED_SIZE &&
              s->magic == STATE_MAGIC && s->version == STATE_VERSION &&
              s->depth >= STACK_MIN && s->depth <= STACK_MAX &&
              s->extra_count <= REG_EXTRA_MAX;
    if (ok) {
        size_t tail = sizeof(value_t) * s->extra_count;
        ok = ti_Read(s->extra, 1, tail, f) == tail;
    }
    *have_precision = ok && ti_Read(work.precision, 1, PRECISION_SIZE, f) == PRECISION_SIZE &&
                      precision_valid(&s->stats, work.precision);
    ti_Close(f);
    if (!ok) return false;
    for (int i = 0; i < STACK_MAX; i++) if (!value_valid(&s->stack[i])) return false;
    if (!value_valid(&s->lastx)) return false;
    for (int i = 0; i < REG_BASE_COUNT; i++) if (!value_valid(&s->regs[i])) return false;
    for (int i = 0; i < s->extra_count; i++) if (!value_valid(&s->extra[i])) return false;
    return true;
}

bool storage_load(calc_t *c)
{
    bool have_precision;
    if (!read_state(&have_precision))
        return false;

    const storage_state_t *s = &work.state;
    c->depth = s->depth; c->lastx = s->lastx;
    memcpy(c->stack, s->stack, sizeof c->stack);
    memcpy(c->regs, s->regs, sizeof c->regs);
    c->extra_count = s->extra_count;
    for (int i = 0; i < REG_EXTRA_MAX; i++)
        c->extra[i] = i < s->extra_count ? s->extra[i] : value_real(os_Int24ToReal(0));
    c->lift = s->lift != 0; c->degrees = s->degrees != 0;
    c->complex_results = s->complex_results != 0;
    c->complex_mode = s->complex_mode <= COMPLEX_DEG ? s->complex_mode : COMPLEX_RECT;
    if (s->disp_style >= DISP_FIX && s->disp_style <= DISP_ENG &&
        s->disp_digits >= DISP_FLOAT && s->disp_digits <= DISP_MAX_DIGITS) {
        c->disp_style = s->disp_style; c->disp_digits = s->disp_digits;
    }

    c->tvm = s->tvm;
    if (!c->tvm.iter_max) tvm_settings_reset(&c->tvm);
    memcpy(&c->stats, &s->stats, sizeof s->stats);
    memset(c->stats.first, 0, sizeof c->stats.first);
    c->stats.constant = 0;          /* without witnesses, claim no constancy */
    if (have_precision) {
        memcpy(c->stats.first, work.precision + 2, sizeof c->stats.first);
        c->stats.constant = work.precision[PRECISION_CONSTANT];
    }
    if (c->stats.model >= FIT_COUNT) c->stats.model = FIT_LIN;
    c->base = s->base;
    base_validate(&c->base);
    return true;
}

/* ---------- settings ---------- */

#define SETTINGS_NAME "RPNCEcfg"
#define SETTINGS_VERSION 3          /* the layout rpnCE 0.7.0 and later write */

typedef struct {
    uint8_t magic;
    uint8_t version;
    uint8_t tint;
    uint8_t date_as_string;
    uint8_t epoch;
    dt_obj_t custom_epoch;
    int8_t app_tz, clock_tz;
    uint8_t decimal_comma;
} settings_t;
/* The file ends with the decimal preference; any padding after it is not
 * written. rpnCE 0.7.0 and 0.8.0 write the same 21 bytes on the CE. */
#define SETTINGS_SIZE (offsetof(settings_t, decimal_comma) + 1)
#ifdef __ez80__
_Static_assert(SETTINGS_SIZE == 21, "settings disk layout changed");
#endif

void storage_save_settings(uint8_t tint, const calc_t *c)
{
    settings_t s;
    memset(&s, 0, sizeof s);
    s.magic = STATE_MAGIC;
    s.version = SETTINGS_VERSION;
    s.tint = tint;
    s.date_as_string = c->date_as_string;
    s.epoch = c->epoch;
    s.custom_epoch = c->custom_epoch;
    s.app_tz = c->app_tz;
    s.clock_tz = c->clock_tz;
    s.decimal_comma = c->decimal_comma;
    uint8_t f = ti_Open(SETTINGS_NAME, "w");
    if (!f)
        return;
    ti_Write(&s, 1, SETTINGS_SIZE, f);
    ti_SetArchiveStatus(true, f);
    ti_Close(f);
}

uint8_t storage_load_settings(calc_t *c)
{
    settings_t s;
    uint8_t f = ti_Open(SETTINGS_NAME, "r");
    if (!f)
        return 0;
    bool ok = ti_Read(&s, 1, SETTINGS_SIZE, f) == SETTINGS_SIZE &&
              s.magic == STATE_MAGIC && s.version == SETTINGS_VERSION;
    ti_Close(f);
    if (!ok)
        return 0;
    c->decimal_comma = s.decimal_comma == 1;
    c->date_as_string = s.date_as_string != 0;
    if (s.epoch <= EPOCH_CUSTOM)
        c->epoch = s.epoch;
    if (s.custom_epoch.type == OBJ_DATE && dt_validate(&s.custom_epoch) == DT_OK)
        c->custom_epoch = s.custom_epoch;
    if (s.app_tz >= -95 && s.app_tz <= 95)
        c->app_tz = s.app_tz;
    if (s.clock_tz >= -95 && s.clock_tz <= 95)
        c->clock_tz = s.clock_tz;
    return s.tint;
}
