/* Adapts DATE values and functions to the calculator stack and registers.
 * Calendar arithmetic and parsing remain in datetime.c. */
#include <string.h>
#include "calc_date.h"
#include "calc_internal.h"
#include "mathx.h"
#include "value_date.h"
#include "value_math.h"
#include "value_parse.h"

/* ---------- DATE ---------- */

static dt_int epoch_days(const calc_t *c)
{
    switch (c->epoch) {
        case EPOCH_NTP:    return dt_days_from_civil(1900, 1, 1);
        case EPOCH_GPS:    return dt_days_from_civil(1980, 1, 6);
        case EPOCH_TIOS:   return dt_days_from_civil(1997, 1, 1);
        case EPOCH_Y2K:    return dt_days_from_civil(2000, 1, 1);
        case EPOCH_CUSTOM: return dt_days_from_civil(c->custom_epoch.year,
                                   c->custom_epoch.month, c->custom_epoch.day);
        default:           return 0;
    }
}

static void set_x_obj(calc_t *c, const dt_obj_t *o)
{
    value_t v = value_date(o);
    calc_stack_set_x(c, &v);
}

static void set_x_whole(calc_t *c, dt_int n)
{
    value_t v = value_real(value_whole_real(n));
    calc_stack_set_x(c, &v);
}

