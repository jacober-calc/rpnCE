/* Adapts DATE values and functions to the calculator stack and registers.
 * Calendar arithmetic and parsing remain in datetime.c. */
#include <string.h>
#include <stddef.h>
#include "calc_date.h"
#include "calc_internal.h"
#include "value_date.h"
#include "value_parse.h"

/* ---------- DATE ---------- */

static dt_int epoch_days(const calc_t *c)
{
    /* Days from Unix: 1970-01-01, 1900-01-01, 1980-01-06, 1997-01-01,
     * 2000-01-01. Only the user-defined epoch needs calendar conversion. */
    static const int16_t days[] = { 0, -25567, 3657, 9862, 10957 };
    if (c->epoch < EPOCH_CUSTOM)
        return days[c->epoch];
    if (c->epoch == EPOCH_CUSTOM)
        return dt_days_from_civil(c->custom_epoch.year,
                                 c->custom_epoch.month, c->custom_epoch.day);
    return 0;
}

/* Component commands differ only in their field and accepted record types.
 * Duration days share the wider year field; all remaining fields are int8_t. */
static dt_err_t component(const dt_obj_t *o, date_fn_t fn, dt_int *out)
{
    enum {
        DATED = (1 << OBJ_DATE) | (1 << OBJ_DATETIME) | (1 << OBJ_DZ),
        TIMED = (1 << OBJ_TIME) | (1 << OBJ_DATETIME) | (1 << OBJ_DZ) | (1 << OBJ_DR),
        ZONED = (1 << OBJ_TZ) | (1 << OBJ_DZ)
    };
    static const struct { uint8_t offset, types; } fields[] = {
        { offsetof(dt_obj_t, year), DATED },
        { offsetof(dt_obj_t, month), DATED },
        { offsetof(dt_obj_t, day), DATED | (1 << OBJ_DR) },
        { offsetof(dt_obj_t, hour), TIMED },
        { offsetof(dt_obj_t, minute), TIMED },
        { offsetof(dt_obj_t, second), TIMED },
        { offsetof(dt_obj_t, tz_hour), ZONED },
        { offsetof(dt_obj_t, tz_minute), ZONED }
    };
    _Static_assert(OBJ_COUNT <= 8, "component type masks require 8 bits");
    _Static_assert(sizeof fields / sizeof fields[0] == DF_TZ_MINUTE - DF_YEAR + 1,
                   "component commands must match their field table");
    unsigned index = fn - DF_YEAR;
    if (o->type >= OBJ_COUNT || !(fields[index].types & (1 << o->type)))
        return DT_ERR_TYPE;
    *out = (fn == DF_YEAR || (fn == DF_DAY && o->type == OBJ_DR)) ? o->year :
           *((const int8_t *)o + fields[index].offset);
    return DT_OK;
}

bool calc_clock_now(const calc_t *c, bool utc, dt_obj_t *out)
{
    int y, mo, d, h, mi, s;
    if (!sys_clock_read(&y, &mo, &d, &h, &mi, &s))
        return false;
    memset(out, 0, sizeof *out);
    out->type = OBJ_DZ;
    out->year = (int16_t)y; out->month = (int8_t)mo; out->day = (int8_t)d;
    out->hour = (int8_t)h; out->minute = (int8_t)mi; out->second = (int8_t)s;
    out->tz_hour = (int8_t)(c->clock_tz / 4);
    out->tz_minute = (int8_t)((c->clock_tz % 4) * 15);
    if (dt_validate(out) != DT_OK)
        return false;
    int zone = utc ? 0 : c->app_tz;
    if (zone == c->clock_tz)
        return true;
    dt_obj_t clock = *out;
    return dt_convert_tz(&clock, zone, out) == DT_OK;
}

/* A time zone from X: a TimeZone object or hours (multiples of 0.25). */
static bool tz_from_value(calc_t *c, const dt_val_t *v, int *q)
{
    if (v->is_obj && v->obj.type == OBJ_TZ) {
        *q = dt_tz_quarters(&v->obj);
        return true;
    }
    if (!v->is_obj && !c->stack[REG_X].unit && v->is_quarter && v->quarters >= -95 && v->quarters <= 95) {
        *q = (int)v->quarters;
        return true;
    }
    c->message = value_date_error(!value_is_plain(&c->stack[REG_X]) ? DT_ERR_TYPE : DT_ERR_DOMAIN);
    return false;
}

/* Pure conversions stage a DATE result. Stack updates, settings and the
 * hardware clock belong to the adapter below. Keep this call boundary under
 * LTO so conversion temporaries do not enlarge every settings/clock branch. */
