#include <string.h>

#include "datetime.h"

#define SECS_PER_DAY 86400

/* ---------- calendar ---------- */

bool dt_is_leap(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int dt_days_in_month(int year, int month)
{
    static const uint8_t days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && dt_is_leap(year))
        return 29;
    return days[month - 1];
}

/* Howard Hinnant's days_from_civil: days since 1970-01-01. */
dt_int dt_days_from_civil(int y, int m, int d)
{
    y -= (m <= 2);
    dt_int era = (y >= 0 ? y : y - 399) / 400;
    dt_int yoe = y - era * 400;                                 /* [0, 399] */
    dt_int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; /* [0, 365] */
    dt_int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;         /* [0, 146096] */
    return era * 146097 + doe - 719468;
}

void dt_civil_from_days(dt_int z, int *year, int *month, int *day)
{
    z += 719468;
    dt_int era = (z >= 0 ? z : z - 146096) / 146097;
    dt_int doe = z - era * 146097;
    dt_int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    dt_int y = yoe + era * 400;
    dt_int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    dt_int mp = (5 * doy + 2) / 153;
    int d = (int)(doy - (153 * mp + 2) / 5 + 1);
    int m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *year = (int)(y + (m <= 2));
    *month = m;
    *day = d;
}

/* 1970-01-01 was a Thursday (ISO 4). */
int dt_iso_dow(dt_int days)
{
    dt_int r = (days + 3) % 7;      /* Monday = 0 */
    if (r < 0)
        r += 7;
    return (int)r + 1;
}

/* Floor division and modulo, for negative seconds. */
static dt_int floor_div(dt_int a, dt_int b)
{
    dt_int q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0)))
        q--;
    return q;
}

static dt_int floor_mod(dt_int a, dt_int b)
{
    return a - floor_div(a, b) * b;
}

/* ---------- validation ---------- */

static bool valid_date(int y, int m, int d)
{
    return y >= 1 && y <= 9999 && m >= 1 && m <= 12 &&
           d >= 1 && d <= dt_days_in_month(y, m);
}

static bool valid_time(int h, int m, int s)
{
    return h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59;
}

static bool same_sign(int a, int b)
{
    return !((a > 0 && b < 0) || (a < 0 && b > 0));
}

static bool valid_tz(int h, int m)
{
    return h >= -23 && h <= 23 && m >= -45 && m <= 45 && m % 15 == 0 &&
           same_sign(h, m);
}

dt_err_t dt_validate(const dt_obj_t *o)
{
    bool ok;
    switch (o->type) {
        case OBJ_DATE:
            ok = valid_date(o->year, o->month, o->day);
            break;
        case OBJ_TIME:
            ok = valid_time(o->hour, o->minute, o->second);
            break;
        case OBJ_DATETIME:
            ok = valid_date(o->year, o->month, o->day) &&
                 valid_time(o->hour, o->minute, o->second);
            break;
        case OBJ_TZ:
            ok = valid_tz(o->tz_hour, o->tz_minute);
            break;
        case OBJ_DZ:
            ok = valid_date(o->year, o->month, o->day) &&
                 valid_time(o->hour, o->minute, o->second) &&
                 valid_tz(o->tz_hour, o->tz_minute);
            break;
        case OBJ_DW:
            ok = o->day >= 1 && o->day <= 7;
            break;
        case OBJ_DR: {
            int d = o->year, h = o->hour, m = o->minute, s = o->second;
            bool neg = d < 0 || h < 0 || m < 0 || s < 0;
            bool pos = d > 0 || h > 0 || m > 0 || s > 0;
            ok = d >= -9999 && d <= 9999 && h >= -23 && h <= 23 &&
                 m >= -59 && m <= 59 && s >= -59 && s <= 59 && !(neg && pos);
            break;
        }
        default:
            ok = false;
    }
    return ok ? DT_OK : DT_ERR_INVALID;
}

/* ---------- parsing ---------- */

static const struct { const char *tag; uint8_t type; uint8_t arity; } tags[] = {
    { "D", OBJ_DATE, 3 }, { "T", OBJ_TIME, 3 }, { "DT", OBJ_DATETIME, 6 },
    { "TZ", OBJ_TZ, 2 }, { "DZ", OBJ_DZ, 8 }, { "DW", OBJ_DW, 1 },
    { "DR", OBJ_DR, 4 },
};