static void push_obj(calc_t *c, const dt_obj_t *o)
{
    value_t v = value_date(o);
    calc_stack_enter(c, &v);
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

void calc_date_execute(calc_t *c, date_fn_t fn)
{
    dt_val_t x = value_date_operand(&c->stack[0], fn == DF_HR_TO_TZ ||
                         fn == DF_SET_APP_TZ || fn == DF_SET_CLOCK_TZ);
    uint8_t t = x.is_obj ? x.obj.type : OBJ_NONE;
    bool dated = (t == OBJ_DATE || t == OBJ_DATETIME || t == OBJ_DZ);
    dt_obj_t o;
    dt_int n;
    dt_err_t e = DT_OK;
    int q;

    memset(&o, 0, sizeof o);
    switch (fn) {
        /* ---- epoch days and seconds ---- */
        case DF_TO_ED:
        case DF_TO_ES:
            if (!dated) { e = DT_ERR_TYPE; break; }
            dt_to_epoch_seconds(&x.obj, epoch_days(c), &n);
            if (fn == DF_TO_ED) {               /* the time is dropped */
                n = (n >= 0) ? n / 86400 : -((-n + 86399) / 86400);
            }
            set_x_whole(c, n);
            return;
        case DF_ED_TO_D: case DF_ED_TO_DT: case DF_ED_TO_DZ:
        case DF_ES_TO_D: case DF_ES_TO_DT: case DF_ES_TO_DZ: case DF_ES_TO_DZ_APP: {
            if (!value_is_plain(&c->stack[REG_X])) { e = DT_ERR_TYPE; break; }
            if (!x.is_whole) { e = DT_ERR_DOMAIN; break; }
            bool days = (fn == DF_ED_TO_D || fn == DF_ED_TO_DT || fn == DF_ED_TO_DZ);
            obj_type_t ty = (fn == DF_ED_TO_D || fn == DF_ES_TO_D) ? OBJ_DATE :
                            (fn == DF_ED_TO_DT || fn == DF_ES_TO_DT) ? OBJ_DATETIME : OBJ_DZ;
            int tzq = (fn == DF_ES_TO_DZ_APP) ? c->app_tz : 0;
            e = dt_from_epoch_seconds(days ? x.whole * 86400 : x.whole, epoch_days(c),
                                      ty, tzq, &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        }
        case DF_LEAP:
            if (dated)
                n = dt_is_leap(x.obj.year);
            else if (!x.is_obj && !c->stack[REG_X].unit && x.is_whole && x.whole >= 1 && x.whole <= 9999)
                n = dt_is_leap((int)x.whole);
            else { e = !value_is_plain(&c->stack[REG_X]) ? DT_ERR_TYPE : DT_ERR_DOMAIN; break; }
            set_x_whole(c, n);
            return;
        case DF_DOW:
            if (!dated) { e = DT_ERR_TYPE; break; }
            o.type = OBJ_DW;
            o.day = (int8_t)dt_iso_dow(dt_days_from_civil(x.obj.year, x.obj.month, x.obj.day));
            set_x_obj(c, &o);
            return;
        case DF_CVTZ:
            calc_execute(c, CMD_DATE_VALUE, DF_CVTZ);
            return;

        /* ---- components ---- */
        case DF_YEAR: case DF_MONTH:
            if (!dated) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, fn == DF_YEAR ? x.obj.year : x.obj.month);
            return;
        case DF_DAY:
            if (dated) { set_x_whole(c, x.obj.day); return; }
            if (t == OBJ_DR) { set_x_whole(c, x.obj.year); return; }
            e = DT_ERR_TYPE;
            break;
        case DF_HOUR: case DF_MINUTE: case DF_SECOND:
            if (t == OBJ_TIME || t == OBJ_DATETIME || t == OBJ_DZ || t == OBJ_DR) {
                set_x_whole(c, fn == DF_HOUR ? x.obj.hour :
                               fn == DF_MINUTE ? x.obj.minute : x.obj.second);
                return;
            }
            e = DT_ERR_TYPE;
            break;
        case DF_TZ_HOUR: case DF_TZ_MINUTE:
            if (t != OBJ_TZ && t != OBJ_DZ) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, fn == DF_TZ_HOUR ? x.obj.tz_hour : x.obj.tz_minute);
            return;
        case DF_GET_DATE: case DF_GET_TIME: case DF_GET_DT: case DF_GET_TZ:
            if (!(t == OBJ_DATETIME || t == OBJ_DZ) ||
                ((fn == DF_GET_DT || fn == DF_GET_TZ) && t != OBJ_DZ)) {
                e = DT_ERR_TYPE;
                break;
            }
            o = x.obj;
            o.type = (fn == DF_GET_DATE) ? OBJ_DATE : (fn == DF_GET_TIME) ? OBJ_TIME :
                     (fn == DF_GET_DT) ? OBJ_DATETIME : OBJ_TZ;
            if (o.type == OBJ_DATE)
                o.hour = o.minute = o.second = 0;
            if (o.type != OBJ_TZ)
                o.tz_hour = o.tz_minute = 0;
            set_x_obj(c, &o);
            return;

        /* Menu aliases require DATE operands and share the arithmetic rules. */
        case DF_SHRINK: case DF_EXTEND: case DF_CUT: case DF_LINK:
            calc_execute(c, CMD_DATE_VALUE, fn);
            return;

        /* ---- Time, TimeZone, Duration, DayOfWeek ---- */
        case DF_TIME_TO_S:
            if (t != OBJ_TIME) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, dt_time_seconds(&x.obj));
            return;
        case DF_S_TO_TIME:
            if (!value_is_plain(&c->stack[REG_X])) { e = DT_ERR_TYPE; break; }
            if (!x.is_whole) { e = DT_ERR_DOMAIN; break; }
            e = dt_time_from_seconds(x.whole, &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        case DF_TZ_TO_HR: {
            if (t != OBJ_TZ) { e = DT_ERR_TYPE; break; }
            real_t qq = os_Int24ToReal(dt_tz_quarters(&x.obj)), four = os_Int24ToReal(4);
            value_t h = value_real(os_RealDiv(&qq, &four));
            calc_stack_set_x(c, &h);
            return;
        }
        case DF_HR_TO_TZ:
            if (!value_is_plain(&c->stack[REG_X])) { e = DT_ERR_TYPE; break; }
            if (!x.is_quarter) { e = DT_ERR_DOMAIN; break; }
            e = dt_tz_from_quarters(x.quarters, &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        case DF_DAYS_TO_DR: case DF_HOURS_TO_DR: case DF_MINUTES_TO_DR: case DF_SECONDS_TO_DR: {
            static const dt_int unit[4] = { 86400, 3600, 60, 1 };
            if (!value_is_plain(&c->stack[REG_X])) { e = DT_ERR_TYPE; break; }
            if (!x.is_whole) { e = DT_ERR_DOMAIN; break; }
            e = dt_duration_from_seconds(x.whole * unit[fn - DF_DAYS_TO_DR], &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        }
        case DF_DR_TO_S:
            if (t != OBJ_DR) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, dt_duration_seconds(&x.obj));
            return;
        case DF_DW_TO_ISO: case DF_DW_TO_UNIX:
            if (t != OBJ_DW) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, fn == DF_DW_TO_ISO ? x.obj.day : x.obj.day % 7);
            return;
        case DF_ISO_TO_DW: case DF_UNIX_TO_DW: {
            if (!value_is_plain(&c->stack[REG_X])) { e = DT_ERR_TYPE; break; }
            dt_int lo = (fn == DF_ISO_TO_DW) ? 1 : 0;
            if (!x.is_whole || x.whole < lo || x.whole > lo + 6) { e = DT_ERR_DOMAIN; break; }
            o.type = OBJ_DW;
            o.day = (int8_t)(x.whole == 0 ? 7 : x.whole);
            set_x_obj(c, &o);
            return;
        }

        /* ---- epochs ---- */
        case DF_EPOCH_UNIX: case DF_EPOCH_NTP: case DF_EPOCH_GPS:
        case DF_EPOCH_TIOS: case DF_EPOCH_Y2K: case DF_EPOCH_CUSTOM:
            c->epoch = (uint8_t)(fn - DF_EPOCH_UNIX);
            return;
        case DF_EPOCH_SET_CUSTOM:
            if (t != OBJ_DATE) { e = DT_ERR_TYPE; break; }
            c->custom_epoch = x.obj;
            c->epoch = EPOCH_CUSTOM;
            c->message = "Epoch Set";
            return;
        case DF_EPOCH_GET_CUSTOM:
            push_obj(c, &c->custom_epoch);
            return;

        /* ---- clock ---- */
        case DF_NOW: case DF_NOW_DATE: case DF_NOW_TIME: case DF_NOW_DZ: case DF_NOW_DZ_UTC: {
            if (!calc_clock_now(c, fn == DF_NOW || fn == DF_NOW_DZ_UTC, &o)) {
                c->message = "Err: Clock";
                return;
            }
            if (fn == DF_NOW) {
                dt_to_epoch_seconds(&o, epoch_days(c), &n);
                value_t v = value_real(value_whole_real(n));
                calc_stack_enter(c, &v);
                return;
            }
            if (fn == DF_NOW_DATE) {
                o.type = OBJ_DATE;
                o.hour = o.minute = o.second = 0;
                o.tz_hour = o.tz_minute = 0;
            } else if (fn == DF_NOW_TIME) {
                o.type = OBJ_TIME;
                o.year = 0; o.month = o.day = 0;
                o.tz_hour = o.tz_minute = 0;
            }
            push_obj(c, &o);
            return;
        }
        case DF_SET_APP_TZ:
        case DF_SET_CLOCK_TZ:
            if (!tz_from_value(c, &x, &q))
                return;
            if (fn == DF_SET_APP_TZ)
                c->app_tz = (int8_t)q;
            else
                c->clock_tz = (int8_t)q;
            c->message = "Time Zone Set";
            return;
        case DF_GET_APP_TZ:
        case DF_GET_CLOCK_TZ:
            dt_tz_from_quarters(fn == DF_GET_APP_TZ ? c->app_tz : c->clock_tz, &o);
            push_obj(c, &o);
            return;
        case DF_SET_CLOCK:
            /* a ZonedDateTime, shown in the clock's own zone */
            if (t != OBJ_DZ && t != OBJ_DATETIME) { e = DT_ERR_TYPE; break; }
            if (t == OBJ_DATETIME) {            /* taken as already in the clock's zone */
                o = x.obj;
            } else {
                e = dt_convert_tz(&x.obj, c->clock_tz, &o);
                if (e)
                    break;
            }
            sys_clock_write(o.year, o.month, o.day, o.hour, o.minute, o.second);
            c->message = "Clock Set";
            return;

        /* ---- display ---- */
        case DF_FORMAT_RAW:
        case DF_FORMAT_STRING:
            c->date_as_string = (fn == DF_FORMAT_STRING);
            return;
    }
    if (e)
        c->message = value_date_error(e);
}

