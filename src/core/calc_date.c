/* Adapts DATE values and functions to the calculator stack and registers.
 * Calendar arithmetic and parsing remain in datetime.c. */
#include <string.h>
#include "calc_date.h"
#include "calc_internal.h"
#include "mathx.h"
#include "dateobj.h"

/* ---------- DATE ---------- */

/* Whole numbers between reals and 64-bit integers (split into millions so
 * the OS's 24-bit conversions can handle each part). */
static bool real_to_whole(const real_t *v, dt_int *out)
{
    if (obj_is(v))
        return false;
    real_t f = os_RealFrac(v);
    real_t lim = os_StrToReal("8E12", NULL);
    real_t a = mx_abs(v);
    if (!mx_is_zero(&f) || os_RealCompare(&a, &lim) >= 0)
        return false;
    real_t mil = os_Int24ToReal(1000000);
    real_t q = os_RealDiv(v, &mil);
    q = os_RealInt(&q);
    real_t qm = os_RealMul(&q, &mil);
    real_t r = os_RealSub(v, &qm);
    *out = (dt_int)os_RealToInt24(&q) * 1000000 + os_RealToInt24(&r);
    return true;
}

static real_t whole_to_real(dt_int n)
{
    real_t q = os_Int24ToReal((int)(n / 1000000));
    real_t r = os_Int24ToReal((int)(n % 1000000));
    real_t mil = os_Int24ToReal(1000000);
    real_t t = os_RealMul(&q, &mil);
    return os_RealAdd(&t, &r);
}

/* A stack value as a DATE operand. Only timezone operations need the
 * second conversion to quarter hours. */
static dt_val_t date_val(const real_t *v, bool quarters)
{
    dt_val_t d;
    memset(&d, 0, sizeof d);
    if (obj_is(v)) {
        d.is_obj = true;
        d.obj = obj_unpack(v);
        return d;
    }
    d.is_whole = real_to_whole(v, &d.whole);
    if (quarters) {
        real_t four = os_Int24ToReal(4);
        real_t q = os_RealMul(v, &four);
        d.is_quarter = real_to_whole(&q, &d.quarters);
    }
    return d;
}

void calc_date_error(calc_t *c, dt_err_t e)
{
    c->message = (e == DT_ERR_TYPE) ? "Err: Data Type" :
                 (e == DT_ERR_DOMAIN) ? "Err: Domain" :
                 (e == DT_ERR_SYNTAX) ? "Err: Syntax" : "Err: Invalid";
}

static real_t result_real(const dt_result_t *r)
{
    if (r->is_obj)
        return obj_pack(&r->obj);
    real_t n = whole_to_real(r->num);
    if (r->den == 1)
        return n;
    real_t d = os_Int24ToReal(r->den);
    return os_RealDiv(&n, &d);
}

/* CVTZ (also the x key): a Date, DateTime or ZonedDateTime and a TimeZone
 * or hours, in either order, give a ZonedDateTime in that zone. */
static dt_err_t date_cvtz(const dt_val_t *a, const dt_val_t *b, dt_result_t *out)
{
    const dt_val_t *when = NULL, *zone = NULL;
    for (int k = 0; k < 2; k++) {
        const dt_val_t *v = k ? b : a, *w = k ? a : b;
        if (v->is_obj && (v->obj.type == OBJ_DATE || v->obj.type == OBJ_DATETIME ||
                          v->obj.type == OBJ_DZ)) {
            when = v;
            zone = w;
        }
    }
    if (!when)
        return DT_ERR_TYPE;
    int q;
    if (zone->is_obj && zone->obj.type == OBJ_TZ)
        q = dt_tz_quarters(&zone->obj);
    else if (!zone->is_obj && zone->is_quarter)
        q = (int)zone->quarters;
    else
        return zone->is_obj ? DT_ERR_TYPE : DT_ERR_DOMAIN;
    out->is_obj = true;
    return dt_convert_tz(&when->obj, q, &out->obj);
}

