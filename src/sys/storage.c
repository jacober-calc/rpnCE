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
/* Keep the v5 payload readable by earlier builds. Those readers ignore
 * trailing bytes; this optional extension preserves register units. */
static const uint8_t unit_extension[] = { 'U', 1 };
static uint8_t saved_units[REG_BASE_COUNT + REG_EXTRA_MAX];

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
    ti_Write(unit_extension, 1, sizeof unit_extension, h);
    ti_Write(c->reg_unit, 1, REG_BASE_COUNT, h);
    ti_Write(c->extra_unit, 1, c->extra_count, h);
    ti_SetArchiveStatus(true, h);   /* archived: survives a RAM clear */
    ti_Close(h);
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
    memset(saved_units, 0, sizeof saved_units);
    if (valid) {
        uint8_t extension[sizeof unit_extension];
        size_t n = ti_Read(extension, 1, sizeof extension, h);
        /* No extension means an original v5 save, with scalar registers. */
        if (n != 0) {
            size_t tags = REG_BASE_COUNT + s.extra_count;
            valid = n == sizeof extension &&
                    memcmp(extension, unit_extension, sizeof extension) == 0 &&
                    ti_Read(saved_units, 1, tags, h) == tags;
        }
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
    for (int i = 0; i < REG_BASE_COUNT; i++)
        c->reg_unit[i] = saved_units[i] <= UNIT_COUNT ? saved_units[i] : 0;
    memset(c->extra_unit, 0, sizeof c->extra_unit);
    for (int i = 0; i < s.extra_count; i++) {
        uint8_t u = saved_units[REG_BASE_COUNT + i];
        c->extra_unit[i] = u <= UNIT_COUNT ? u : 0;
    }
    c->extra_count = s.extra_count;
    memcpy(c->extra, s.extra, sizeof(real_t) * s.extra_count);
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