static __attribute__((noinline)) dt_err_t convert_value(const calc_t *c, date_fn_t fn,
                                                 dt_result_t *out)
{
    const value_t *v = &c->stack[REG_X];
    dt_val_t x = value_date_operand(v, fn == DF_HR_TO_TZ);
    uint8_t t = x.is_obj ? x.obj.type : OBJ_NONE;
    bool dated = (t == OBJ_DATE || t == OBJ_DATETIME || t == OBJ_DZ);
    dt_obj_t *o = &out->obj;
    out->is_obj = true;
    out->den = 1;
    memset(o, 0, sizeof *o);

    if (fn >= DF_YEAR && fn <= DF_TZ_MINUTE) {
        out->is_obj = false;
        return component(&x.obj, fn, &out->num);
    }

    /* These conversions share one plain-number/whole-number boundary. */
    if ((fn >= DF_ED_TO_D && fn <= DF_ED_TO_DZ) ||
        (fn >= DF_ES_TO_D && fn <= DF_ES_TO_DZ_APP) || fn == DF_S_TO_TIME ||
        (fn >= DF_DAYS_TO_DR && fn <= DF_SECONDS_TO_DR) ||
        fn == DF_ISO_TO_DW || fn == DF_UNIX_TO_DW || fn == DF_HR_TO_TZ) {
        if (!value_is_plain(v)) return DT_ERR_TYPE;
        if (!(fn == DF_HR_TO_TZ ? x.is_quarter : x.is_whole)) return DT_ERR_DOMAIN;
    }

    switch (fn) {
        case DF_TO_ED:
        case DF_TO_ES:
            if (!dated) return DT_ERR_TYPE;
            out->is_obj = false;
            dt_to_epoch_seconds(&x.obj, epoch_days(c), &out->num);
            if (fn == DF_TO_ED) {               /* the time is dropped */
                dt_int n = out->num;
                out->num = (n >= 0) ? n / 86400 : -((-n + 86399) / 86400);
            }
            return DT_OK;
        case DF_ED_TO_D: case DF_ED_TO_DT: case DF_ED_TO_DZ:
        case DF_ES_TO_D: case DF_ES_TO_DT: case DF_ES_TO_DZ: case DF_ES_TO_DZ_APP: {
            bool days = (fn >= DF_ED_TO_D && fn <= DF_ED_TO_DZ);
            obj_type_t ty = (fn == DF_ED_TO_D || fn == DF_ES_TO_D) ? OBJ_DATE :
                            (fn == DF_ED_TO_DT || fn == DF_ES_TO_DT) ? OBJ_DATETIME : OBJ_DZ;
            int tzq = (fn == DF_ES_TO_DZ_APP) ? c->app_tz : 0;
            return dt_from_epoch_seconds(days ? x.whole * 86400 : x.whole,
                                         epoch_days(c), ty, tzq, o);
        }
        case DF_LEAP:
            out->is_obj = false;
            if (dated)
                out->num = dt_is_leap(x.obj.year);
            else if (value_is_plain(v) && x.is_whole && x.whole >= 1 && x.whole <= 9999)
                out->num = dt_is_leap((int)x.whole);
            else return !value_is_plain(v) ? DT_ERR_TYPE : DT_ERR_DOMAIN;
            return DT_OK;
        case DF_DOW:
            if (!dated) return DT_ERR_TYPE;
            o->type = OBJ_DW;
            o->day = (int8_t)dt_iso_dow(dt_days_from_civil(x.obj.year, x.obj.month, x.obj.day));
            return DT_OK;
        case DF_GET_DATE: case DF_GET_TIME: case DF_GET_DT: case DF_GET_TZ:
            if (!(t == OBJ_DATETIME || t == OBJ_DZ) ||
                ((fn == DF_GET_DT || fn == DF_GET_TZ) && t != OBJ_DZ))
                return DT_ERR_TYPE;
            *o = x.obj;
            o->type = (fn == DF_GET_DATE) ? OBJ_DATE : (fn == DF_GET_TIME) ? OBJ_TIME :
                      (fn == DF_GET_DT) ? OBJ_DATETIME : OBJ_TZ;
            if (o->type == OBJ_DATE)
                o->hour = o->minute = o->second = 0;
            if (o->type != OBJ_TZ)
                o->tz_hour = o->tz_minute = 0;
            return DT_OK;
        case DF_TIME_TO_S:
            if (t != OBJ_TIME) return DT_ERR_TYPE;
            out->is_obj = false;
            out->num = dt_time_seconds(&x.obj);
            return DT_OK;
        case DF_S_TO_TIME:
            return dt_time_from_seconds(x.whole, o);
        case DF_TZ_TO_HR:
            if (t != OBJ_TZ) return DT_ERR_TYPE;
            out->is_obj = false;
            out->num = dt_tz_quarters(&x.obj);
            out->den = 4;
            return DT_OK;
        case DF_HR_TO_TZ:
            return dt_tz_from_quarters(x.quarters, o);
        case DF_DAYS_TO_DR: case DF_HOURS_TO_DR: case DF_MINUTES_TO_DR: case DF_SECONDS_TO_DR: {
            static const int unit[4] = { 86400, 3600, 60, 1 };
            return dt_duration_from_seconds(x.whole * unit[fn - DF_DAYS_TO_DR], o);
        }
        case DF_DR_TO_S:
            if (t != OBJ_DR) return DT_ERR_TYPE;
            out->is_obj = false;
            out->num = dt_duration_seconds(&x.obj);
            return DT_OK;
        case DF_DW_TO_ISO: case DF_DW_TO_UNIX:
            if (t != OBJ_DW) return DT_ERR_TYPE;
            out->is_obj = false;
            out->num = fn == DF_DW_TO_ISO ? x.obj.day : x.obj.day % 7;
            return DT_OK;
        case DF_ISO_TO_DW: case DF_UNIX_TO_DW: {
            dt_int lo = (fn == DF_ISO_TO_DW) ? 1 : 0;
            if (x.whole < lo || x.whole > lo + 6) return DT_ERR_DOMAIN;
            o->type = OBJ_DW;
            o->day = (int8_t)(x.whole == 0 ? 7 : x.whole);
            return DT_OK;
        }
        default:
            return DT_ERR_TYPE;
    }
}