/* y op x where either may be an object: + and -, and x for CVTZ. */
dt_err_t calc_date_op(calc_cmd_t op, const real_t *y, const real_t *x, real_t *out)
{
    dt_val_t a = date_val(y, true), b = date_val(x, true);
    dt_result_t r;
    dt_err_t e;
    if (op == CMD_ADD)
        e = dt_add(&a, &b, &r);
    else if (op == CMD_SUB)
        e = dt_sub(&a, &b, &r);
    else if (op == CMD_MUL)
        e = date_cvtz(&a, &b, &r);
    else
        e = DT_ERR_TYPE;
    if (e == DT_OK)
        *out = result_real(&r);
    return e;
}

/* y op x on the stack, for objects. */
void calc_date_binary(calc_t *c, calc_cmd_t op)
{
    real_t r;
    dt_err_t e = calc_date_op(op, &c->stack[1], &c->stack[0], &r);
    if (e) {
        calc_date_error(c, e);
        return;
    }
    calc_stack_set_result(c, &r, true);
}

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
    real_t v = obj_pack(o);
    calc_stack_set_x(c, &v);
}

static void set_x_whole(calc_t *c, dt_int n)
{
    real_t v = whole_to_real(n);
    calc_stack_set_x(c, &v);
}

static void push_obj(calc_t *c, const dt_obj_t *o)
{
    real_t v = obj_pack(o);
    calc_stack_enter(c, &v);
}

/* The hardware clock as a ZonedDateTime in the clock's zone. */
static bool clock_now(const calc_t *c, dt_obj_t *out)
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
    return dt_validate(out) == DT_OK;
}

/* A time zone from X: a TimeZone object or hours (multiples of 0.25). */
static bool tz_from_value(calc_t *c, const dt_val_t *v, int *q)
{
    if (v->is_obj && v->obj.type == OBJ_TZ) {
        *q = dt_tz_quarters(&v->obj);
        return true;
    }
    if (!v->is_obj && v->is_quarter && v->quarters >= -95 && v->quarters <= 95) {
        *q = (int)v->quarters;
        return true;
    }
    calc_date_error(c, v->is_obj ? DT_ERR_TYPE : DT_ERR_DOMAIN);
    return false;
}

