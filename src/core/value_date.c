#include <string.h>
#include "value_date.h"
#include "mathx.h"

/* Whole numbers between reals and 64-bit integers (split into millions so
 * the OS's 24-bit conversions can handle each part). */
static bool real_to_whole(const real_t *v, dt_int *out)
{
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

real_t value_whole_real(dt_int n)
{
    real_t q = os_Int24ToReal((int)(n / 1000000));
    real_t r = os_Int24ToReal((int)(n % 1000000));
    real_t mil = os_Int24ToReal(1000000);
    real_t t = os_RealMul(&q, &mil);
    return os_RealAdd(&t, &r);
}

/* A stack value as a DATE operand. Only timezone operations need the
 * second conversion to quarter hours. */
dt_val_t value_date_operand(const value_t *v, bool quarters)
{
    dt_val_t d;
    memset(&d, 0, sizeof d);
    if (value_is_date(v)) {
        d.is_obj = true;
        d.obj = v->date;
        return d;
    }
    if (!value_is_plain(v)) return d;
    d.is_whole = real_to_whole(&v->real, &d.whole);
    if (quarters) {
        real_t four = os_Int24ToReal(4);
        real_t q = os_RealMul(&v->real, &four);
        d.is_quarter = real_to_whole(&q, &d.quarters);
    }
    return d;
}

value_t value_date_result(const dt_result_t *r)
{
    if (r->is_obj)
        return value_date(&r->obj);
    real_t n = value_whole_real(r->num);
    if (r->den == 1)
        return value_real(n);
    real_t d = os_Int24ToReal(r->den);
    return value_real(os_RealDiv(&n, &d));
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
static dt_err_t value_date_binary(calc_cmd_t op, const value_t *y, const value_t *x, value_t *out)
{
    if ((!value_is_plain(y) && !value_is_date(y)) ||
        (!value_is_plain(x) && !value_is_date(x)) || y->unit || x->unit)
        return DT_ERR_TYPE;
    dt_val_t a = value_date_operand(y, true), b = value_date_operand(x, true);
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
        *out = value_date_result(&r);
    return e;
}


/* Arithmetic and structural DATE operations share the universal evaluator
 * with numbers and quantities. Menu aliases call these same commands. */
dt_err_t value_date_eval(calc_cmd_t cmd, const value_t *y, const value_t *x, value_result_t *out)
{
    if (x->unit || ((command_flags(cmd) & COMMAND_DROP) && y->unit)) return DT_ERR_TYPE;
    if (cmd == CMD_ADD || cmd == CMD_SUB || cmd == CMD_MUL)
        return value_date_binary(cmd, y, x, &out->x);
    if (!value_is_date(x)) return DT_ERR_TYPE;
    dt_obj_t first = x->date, second = x->date;
    uint8_t t = first.type;
    switch (cmd) {
        case CMD_SQRT:
            if (t != OBJ_DATETIME && t != OBJ_DZ) return DT_ERR_TYPE;
            first.type = t == OBJ_DZ ? OBJ_DATETIME : OBJ_DATE;
            if (first.type == OBJ_DATE) first.hour = first.minute = first.second = 0;
            first.tz_hour = first.tz_minute = 0;
            break;
        case CMD_SQUARE:
            if (t != OBJ_DATE && t != OBJ_DATETIME) return DT_ERR_TYPE;
            first.type = t == OBJ_DATE ? OBJ_DATETIME : OBJ_DZ;
            break;
        case CMD_RECIP:
            if (t != OBJ_DATETIME && t != OBJ_DZ) return DT_ERR_TYPE;
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
                second.tz_hour = x->date.tz_hour; second.tz_minute = x->date.tz_minute;
            }
            out->y = value_date(&first); out->x = value_date(&second); out->pair = out->split = true;
            out->drop = false;
            return DT_OK;
        case CMD_COMPLEX_LINK: {
            if (!value_is_date(y) || y->unit) return DT_ERR_TYPE;
            const dt_obj_t *a = &y->date, *b = &x->date;
            if (a->type > b->type) { const dt_obj_t *swap = a; a = b; b = swap; }
            first = *a;
            if (a->type == OBJ_DATE && b->type == OBJ_TIME) {
                first.type = OBJ_DATETIME;
                first.hour = b->hour; first.minute = b->minute; first.second = b->second;
            } else if (a->type == OBJ_DATETIME && b->type == OBJ_TZ) {
                first.type = OBJ_DZ;
                first.tz_hour = b->tz_hour; first.tz_minute = b->tz_minute;
            } else return DT_ERR_TYPE;
            break;
        }
        default: return DT_ERR_TYPE;
    }
    out->x = value_date(&first);
    return DT_OK;
}
