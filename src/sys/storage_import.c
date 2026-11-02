/* Read-only migration for version 5 saves. This module never writes the
 * retired format; all current persistence lives in storage.c. */
#include <string.h>
#include <stddef.h>
#include <fileioc.h>

#include "storage.h"
#include "../core/format.h"
#include "../core/units.h"

#define APPVAR_NAME   "RPNCEdat"
#define STATE_MAGIC   0x52   /* 'R' */
#define STATE_VERSION 5      /* bump when the saved layout changes */

/* Keep the existing state prefix readable by earlier program files. */
typedef struct {
    real_t sum[SUM_COUNT];
    bool linear_only, lnx_ok, lny_ok;
    uint8_t model;
} saved_stats_t;
_Static_assert(offsetof(stats_t, first) == sizeof(saved_stats_t), "statistics prefix changed");

/* TVM values live in TI-OS variables and are not part of this payload. */
typedef struct {
    uint8_t magic;
    uint8_t version;
    uint8_t depth;
    real_t stack[STACK_MAX];
    uint8_t unit[STACK_MAX];
    real_t lastx;
    uint8_t unit_lastx;
    real_t regs[REG_BASE_COUNT];
    uint8_t extra_count;
    uint8_t lift;
    uint8_t degrees;
    uint8_t disp_style;
    int8_t disp_digits;
    tvm_settings_t tvm;
    saved_stats_t stats;
    base_state_t base;
    real_t extra[REG_EXTRA_MAX];    /* last, so only the used part is saved */
} saved_state_t;

#define SAVED_FIXED_SIZE (sizeof(saved_state_t) - sizeof(real_t) * REG_EXTRA_MAX)

/* Save/load share scratch storage rather than using the runtime stack. */
static saved_state_t s;

/* Register units are separate so damaged metadata cannot reject values.
 * Layout: 'UN', version, count, u32 value fingerprint, tags[count],
 * u32 checksum. Both hashes use FNV-1a; stack/LASTX units live in RPNCEdat. */
#define UNITS_NAME    "RPNCEunt"
#define UNITS_VERSION 1
#define UNITS_MAX     (REG_BASE_COUNT + REG_EXTRA_MAX)

static uint8_t unit_file[4 + 4 + UNITS_MAX + 4];
/* Optional precision witnesses: 'P',1, first X/Y, constancy bits, checksum. */
#define PRECISION_SIZE (2 + 2 * sizeof(real_t) + 1 + 4)
static uint8_t precision_file[PRECISION_SIZE];

static uint32_t fnv1a(uint32_t h, const void *data, size_t n)
{
    const uint8_t *p = data;
    while (n--) {
        h ^= *p++;
        h *= 16777619UL;
    }
    return h;
}