void calc_date_execute(calc_t *c, date_fn_t fn)
{
    calc_finish_entry(c);
    dt_val_t x = date_val(&c->stack[0], fn == DF_HR_TO_TZ ||
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
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
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
            else if (!x.is_obj && x.is_whole && x.whole >= 1 && x.whole <= 9999)
                n = dt_is_leap((int)x.whole);
            else { e = x.is_obj ? DT_ERR_TYPE : DT_ERR_DOMAIN; break; }
            set_x_whole(c, n);
            return;
        case DF_DOW:
            if (!dated) { e = DT_ERR_TYPE; break; }
            o.type = OBJ_DW;
            o.day = (int8_t)dt_iso_dow(dt_days_from_civil(x.obj.year, x.obj.month, x.obj.day));
            set_x_obj(c, &o);
            return;
        case DF_CVTZ:
            calc_date_binary(c, CMD_MUL);
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

        /* ---- casting ---- */
        case DF_SHRINK:                         /* DT -> D, DZ -> DT */
            if (t != OBJ_DATETIME && t != OBJ_DZ) { e = DT_ERR_TYPE; break; }
            o = x.obj;
            o.type = (t == OBJ_DZ) ? OBJ_DATETIME : OBJ_DATE;
            if (o.type == OBJ_DATE)
                o.hour = o.minute = o.second = 0;
            o.tz_hour = o.tz_minute = 0;
            set_x_obj(c, &o);
            return;
        case DF_EXTEND:                         /* D -> DT (midnight), DT -> DZ (UTC) */
            if (t != OBJ_DATE && t != OBJ_DATETIME) { e = DT_ERR_TYPE; break; }
            o = x.obj;
            o.type = (t == OBJ_DATE) ? OBJ_DATETIME : OBJ_DZ;
            set_x_obj(c, &o);
            return;
        case DF_CUT: {                          /* DT -> D and T; DZ -> DT and TZ */
            if (t != OBJ_DATETIME && t != OBJ_DZ) { e = DT_ERR_TYPE; break; }
            dt_obj_t first = x.obj, second = x.obj;
            if (t == OBJ_DATETIME) {
                first.type = OBJ_DATE;
                first.hour = first.minute = first.second = 0;
                second.type = OBJ_TIME;
                second.year = 0; second.month = second.day = 0;
            } else {
                first.type = OBJ_DATETIME;
                first.tz_hour = first.tz_minute = 0;
                memset(&second, 0, sizeof second);
                second.type = OBJ_TZ;
                second.tz_hour = x.obj.tz_hour;
                second.tz_minute = x.obj.tz_minute;
            }
            set_x_obj(c, &first);               /* first in Y, second in X */
            real_t v = obj_pack(&second);
            calc_stack_push(c, &v);
            return;
        }
        case DF_LINK: {                         /* D + T -> DT; DT + TZ -> DZ */
            dt_val_t y = date_val(&c->stack[1], false);
            if (!x.is_obj || !y.is_obj) { e = DT_ERR_TYPE; break; }
            const dt_obj_t *a = &y.obj, *b = &x.obj;
            if (a->type > b->type) { const dt_obj_t *s = a; a = b; b = s; }
            if (a->type == OBJ_DATE && b->type == OBJ_TIME) {
                o = *a;
                o.type = OBJ_DATETIME;
                o.hour = b->hour; o.minute = b->minute; o.second = b->second;
            } else if (a->type == OBJ_DATETIME && b->type == OBJ_TZ) {
                o = *a;
                o.type = OBJ_DZ;
                o.tz_hour = b->tz_hour; o.tz_minute = b->tz_minute;
            } else {
                e = DT_ERR_TYPE;
                break;
            }
            real_t result = obj_pack(&o);
            calc_stack_set_result(c, &result, true);
            return;
        }

        /* ---- Time, TimeZone, Duration, DayOfWeek ---- */
        case DF_TIME_TO_S:
            if (t != OBJ_TIME) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, dt_time_seconds(&x.obj));
            return;
        case DF_S_TO_TIME:
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
            if (!x.is_whole) { e = DT_ERR_DOMAIN; break; }
            e = dt_time_from_seconds(x.whole, &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        case DF_TZ_TO_HR: {
            if (t != OBJ_TZ) { e = DT_ERR_TYPE; break; }
            real_t qq = os_Int24ToReal(dt_tz_quarters(&x.obj)), four = os_Int24ToReal(4);
            real_t h = os_RealDiv(&qq, &four);
            calc_stack_set_x(c, &h);
            return;
        }
        case DF_HR_TO_TZ:
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
            if (!x.is_quarter) { e = DT_ERR_DOMAIN; break; }
            e = dt_tz_from_quarters(x.quarters, &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        case DF_DAYS_TO_DR: case DF_HOURS_TO_DR: case DF_MINUTES_TO_DR: case DF_SECONDS_TO_DR: {
            static const dt_int unit[4] = { 86400, 3600, 60, 1 };
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
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
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
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
            dt_obj_t now;
            if (!clock_now(c, &now)) {
                c->message = "Err: Clock";
                return;
            }
            if (fn == DF_NOW) {
                dt_to_epoch_seconds(&now, epoch_days(c), &n);
                real_t v = whole_to_real(n);
                calc_stack_enter(c, &v);
                return;
            }
            dt_convert_tz(&now, fn == DF_NOW_DZ_UTC ? 0 : c->app_tz, &o);
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
        calc_date_error(c, e);
}

