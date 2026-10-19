#include <string.h>
#include <fileioc.h>

#include "storage.h"
#include "../core/format.h"
#include "../core/units.h"

#define APPVAR_NAME   "RPNCEdat"
#define STATE_MAGIC   0x52   /* 'R' */
#define STATE_VERSION 5      /* bump when the saved layout changes */

/* Everything that survives quitting. The TVM values themselves live in the
 * OS's own TVM variables, so they aren't stored here. */
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
    stats_t stats;
    base_state_t base;
    real_t extra[REG_EXTRA_MAX];    /* last, so only the used part is saved */
} saved_state_t;

#define SAVED_FIXED_SIZE (sizeof(saved_state_t) - sizeof(real_t) * REG_EXTRA_MAX)

/* Loading and saving run sequentially. Share one scratch buffer, kept off
 * the stack because it includes space for every extra register. */
static saved_state_t s;

/* ---------- register units: their own appvar ----------
 *
 * Units on the registers (A-Z, i and the extra registers) are kept in
 * RPNCEunt, separate from RPNCEdat, so a problem with them can never cost
 * the saved values: RPNCEdat keeps exactly the 0.6.6 layout, and if
 * RPNCEunt is missing, damaged, or doesn't belong to the registers just
 * loaded, the registers simply load without units.
 *
 *   'U' 'N'  version  count        header (count = number of tags)
 *   u32      fingerprint           FNV-1a of the register values the tags
 *                                  belong to, so tags never attach to
 *                                  different values (e.g. an older RPNCEdat)
 *   u8 x count                     a unit tag per register (0: none)
 *   u32      checksum              FNV-1a of everything before it
 *
 * (Stack and LASTX units are part of RPNCEdat, as in 0.6.6.)
 */
#define UNITS_NAME    "RPNCEunt"
#define UNITS_VERSION 1
#define UNITS_MAX     (REG_BASE_COUNT + REG_EXTRA_MAX)

static uint8_t unit_file[4 + 4 + UNITS_MAX + 4];

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

static void put32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++)
        p[i] = (uint8_t)(v >> (8 * i));
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void save_units(const calc_t *c)
{
    size_t count = REG_BASE_COUNT + c->extra_count;
    bool any = false;
    for (int i = 0; i < REG_BASE_COUNT; i++)
        any |= c->reg_unit[i] != 0;
    for (int i = 0; i < c->extra_count; i++)
        any |= c->extra_unit[i] != 0;
    if (!any) {
        ti_Delete(UNITS_NAME);              /* nothing to keep: no stale file */
        return;
    }

    uint8_t *p = unit_file;
    p[0] = 'U';
    p[1] = 'N';
    p[2] = UNITS_VERSION;
    p[3] = (uint8_t)count;
    put32(p + 4, register_fingerprint(c->regs, c->extra, c->extra_count));
    memcpy(p + 8, c->reg_unit, REG_BASE_COUNT);
    memcpy(p + 8 + REG_BASE_COUNT, c->extra_unit, c->extra_count);
    put32(p + 8 + count, fnv1a(2166136261UL, p, 8 + count));

    uint8_t h = ti_Open(UNITS_NAME, "w");
    if (!h)
        return;
    ti_Write(p, 1, 8 + count + 4, h);
    ti_SetArchiveStatus(true, h);
    ti_Close(h);
}

static void apply_unit_tags(calc_t *c, const uint8_t *tags)
{
    for (int i = 0; i < REG_BASE_COUNT; i++)
        c->reg_unit[i] = tags[i] <= UNIT_COUNT ? tags[i] : 0;
    for (int i = 0; i < c->extra_count; i++) {
        uint8_t u = tags[REG_BASE_COUNT + i];
        c->extra_unit[i] = u <= UNIT_COUNT ? u : 0;
    }
}

/* Validate RPNCEunt against the loaded values. Only when it is absent may
 * a complete legacy footer supply the tags instead. */
