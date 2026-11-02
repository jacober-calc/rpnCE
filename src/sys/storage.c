#include <string.h>
#include <stddef.h>
#include <fileioc.h>
#include "storage_internal.h"
#include "../core/format.h"

#define SAVED_FIXED_SIZE offsetof(storage_state_t, extra)

static void precision_checksum(uint8_t out[4], const storage_stats_t *stats,
                               const uint8_t *precision)
{
    uint32_t h = storage_precision_hash(stats, precision);
    for (int i = 0; i < 4; i++) out[i] = (uint8_t)(h >> (8 * i));
}

void storage_save(calc_t *c)
{
    storage_state_t *s = &storage_workspace.current.state;
    uint8_t *precision = storage_workspace.current.precision;
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
    precision[2 + sizeof c->stats.first] = c->stats.constant;
    precision_checksum(precision + PRECISION_SIZE - 4, &s->stats, precision);
    uint8_t h = ti_Open(STATE_APPVAR, "w");
    if (!h) return;
    size_t n = SAVED_FIXED_SIZE + sizeof(value_t) * c->extra_count;
    bool saved = ti_Write(s, 1, n, h) == n && ti_Write(precision, 1, PRECISION_SIZE, h) == PRECISION_SIZE;
    ti_SetArchiveStatus(true, h);
    ti_Close(h);
    if (saved) ti_Delete("RPNCEunt"); /* superseded by each value's own unit */
}

/* The rest of a version-6 save, after its header. */
static bool load_current(calc_t *c, uint8_t h)
{
    storage_state_t *s = &storage_workspace.current.state;
    uint8_t *precision = storage_workspace.current.precision;
    const size_t rest = SAVED_FIXED_SIZE - STORAGE_HEADER_SIZE;
    if (ti_Read((uint8_t *)s + STORAGE_HEADER_SIZE, 1, rest, h) != rest ||
        s->depth < STACK_MIN || s->depth > STACK_MAX || s->extra_count > REG_EXTRA_MAX)
        return false;
    size_t tail = sizeof(value_t) * s->extra_count;
    if (ti_Read(s->extra, 1, tail, h) != tail)
        return false;
    bool have_precision = ti_Read(precision, 1, PRECISION_SIZE, h) == PRECISION_SIZE &&
                          storage_precision_valid(&s->stats, precision);
    for (int i = 0; i < STACK_MAX; i++) if (!value_valid(&s->stack[i])) return false;
    if (!value_valid(&s->lastx)) return false;
    for (int i = 0; i < REG_BASE_COUNT; i++) if (!value_valid(&s->regs[i])) return false;
    for (int i = 0; i < s->extra_count; i++) if (!value_valid(&s->extra[i])) return false;
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
    storage_restore_math(c, &s->tvm, &s->stats, &s->base, have_precision ? precision : NULL);
    return true;
}

/* One pass over the appvar: the header picks the reader, which continues
 * from the same handle. Readers stage everything before changing c. */
bool storage_load(calc_t *c)
{
    storage_state_t *s = &storage_workspace.current.state;
    uint8_t h = ti_Open(STATE_APPVAR, "r");
    if (!h) return false;
    bool loaded = ti_Read(s, 1, STORAGE_HEADER_SIZE, h) == STORAGE_HEADER_SIZE &&
                  s->magic == STATE_MAGIC &&
                  (s->version == 5 ? storage_import_v5(c, h) :
                   s->version == STATE_VERSION && load_current(c, h));
    ti_Close(h);
    return loaded;
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