bool dt_text_is_object(const char *text)
{
    for (const char *p = text; *p; p++)
        if (*p == '{' || *p == 'D' || *p == 'H' || *p == 'M' || *p == 'S' ||
            *p == 'T' || *p == 'Z' || *p == 'W' || *p == 'R')
            return true;
    return false;
}

/* Compact durations: digits followed by D, H, M or S, in any combination
 * ("1D2H", "90M", "-3D"). */
static dt_err_t parse_compact(const char *p, dt_obj_t *out)
{
    bool neg = false;
    long fields[4] = { 0, 0, 0, 0 };    /* D H M S */
    bool any = false;
    if (*p == '-') {
        neg = true;
        p++;
    }
    while (*p) {
        long n = 0;
        int digits = 0;
        while (*p >= '0' && *p <= '9') {
            n = n * 10 + (*p++ - '0');
            if (++digits > 5)
                return DT_ERR_INVALID;
        }
        if (!digits)
            return DT_ERR_SYNTAX;
        int k = (*p == 'D') ? 0 : (*p == 'H') ? 1 : (*p == 'M') ? 2 : (*p == 'S') ? 3 : -1;
        if (k < 0)
            return DT_ERR_SYNTAX;
        fields[k] += n;
        any = true;
        p++;
    }
    if (!any)
        return DT_ERR_SYNTAX;
    dt_int secs = ((dt_int)fields[0] * 24 + fields[1]) * 3600 +
                  (dt_int)fields[2] * 60 + fields[3];
    return dt_duration_from_seconds(neg ? -secs : secs, out);
}

dt_err_t dt_parse(const char *text, dt_obj_t *out)
{
    memset(out, 0, sizeof *out);
    const char *brace = strchr(text, '{');
    if (!brace)
        return parse_compact(text, out);

    /* the tag before the brace */
    char tag[4];
    size_t tl = (size_t)(brace - text);
    if (tl > 2)
        return DT_ERR_SYNTAX;
    memcpy(tag, text, tl);
    tag[tl] = '\0';

    /* the fields */
    long f[8];
    int n = 0;
    const char *p = brace + 1;
    for (;;) {
        bool neg = false;
        long v = 0;
        int digits = 0;
        if (*p == '-') {
            neg = true;
            p++;
        }
        while (*p >= '0' && *p <= '9') {
            v = v * 10 + (*p++ - '0');
            if (++digits > 5)
                return DT_ERR_INVALID;
        }
        if (!digits)
            return DT_ERR_SYNTAX;
        if (n == 8)
            return DT_ERR_SYNTAX;
        f[n++] = neg ? -v : v;
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == '}')
            p++;
        if (*p != '\0')
            return DT_ERR_SYNTAX;
        break;
    }

    uint8_t type = OBJ_NONE;
    if (tl == 0) {
        switch (n) {                    /* type from the number of fields */
            case 2: type = OBJ_TZ; break;
            case 3: type = OBJ_DATE; break;
            case 4: type = OBJ_DR; break;
            case 6: type = OBJ_DATETIME; break;
            case 8: type = OBJ_DZ; break;
            default: return DT_ERR_SYNTAX;
        }
    } else {
        for (unsigned i = 0; i < sizeof tags / sizeof *tags; i++)
            if (strcmp(tag, tags[i].tag) == 0) {
                if (n != tags[i].arity)
                    return DT_ERR_SYNTAX;
                type = tags[i].type;
            }
        if (type == OBJ_NONE)
            return DT_ERR_SYNTAX;
    }

    /* fields fit int8 except a year or day count */
    for (int i = 0; i < n; i++) {
        bool wide = (i == 0) && (type == OBJ_DATE || type == OBJ_DATETIME ||
                                 type == OBJ_DZ || type == OBJ_DR);
        if (!wide && (f[i] < -128 || f[i] > 127))
            return DT_ERR_INVALID;
        if (wide && (f[i] < -32768 || f[i] > 32767))
            return DT_ERR_INVALID;
    }

    out->type = type;
    switch (type) {
        case OBJ_DATE:
            out->year = (int16_t)f[0]; out->month = (int8_t)f[1]; out->day = (int8_t)f[2];
            break;
        case OBJ_TIME:
            out->hour = (int8_t)f[0]; out->minute = (int8_t)f[1]; out->second = (int8_t)f[2];
            break;
        case OBJ_DATETIME:
        case OBJ_DZ:
            out->year = (int16_t)f[0]; out->month = (int8_t)f[1]; out->day = (int8_t)f[2];
            out->hour = (int8_t)f[3]; out->minute = (int8_t)f[4]; out->second = (int8_t)f[5];
            if (type == OBJ_DZ) {
                out->tz_hour = (int8_t)f[6];
                out->tz_minute = (int8_t)f[7];
            }
            break;
        case OBJ_TZ:
            out->tz_hour = (int8_t)f[0]; out->tz_minute = (int8_t)f[1];
            break;
        case OBJ_DW:
            out->day = (int8_t)f[0];
            break;
        case OBJ_DR:
            out->year = (int16_t)f[0]; out->hour = (int8_t)f[1];
            out->minute = (int8_t)f[2]; out->second = (int8_t)f[3];
            break;
    }
    return dt_validate(out);
}