void calc_date_execute(calc_t *c, date_fn_t fn)
{
    const value_t *x = &c->stack[REG_X];
    uint8_t t = value_is_date(x) ? x->date.type : OBJ_NONE;
    dt_result_t result;
    dt_obj_t *o = &result.obj;
    dt_err_t e = DT_OK;
    bool push = true;
    result.is_obj = true;
    result.den = 1;

    switch (fn) {
        /* Menu aliases share the universal evaluator's arithmetic rules. */
        case DF_CVTZ: case DF_SHRINK: case DF_EXTEND: case DF_CUT: case DF_LINK:
            calc_execute(c, CMD_DATE_VALUE, fn);
            return;
        case DF_EPOCH_SET_CUSTOM:
            if (t != OBJ_DATE) { e = DT_ERR_TYPE; break; }
            c->custom_epoch = x->date;
            c->epoch = EPOCH_CUSTOM;
            c->message = "Epoch Set";
            return;
        case DF_EPOCH_GET_CUSTOM:
            *o = c->custom_epoch;
            break;
        case DF_NOW: case DF_NOW_DATE: case DF_NOW_TIME: case DF_NOW_DZ: case DF_NOW_DZ_UTC:
            if (!calc_clock_now(c, fn == DF_NOW || fn == DF_NOW_DZ_UTC, o)) {
                c->message = "Err: Clock";
                return;
            }
            if (fn == DF_NOW) {
                dt_to_epoch_seconds(o, epoch_days(c), &result.num);
                result.is_obj = false;
            } else if (fn == DF_NOW_DATE) {
                o->type = OBJ_DATE;
                o->hour = o->minute = o->second = 0;
                o->tz_hour = o->tz_minute = 0;
            } else if (fn == DF_NOW_TIME) {
                o->type = OBJ_TIME;
                o->year = 0; o->month = o->day = 0;
                o->tz_hour = o->tz_minute = 0;
            }
            break;
        case DF_SET_APP_TZ:
        case DF_SET_CLOCK_TZ: {
            dt_val_t v = value_date_operand(x, true);
            int q;
            if (!tz_from_value(c, &v, &q)) return;
            if (fn == DF_SET_APP_TZ) c->app_tz = (int8_t)q;
            else c->clock_tz = (int8_t)q;
            c->message = "Time Zone Set";
            return;
        }
        case DF_GET_APP_TZ:
        case DF_GET_CLOCK_TZ:
            dt_tz_from_quarters(fn == DF_GET_APP_TZ ? c->app_tz : c->clock_tz, o);
            break;
        case DF_SET_CLOCK:
            /* A ZonedDateTime shown in the clock's own zone; an unzoned
             * DateTime is already in that zone. */
            if (t != OBJ_DZ && t != OBJ_DATETIME) { e = DT_ERR_TYPE; break; }
            if (t == OBJ_DATETIME) *o = x->date;
            else {
                e = dt_convert_tz(&x->date, c->clock_tz, o);
                if (e) break;
            }
            sys_clock_write(o->year, o->month, o->day, o->hour, o->minute, o->second);
            c->message = "Clock Set";
            return;
        default:
            if (fn > DF_UNIX_TO_DW) return;
            e = convert_value(c, fn, &result);
            push = false;
            break;
    }
    if (e) {
        c->message = value_date_error(e);
        return;
    }
    value_t v = value_date_result(&result);
    if (push) calc_stack_enter(c, &v);
    else calc_stack_set_x(c, &v);
}
