/* Read-only migration for version 5 saves. This module never writes the
 * retired format; all current persistence lives in storage.c. */
#include <string.h>
#include <stddef.h>
#include <fileioc.h>

#include "storage_internal.h"
#include "../core/format.h"
#include "../core/units.h"

#define SAVED_FIXED_SIZE offsetof(storage_legacy_state_t, extra)
#define UNITS_NAME "RPNCEunt"
#define UNITS_VERSION 1

static uint32_t register_fingerprint(const real_t *regs, const real_t *extra, uint8_t extra_count)
{
    uint32_t h = storage_hash(2166136261UL, regs, sizeof(real_t) * REG_BASE_COUNT);
    h = storage_hash(h, &extra_count, 1);
    return storage_hash(h, extra, sizeof(real_t) * extra_count);
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
    const storage_legacy_state_t *s = &storage_workspace.legacy.state;
    uint8_t *unit_file = storage_workspace.legacy.units;
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
    size_t n = ti_Read(unit_file, 1, sizeof storage_workspace.legacy.units, h);
    ti_Close(h);

    const uint8_t *p = unit_file;
    if (n != want || p[0] != 'U' || p[1] != 'N' || p[2] != UNITS_VERSION || p[3] != count)
        return;
    if (storage_get32(p + 8 + count) != storage_hash(2166136261UL, p, 8 + count))
        return;                             /* damaged */
    if (storage_get32(p + 4) != register_fingerprint(s->regs, s->extra, s->extra_count))
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
    storage_legacy_state_t *s = &storage_workspace.legacy.state;
    uint8_t *unit_file = storage_workspace.legacy.units;
    uint8_t *precision_file = storage_workspace.legacy.precision;
    uint8_t h = ti_Open(STATE_APPVAR, "r");
    if (!h)
        return false;
    bool valid = ti_Read(s, 1, SAVED_FIXED_SIZE, h) == SAVED_FIXED_SIZE &&
                 s->magic == STATE_MAGIC && s->version == 5 &&
                 s->depth >= STACK_MIN && s->depth <= STACK_MAX &&
                 s->extra_count <= REG_EXTRA_MAX;
    if (valid) {
        size_t tail = sizeof(real_t) * s->extra_count;
        valid = ti_Read(s->extra, 1, tail, h) == tail;
    }
    /* Optional 'U',1 footer: incomplete tags never reject numeric state. */
    bool have_embedded_units = false;
    bool have_precision = false;
    if (valid) {
        uint8_t header[2];
        size_t count = REG_BASE_COUNT + s->extra_count;
        for (uint8_t i = 0; i < 2; i++) {
            if (ti_Read(header, 1, sizeof header, h) != sizeof header || header[1] != 1)
                break;
            if (header[0] == 'U') {
                have_embedded_units = ti_Read(unit_file + 8, 1, count, h) == count;
            } else if (header[0] == 'P') {
                memcpy(precision_file, header, 2);
                have_precision = ti_Read(precision_file + 2, 1, PRECISION_SIZE - 2, h) == PRECISION_SIZE - 2 &&
                    storage_precision_valid(&s->stats, precision_file);
            } else break;
        }
    }
    ti_Close(h);
    if (!valid)
        return false;

    c->depth = s->depth;
    for (int i = 0; i < STACK_MAX; i++) c->stack[i] = import_value(&s->stack[i], s->unit[i]);
    c->lastx = import_value(&s->lastx, s->unit_lastx);
    for (int i = 0; i < REG_BASE_COUNT; i++) c->regs[i] = import_value(&s->regs[i], 0);
    c->extra_count = s->extra_count;
    for (int i = 0; i < REG_EXTRA_MAX; i++)
        c->extra[i] = i < s->extra_count ? import_value(&s->extra[i], 0) : value_real(os_Int24ToReal(0));
    c->complex_results = false;
    c->complex_mode = COMPLEX_RECT;
    load_units(c, have_embedded_units);
    c->lift = s->lift != 0;
    c->degrees = s->degrees != 0;
    if (s->disp_style >= DISP_FIX && s->disp_style <= DISP_ENG &&
        s->disp_digits >= DISP_FLOAT && s->disp_digits <= DISP_MAX_DIGITS) {
        c->disp_style = s->disp_style;
        c->disp_digits = s->disp_digits;
    }
    storage_restore_math(c, &s->tvm, &s->stats, &s->base, have_precision ? precision_file : NULL);
    for (int i = 0; i < REG_BASE_COUNT; i++)
        if (!value_is_real(&c->regs[i])) c->regs[i].unit = 0;
    for (int i = 0; i < c->extra_count; i++)
        if (!value_is_real(&c->extra[i])) c->extra[i].unit = 0;
    return true;
}