/* ---------- formatting ---------- */

/* Small number writers (printf is large on the calculator). Each returns
 * the end of what it wrote. */
static char *put_int(char *p, int v)
{
    char tmp[8];
    int n = 0;
    if (v < 0) {
        *p++ = '-';
        v = -v;
    }
    do {
        tmp[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n)
        *p++ = tmp[--n];
    return p;
}

static char *put_padded(char *p, int v, int width)
{
    char tmp[8];
    int n = 0;
    do {
        tmp[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n < width)
        tmp[n++] = '0';
    while (n)
        *p++ = tmp[--n];
    return p;
}

static char *put_str(char *p, const char *s)
{
    while (*s)
        *p++ = *s++;
    return p;
}

/* "D{" then the fields with commas, then "}" */
static char *put_fields(char *p, const char *tag, const int *f, int n)
{
    p = put_str(p, tag);
    *p++ = '{';
    for (int i = 0; i < n; i++) {
        if (i)
            *p++ = ',';
        p = put_int(p, f[i]);
    }
    *p++ = '}';
    return p;
}

/* "+05:30" or "-07:00" */
static char *put_tz(char *p, int h, int m)
{
    bool neg = h < 0 || m < 0;
    *p++ = neg ? '-' : '+';
    p = put_padded(p, neg ? -h : h, 2);
    *p++ = ':';
    return put_padded(p, neg ? -m : m, 2);
}

static char *put_date(char *p, const dt_obj_t *o)
{
    p = put_padded(p, o->year, 4);
    *p++ = '-';
    p = put_padded(p, o->month, 2);
    *p++ = '-';
    return put_padded(p, o->day, 2);
}

static char *put_time(char *p, const dt_obj_t *o)
{
    p = put_padded(p, o->hour, 2);
    *p++ = ':';
    p = put_padded(p, o->minute, 2);
    *p++ = ':';
    return put_padded(p, o->second, 2);
}

void dt_format(char *out, const dt_obj_t *o, bool as_string)
{
    static const char dow[7][4] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    char *p = out;

    if (!as_string) {
        int f[8] = { o->year, o->month, o->day, o->hour, o->minute, o->second,
                     o->tz_hour, o->tz_minute };
        int t[3] = { o->hour, o->minute, o->second };
        int z[2] = { o->tz_hour, o->tz_minute };
        int r[4] = { o->year, o->hour, o->minute, o->second };
        int w[1] = { o->day };
        switch (o->type) {
            case OBJ_DATE:     p = put_fields(p, "D", f, 3); break;
            case OBJ_TIME:     p = put_fields(p, "T", t, 3); break;
            case OBJ_DATETIME: p = put_fields(p, "DT", f, 6); break;
            case OBJ_TZ:       p = put_fields(p, "TZ", z, 2); break;
            case OBJ_DZ:       p = put_fields(p, "DZ", f, 8); break;
            case OBJ_DW:       p = put_fields(p, "DW", w, 1); break;
            case OBJ_DR:       p = put_fields(p, "DR", r, 4); break;
            default:           *p++ = '?'; break;
        }
        *p = '\0';
        return;
    }

    switch (o->type) {
        case OBJ_DATE:
            p = put_date(p, o);
            break;
        case OBJ_TIME:
            p = put_time(p, o);
            break;
        case OBJ_DATETIME:
        case OBJ_DZ:
            p = put_date(p, o);
            *p++ = ' ';
            p = put_time(p, o);
            if (o->type == OBJ_DZ) {
                if (o->tz_hour == 0 && o->tz_minute == 0)
                    *p++ = 'Z';
                else
                    p = put_tz(p, o->tz_hour, o->tz_minute);
            }
            break;
        case OBJ_TZ:
            p = put_tz(put_str(p, "UTC"), o->tz_hour, o->tz_minute);
            break;
        case OBJ_DW:
            p = put_str(p, (o->day >= 1 && o->day <= 7) ? dow[o->day - 1] : "?");
            break;
        case OBJ_DR: {
            int v[4] = { o->year, o->hour, o->minute, o->second };
            static const char unit[4] = { 'd', 'h', 'm', 's' };
            bool neg = v[0] < 0 || v[1] < 0 || v[2] < 0 || v[3] < 0;
            bool any = false;
            if (neg)
                *p++ = '-';
            for (int i = 0; i < 4; i++) {
                if (!v[i])
                    continue;
                if (any)
                    *p++ = ' ';
                p = put_int(p, neg ? -v[i] : v[i]);
                *p++ = unit[i];
                any = true;
            }
            if (!any) {
                p = out;
                p = put_str(p, "0s");
            }
            break;
        }
        default:
            *p++ = '?';
            break;
    }
    *p = '\0';
}

/* ---------- conversions ---------- */

static dt_int date_days(const dt_obj_t *o)
{
    return dt_days_from_civil(o->year, o->month, o->day);
}

static dt_int clock_seconds(const dt_obj_t *o)
{
    return (dt_int)o->hour * 3600 + o->minute * 60 + o->second;
}

int dt_tz_quarters(const dt_obj_t *tz)
{
    return tz->tz_hour * 4 + tz->tz_minute / 15;
}

dt_err_t dt_tz_from_quarters(dt_int q, dt_obj_t *out)
{
    if (q < -95 || q > 95)
        return DT_ERR_DOMAIN;
    memset(out, 0, sizeof *out);
    out->type = OBJ_TZ;
    out->tz_hour = (int8_t)(q / 4);
    out->tz_minute = (int8_t)((q % 4) * 15);
    return DT_OK;
}

/* Seconds since 1970-01-01 UTC. */
static dt_err_t utc_seconds(const dt_obj_t *o, dt_int *out)
{
    switch (o->type) {
        case OBJ_DATE:
            *out = date_days(o) * SECS_PER_DAY;
            return DT_OK;
        case OBJ_DATETIME:
            *out = date_days(o) * SECS_PER_DAY + clock_seconds(o);
            return DT_OK;
        case OBJ_DZ:
            *out = date_days(o) * SECS_PER_DAY + clock_seconds(o) -
                   (dt_int)(o->tz_hour * 60 + o->tz_minute) * 60;
            return DT_OK;
        default:
            return DT_ERR_TYPE;
    }
}

dt_err_t dt_to_epoch_seconds(const dt_obj_t *o, dt_int epoch_days, dt_int *out)
{
    dt_int s;
    dt_err_t e = utc_seconds(o, &s);
    if (e)
        return e;
    *out = s - epoch_days * SECS_PER_DAY;
    return DT_OK;
}

/* Fills the date and time fields from local seconds since 1970-01-01. */
static dt_err_t fill_local(dt_obj_t *o, dt_int local)
{
    int y, m, d;
    dt_int days = floor_div(local, SECS_PER_DAY);
    dt_int rem = floor_mod(local, SECS_PER_DAY);
    dt_civil_from_days(days, &y, &m, &d);
    if (y < 1 || y > 9999)
        return DT_ERR_DOMAIN;
    o->year = (int16_t)y;
    o->month = (int8_t)m;
    o->day = (int8_t)d;
    o->hour = (int8_t)(rem / 3600);
    o->minute = (int8_t)(rem / 60 % 60);
    o->second = (int8_t)(rem % 60);
    return DT_OK;
}

dt_err_t dt_from_epoch_seconds(dt_int secs, dt_int epoch_days, obj_type_t type,
                               int tz_quarters, dt_obj_t *out)
{
    memset(out, 0, sizeof *out);
    out->type = (uint8_t)type;
    dt_int utc = secs + epoch_days * SECS_PER_DAY;
    dt_int local = (type == OBJ_DZ) ? utc + (dt_int)tz_quarters * 900 : utc;
    dt_err_t e = fill_local(out, local);
    if (e)
        return e;
    if (type == OBJ_DATE)
        out->hour = out->minute = out->second = 0;
    if (type == OBJ_DZ) {
        out->tz_hour = (int8_t)(tz_quarters / 4);
        out->tz_minute = (int8_t)((tz_quarters % 4) * 15);
    }
    return DT_OK;
}

dt_int dt_time_seconds(const dt_obj_t *t)
{
    return clock_seconds(t);
}

dt_err_t dt_time_from_seconds(dt_int secs, dt_obj_t *out)
{
    if (secs < 0 || secs >= SECS_PER_DAY)
        return DT_ERR_DOMAIN;
    memset(out, 0, sizeof *out);
    out->type = OBJ_TIME;
    out->hour = (int8_t)(secs / 3600);
    out->minute = (int8_t)(secs / 60 % 60);
    out->second = (int8_t)(secs % 60);
    return DT_OK;
}

dt_int dt_duration_seconds(const dt_obj_t *d)
{
    return (dt_int)d->year * SECS_PER_DAY + (dt_int)d->hour * 3600 +
           d->minute * 60 + d->second;
}

dt_err_t dt_duration_from_seconds(dt_int secs, dt_obj_t *out)
{
    bool neg = secs < 0;
    dt_int a = neg ? -secs : secs;
    dt_int days = a / SECS_PER_DAY;
    if (days > 9999)
        return DT_ERR_DOMAIN;
    dt_int rem = a % SECS_PER_DAY;
    int s = neg ? -1 : 1;
    memset(out, 0, sizeof *out);
    out->type = OBJ_DR;
    out->year = (int16_t)(s * days);
    out->hour = (int8_t)(s * (rem / 3600));
    out->minute = (int8_t)(s * (rem / 60 % 60));
    out->second = (int8_t)(s * (rem % 60));
    return DT_OK;
}

dt_err_t dt_convert_tz(const dt_obj_t *o, int tz_quarters, dt_obj_t *out)
{
    dt_int utc;
    dt_err_t e = utc_seconds(o, &utc);
    if (e)
        return e;
    if (tz_quarters < -95 || tz_quarters > 95)
        return DT_ERR_DOMAIN;
    return dt_from_epoch_seconds(utc, 0, OBJ_DZ, tz_quarters, out);
}

/* ---------- arithmetic ---------- */

static bool is_num(const dt_val_t *v) { return !v->is_obj; }
static bool is_type(const dt_val_t *v, uint8_t t) { return v->is_obj && v->obj.type == t; }

static void result_obj(dt_result_t *r, const dt_obj_t *o)
{
    r->is_obj = true;
    r->obj = *o;
}

static void result_num(dt_result_t *r, dt_int n, int den)
{
    r->is_obj = false;
    r->num = n;
    r->den = den;
}

/* How many units (days or seconds) a number or Duration operand adds to an
 * object of type `t`; false if it can't be used. */
static dt_err_t amount_for(uint8_t t, const dt_val_t *v, dt_int *out)
{
    if (is_type(v, OBJ_DR)) {
        dt_int s = dt_duration_seconds(&v->obj);
        if (t == OBJ_DATE || t == OBJ_DW)
            *out = floor_div(s, SECS_PER_DAY);      /* whole days */
        else if (t == OBJ_TZ)
            *out = s / 900;                         /* quarter hours */
        else
            *out = s;
        return DT_OK;
    }
    if (!is_num(v))
        return DT_ERR_TYPE;
    if (t == OBJ_TZ) {
        if (!v->is_quarter)
            return DT_ERR_DOMAIN;
        *out = v->quarters;
        return DT_OK;
    }
    if (!v->is_whole)
        return DT_ERR_DOMAIN;
    *out = v->whole;
    return DT_OK;
}

/* object + amount (units as in amount_for) */
static dt_err_t shift(const dt_obj_t *o, dt_int amount, dt_result_t *out)
{
    dt_obj_t r = *o;
    dt_err_t e = DT_OK;
    switch (o->type) {
        case OBJ_DATE: {
            int y, m, d;
            dt_civil_from_days(date_days(o) + amount, &y, &m, &d);
            if (y < 1 || y > 9999)
                return DT_ERR_DOMAIN;
            r.year = (int16_t)y; r.month = (int8_t)m; r.day = (int8_t)d;
            break;
        }
        case OBJ_TIME:
            e = dt_time_from_seconds(floor_mod(dt_time_seconds(o) + amount, SECS_PER_DAY), &r);
            break;
        case OBJ_DATETIME:
        case OBJ_DZ: {
            dt_int local = date_days(o) * SECS_PER_DAY + clock_seconds(o) + amount;
            e = fill_local(&r, local);          /* the offset stays the same */
            break;
        }
        case OBJ_TZ:
            e = dt_tz_from_quarters(dt_tz_quarters(o) + amount, &r);
            break;
        case OBJ_DW:
            r.day = (int8_t)(floor_mod(o->day - 1 + amount, 7) + 1);
            break;
        case OBJ_DR:
            e = dt_duration_from_seconds(dt_duration_seconds(o) + amount, &r);
            break;
        default:
            return DT_ERR_TYPE;
    }
    if (e)
        return e;
    result_obj(out, &r);
    return DT_OK;
}

dt_err_t dt_add(const dt_val_t *y, const dt_val_t *x, dt_result_t *out)
{
    /* Duration + Duration */
    if (is_type(y, OBJ_DR) && is_type(x, OBJ_DR))
        return shift(&y->obj, dt_duration_seconds(&x->obj), out);

    /* object + number or Duration, either way round */
    const dt_val_t *obj = NULL, *amt = NULL;
    if (y->is_obj && !is_type(y, OBJ_DR)) {
        obj = y; amt = x;
    } else if (x->is_obj && !is_type(x, OBJ_DR)) {
        obj = x; amt = y;
    } else if (is_type(y, OBJ_DR) && is_num(x)) {
        obj = y; amt = x;
    } else if (is_type(x, OBJ_DR) && is_num(y)) {
        obj = x; amt = y;
    }
    if (!obj || (amt->is_obj && !is_type(amt, OBJ_DR)))
        return DT_ERR_TYPE;

    dt_int a;
    dt_err_t e = amount_for(obj->obj.type, amt, &a);
    if (e)
        return e;
    return shift(&obj->obj, a, out);
}

dt_err_t dt_sub(const dt_val_t *y, const dt_val_t *x, dt_result_t *out)
{
    /* number - Duration is a Duration; number - other objects is invalid */
    if (is_num(y)) {
        if (!is_type(x, OBJ_DR))
            return DT_ERR_TYPE;
        if (!y->is_whole)
            return DT_ERR_DOMAIN;
        dt_obj_t r;
        dt_err_t e = dt_duration_from_seconds(y->whole - dt_duration_seconds(&x->obj), &r);
        if (e)
            return e;
        result_obj(out, &r);
        return DT_OK;
    }

    uint8_t t = y->obj.type;

    /* the same type: the difference */
    if (x->is_obj && x->obj.type == t) {
        dt_int a, b;
        switch (t) {
            case OBJ_DATE:
                result_num(out, date_days(&y->obj) - date_days(&x->obj), 1);
                return DT_OK;
            case OBJ_TIME:
                result_num(out, dt_time_seconds(&y->obj) - dt_time_seconds(&x->obj), 1);
                return DT_OK;
            case OBJ_DATETIME:
            case OBJ_DZ:
                utc_seconds(&y->obj, &a);
                utc_seconds(&x->obj, &b);
                result_num(out, a - b, 1);
                return DT_OK;
            case OBJ_TZ:                         /* hours, maybe fractional */
                result_num(out, dt_tz_quarters(&y->obj) - dt_tz_quarters(&x->obj), 4);
                return DT_OK;
            case OBJ_DW:
                result_num(out, y->obj.day - x->obj.day, 1);
                return DT_OK;
            case OBJ_DR: {
                dt_obj_t r;
                dt_err_t e = dt_duration_from_seconds(
                    dt_duration_seconds(&y->obj) - dt_duration_seconds(&x->obj), &r);
                if (e)
                    return e;
                result_obj(out, &r);
                return DT_OK;
            }
        }
    }

    /* object - number or Duration */
    if (x->is_obj && !is_type(x, OBJ_DR))
        return DT_ERR_TYPE;
    dt_int a;
    dt_err_t e = amount_for(t, x, &a);
    if (e)
        return e;
    return shift(&y->obj, -a, out);
}