static uint32_t register_fingerprint(const real_t *regs, const real_t *extra, uint8_t extra_count)
{
    uint32_t h = fnv1a(2166136261UL, regs, sizeof(real_t) * REG_BASE_COUNT);
    h = fnv1a(h, &extra_count, 1);
    return fnv1a(h, extra, sizeof(real_t) * extra_count);
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint32_t precision_hash(void)
{
    uint32_t h = fnv1a(2166136261UL, &s.stats, sizeof s.stats);
    return fnv1a(h, precision_file, PRECISION_SIZE - 4);
}

static void apply_unit_tags(calc_t *c, const uint8_t *tags)
{
    for (int i = 0; i < REG_BASE_COUNT; i++)
        c->regs[i].unit = tags[i] <= UNIT_COUNT ? tags[i] : 0;
    for (int i = 0; i < c->extra_count; i++) {
        uint8_t u = tags[REG_BASE_COUNT + i];
        c->extra[i].unit = u <= UNIT_COUNT ? u : 0;
    }
}

/* Embedded tags are used only when the separate unit appvar is absent. */
static void load_units(calc_t *c, bool have_embedded_units)
{
    for (int i = 0; i < REG_BASE_COUNT; i++) c->regs[i].unit = 0;
    for (int i = 0; i < c->extra_count; i++) c->extra[i].unit = 0;

    uint8_t h = ti_Open(UNITS_NAME, "r");
    if (!h) {
        if (have_embedded_units)
            apply_unit_tags(c, unit_file + 8);
        return;
    }
    size_t count = REG_BASE_COUNT + c->extra_count;
    size_t want = 8 + count + 4;
    size_t n = ti_Read(unit_file, 1, sizeof unit_file, h);
    ti_Close(h);

    const uint8_t *p = unit_file;
    if (n != want || p[0] != 'U' || p[1] != 'N' || p[2] != UNITS_VERSION || p[3] != count)
        return;
    if (get32(p + 8 + count) != fnv1a(2166136261UL, p, 8 + count))
        return;                             /* damaged */
    if (get32(p + 4) != register_fingerprint(s.regs, s.extra, s.extra_count))
        return;                             /* belongs to other register values */
    apply_unit_tags(c, p + 8);
}

/* The old nine-byte DATE marker is decoded only at this file boundary.
 * No packed records or separate unit arrays exist in runtime state. */
static value_t import_value(const real_t *r, uint8_t unit)
{
    if (((uint8_t)r->sign & 0x60) == 0x60) {
        dt_obj_t date = {0};
        int q = (int8_t)r->mant[6];
        date.type = (uint8_t)r->sign & 0x0F;
        date.year = (int16_t)((uint16_t)(uint8_t)r->exp | ((uint16_t)r->mant[0] << 8));
        date.month = (int8_t)r->mant[1]; date.day = (int8_t)r->mant[2];
        date.hour = (int8_t)r->mant[3]; date.minute = (int8_t)r->mant[4];
        date.second = (int8_t)r->mant[5];
        date.tz_hour = (int8_t)(q / 4); date.tz_minute = (int8_t)((q % 4) * 15);
        return value_date(&date);
    }
    value_t v = value_real(*r);
    v.unit = unit <= UNIT_COUNT ? unit : 0;
    return v;
}

bool storage_import_v5(calc_t *c)
{
    uint8_t h = ti_Open(APPVAR_NAME, "r");
    if (!h)
        return false;
    bool valid = ti_Read(&s, 1, SAVED_FIXED_SIZE, h) == SAVED_FIXED_SIZE &&
                 s.magic == STATE_MAGIC && s.version == STATE_VERSION &&
                 s.depth >= STACK_MIN && s.depth <= STACK_MAX &&
                 s.extra_count <= REG_EXTRA_MAX;
    if (valid) {
        size_t tail = sizeof(real_t) * s.extra_count;
        valid = ti_Read(s.extra, 1, tail, h) == tail;
    }
    /* Optional 'U',1 footer: incomplete tags never reject numeric state. */
    bool have_embedded_units = false;
    bool have_precision = false;
    if (valid) {
        uint8_t header[2];
        size_t count = REG_BASE_COUNT + s.extra_count;
        for (uint8_t i = 0; i < 2; i++) {
            if (ti_Read(header, 1, sizeof header, h) != sizeof header || header[1] != 1)
                break;
            if (header[0] == 'U') {
                have_embedded_units = ti_Read(unit_file + 8, 1, count, h) == count;
            } else if (header[0] == 'P') {
                memcpy(precision_file, header, 2);
                have_precision = ti_Read(precision_file + 2, 1, PRECISION_SIZE - 2, h) == PRECISION_SIZE - 2 &&
                    get32(precision_file + PRECISION_SIZE - 4) == precision_hash() &&
                    precision_file[PRECISION_SIZE - 5] <= 3;
            } else break;
        }
    }
    ti_Close(h);
    if (!valid)
        return false;

    c->depth = s.depth;
    for (int i = 0; i < STACK_MAX; i++) c->stack[i] = import_value(&s.stack[i], s.unit[i]);
    c->lastx = import_value(&s.lastx, s.unit_lastx);
    for (int i = 0; i < REG_BASE_COUNT; i++) c->regs[i] = import_value(&s.regs[i], 0);
    c->extra_count = s.extra_count;
    for (int i = 0; i < REG_EXTRA_MAX; i++)
        c->extra[i] = i < s.extra_count ? import_value(&s.extra[i], 0) : value_real(os_Int24ToReal(0));
    c->complex_results = false;
    c->complex_mode = COMPLEX_RECT;
    load_units(c, have_embedded_units);
    c->lift = s.lift != 0;
    c->degrees = s.degrees != 0;
    if (s.disp_style >= DISP_FIX && s.disp_style <= DISP_ENG &&
        s.disp_digits >= DISP_FLOAT && s.disp_digits <= DISP_MAX_DIGITS) {
        c->disp_style = s.disp_style;
        c->disp_digits = s.disp_digits;
    }
    c->tvm = s.tvm;
    if (c->tvm.iter_max == 0)
        tvm_settings_reset(&c->tvm);
    memcpy(&c->stats, &s.stats, sizeof s.stats);
    memset(c->stats.first, 0, sizeof c->stats.first);
    c->stats.constant = 0;              /* old sums alone cannot prove zero variance */
    if (have_precision) {
        memcpy(c->stats.first, precision_file + 2, sizeof c->stats.first);
        c->stats.constant = precision_file[PRECISION_SIZE - 5];
    }
    if (c->stats.model >= FIT_COUNT)
        c->stats.model = FIT_LIN;
    c->base = s.base;
    base_validate(&c->base);
    for (int i = 0; i < REG_BASE_COUNT; i++)
        if (!value_is_real(&c->regs[i])) c->regs[i].unit = 0;
    for (int i = 0; i < c->extra_count; i++)
        if (!value_is_real(&c->extra[i])) c->extra[i].unit = 0;
    return true;
}