static void load_units(calc_t *c, bool have_legacy_units)
{
    memset(c->reg_unit, 0, sizeof c->reg_unit);
    memset(c->extra_unit, 0, sizeof c->extra_unit);

    uint8_t h = ti_Open(UNITS_NAME, "r");
    if (!h) {
        /* Before the new file exists, migrate the earlier refactor footer.
         * A subsequent save writes these tags to RPNCEunt. */
        if (have_legacy_units)
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
    if (get32(p + 4) != register_fingerprint(c->regs, c->extra, c->extra_count))
        return;                             /* belongs to other register values */
    apply_unit_tags(c, p + 8);
}

void storage_save(calc_t *c)
{
    calc_finish_entry(c);

    s.magic = STATE_MAGIC;
    s.version = STATE_VERSION;
    s.depth = c->depth;
    memcpy(s.stack, c->stack, sizeof s.stack);
    memcpy(s.unit, c->unit, sizeof s.unit);
    s.lastx = c->lastx;
    s.unit_lastx = c->unit_lastx;
    memcpy(s.regs, c->regs, sizeof s.regs);
    s.extra_count = c->extra_count;
    s.lift = c->lift;
    s.degrees = c->degrees;
    s.disp_style = c->disp_style;
    s.disp_digits = c->disp_digits;
    s.tvm = c->tvm;
    s.stats = c->stats;
    s.base = c->base;
    memcpy(s.extra, c->extra, sizeof(real_t) * c->extra_count);

    uint8_t h = ti_Open(APPVAR_NAME, "w");
    if (!h)
        return;
    ti_Write(&s, SAVED_FIXED_SIZE + sizeof(real_t) * c->extra_count, 1, h);
    ti_SetArchiveStatus(true, h);   /* archived: survives a RAM clear */
    ti_Close(h);

    save_units(c);
}

bool storage_load(calc_t *c)
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
    /* Earlier refactor builds appended 'U', 1 and one tag per register.
     * Keep a complete, recognized footer in the existing scratch buffer
     * for migration if RPNCEunt does not exist. Bad metadata never rejects
     * an otherwise valid numeric/DATE save. */
    bool have_legacy_units = false;
    if (valid) {
        uint8_t header[2];
        size_t count = REG_BASE_COUNT + s.extra_count;
        have_legacy_units = ti_Read(header, 1, sizeof header, h) == sizeof header &&
                            header[0] == 'U' && header[1] == 1 &&
                            ti_Read(unit_file + 8, 1, count, h) == count;
    }
    ti_Close(h);
    if (!valid)
        return false;

    c->depth = s.depth;
    memcpy(c->stack, s.stack, sizeof c->stack);
    for (int i = 0; i < STACK_MAX; i++)
        c->unit[i] = (s.unit[i] <= UNIT_COUNT) ? s.unit[i] : 0;
    c->lastx = s.lastx;
    c->unit_lastx = (s.unit_lastx <= UNIT_COUNT) ? s.unit_lastx : 0;
    memcpy(c->regs, s.regs, sizeof c->regs);
    c->extra_count = s.extra_count;
    memcpy(c->extra, s.extra, sizeof(real_t) * s.extra_count);
    load_units(c, have_legacy_units);
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
    c->stats = s.stats;
    if (c->stats.model >= FIT_COUNT)
        c->stats.model = FIT_LIN;
    c->base = s.base;
    base_validate(&c->base);
    return true;
}

/* ---------- settings ---------- */

#define SETTINGS_NAME "RPNCEcfg"

typedef struct {
    uint8_t magic;
    uint8_t version;        /* 1: tint only; 2: and the DATE settings */
    uint8_t tint;
    uint8_t date_as_string;
    uint8_t epoch;
    dt_obj_t custom_epoch;
    int8_t app_tz, clock_tz;
} settings_t;

void storage_save_settings(uint8_t tint, const calc_t *c)
{
    settings_t s;
    s.magic = STATE_MAGIC;
    s.version = 2;
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
    ti_Close(h);
    if (n < 3 || s.magic != STATE_MAGIC)
        return 0;
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
