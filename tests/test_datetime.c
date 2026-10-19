/* Tests for DATE objects, on a PC:
 *   cc -Isrc/core -o test_datetime tests/test_datetime.c src/core/datetime.c
 * Expected values come from RPN83P's DATE guide where it gives them.
 */
#include <stdio.h>
#include <string.h>
#include "datetime.h"

static int fails = 0;
static void ok(const char *name, int cond, const char *got)
{
    if (!cond) fails++;
    printf("%s %-46s %s\n", cond ? "ok  " : "FAIL", name, got ? got : "");
}

static dt_obj_t P(const char *s)
{
    dt_obj_t o;
    dt_err_t e = dt_parse(s, &o);
    if (e) { printf("FAIL parse %s: error %d\n", s, e); fails++; }
    return o;
}
static void fmt_is(const char *name, const dt_obj_t *o, bool str, const char *want)
{
    char buf[DT_FORMAT_MAX];
    dt_format(buf, o, str);
    char msg[80]; sprintf(msg, "%s (want %s)", buf, want);
    ok(name, strcmp(buf, want) == 0, msg);
}
static dt_val_t OBJ(const char *s) { dt_val_t v; memset(&v, 0, sizeof v); v.is_obj = true; v.obj = P(s); return v; }
static dt_val_t NUM(double d)
{
    dt_val_t v; memset(&v, 0, sizeof v);
    v.is_whole = (d == (long long)d); v.whole = (long long)d;
    v.is_quarter = (d * 4 == (long long)(d * 4)); v.quarters = (long long)(d * 4);
    return v;
}
static void res_obj(const char *name, dt_err_t e, const dt_result_t *r, const char *want)
{
    char buf[DT_FORMAT_MAX] = "(error)";
    if (!e && r->is_obj) dt_format(buf, &r->obj, false);
    char msg[80]; sprintf(msg, "%s (want %s)", buf, want);
    ok(name, !e && r->is_obj && strcmp(buf, want) == 0, msg);
}
static void res_num(const char *name, dt_err_t e, const dt_result_t *r, long long num, int den)
{
    char msg[80]; sprintf(msg, "%lld/%d (want %lld/%d)", e ? 0LL : (long long)r->num, e ? 0 : r->den, num, den);
    ok(name, !e && !r->is_obj && r->num == num && r->den == den, msg);
}

int main(void)
{
    dt_obj_t o, r;
    dt_result_t res;
    dt_int n;
    char msg[80];

    /* calendar */
    ok("1970-01-01 is day 0", dt_days_from_civil(1970, 1, 1) == 0, NULL);
    ok("0001-01-01 and 9999-12-31 round-trip", ({
        int y, m, d, good = 1;
        dt_civil_from_days(dt_days_from_civil(1, 1, 1), &y, &m, &d); good &= (y == 1 && m == 1 && d == 1);
        dt_civil_from_days(dt_days_from_civil(9999, 12, 31), &y, &m, &d); good &= (y == 9999 && m == 12 && d == 31);
        good; }), NULL);
    {
        int good = 1;      /* every day from 1600 to 2400 round-trips */
        for (dt_int z = dt_days_from_civil(1600, 1, 1); z < dt_days_from_civil(2400, 1, 1); z++) {
            int y, m, d;
            dt_civil_from_days(z, &y, &m, &d);
            if (dt_days_from_civil(y, m, d) != z || d < 1 || d > dt_days_in_month(y, m)) { good = 0; break; }
        }
        ok("292,194 days 1600-2400 round-trip", good, NULL);
    }
    ok("2024-03-14 is a Thursday", dt_iso_dow(dt_days_from_civil(2024, 3, 14)) == 4, NULL);
    ok("leap years: 2000 yes, 1900 no, 2024 yes", dt_is_leap(2000) && !dt_is_leap(1900) && dt_is_leap(2024), NULL);

    /* parsing and validation */
    o = P("{2024,3,14");            ok("{2024,3,14 is a Date (no closing brace)", o.type == OBJ_DATE, NULL);
    o = P("{2024,3,14,15,36,1}");   ok("6 fields: DateTime", o.type == OBJ_DATETIME, NULL);
    o = P("{2024,3,14,15,36,1,-7,0"); ok("8 fields: ZonedDateTime", o.type == OBJ_DZ, NULL);
    o = P("{5,30");                 ok("2 fields: TimeZone", o.type == OBJ_TZ, NULL);
    o = P("{1,2,3,4");              ok("4 fields: Duration", o.type == OBJ_DR, NULL);
    o = P("T{15,36,1}");            ok("T tag: Time", o.type == OBJ_TIME, NULL);
    o = P("DW{4}");                 ok("DW tag: DayOfWeek", o.type == OBJ_DW, NULL);
    ok("2024-02-30 is invalid", dt_parse("{2024,2,30", &o) == DT_ERR_INVALID, NULL);
    ok("2023-02-29 is invalid, 2024-02-29 valid",
       dt_parse("{2023,2,29", &o) == DT_ERR_INVALID && dt_parse("{2024,2,29", &o) == DT_OK, NULL);
    ok("T{23,59,60} is invalid", dt_parse("T{23,59,60}", &o) == DT_ERR_INVALID, NULL);
    ok("TZ{-7,30} (mixed signs) is invalid", dt_parse("TZ{-7,30}", &o) == DT_ERR_INVALID, NULL);
    ok("TZ{5,20} (not 15-minute) is invalid", dt_parse("TZ{5,20}", &o) == DT_ERR_INVALID, NULL);
    ok("DR{-1,0,0,3} (mixed signs) is invalid", dt_parse("DR{-1,0,0,3}", &o) == DT_ERR_INVALID, NULL);
    ok("5 fields with no tag is a syntax error", dt_parse("{1,2,3,4,5", &o) == DT_ERR_SYNTAX, NULL);
    ok("D tag with 2 fields is a syntax error", dt_parse("D{1,2}", &o) == DT_ERR_SYNTAX, NULL);
    ok("empty field is a syntax error", dt_parse("{2024,,14", &o) == DT_ERR_SYNTAX, NULL);
    o = P("1D3M");  fmt_is("compact 1D3M", &o, false, "DR{1,0,3,0}");
    o = P("2H4S");  fmt_is("compact 2H4S", &o, false, "DR{0,2,0,4}");
    o = P("90M");   fmt_is("compact 90M normalizes", &o, false, "DR{0,1,30,0}");

    /* formatting */
    o = P("{2024,3,14");                fmt_is("Date as string", &o, true, "2024-03-14");
    o = P("T{15,36,1}");                fmt_is("Time as string", &o, true, "15:36:01");
    o = P("{2024,3,14,15,36,1");        fmt_is("DateTime as string", &o, true, "2024-03-14 15:36:01");
    o = P("{2024,3,14,15,36,1,-7,0");   fmt_is("ZonedDateTime as string", &o, true, "2024-03-14 15:36:01-07:00");
    o = P("{2024,3,14,22,36,0,0,0");    fmt_is("UTC ZonedDateTime uses Z", &o, true, "2024-03-14 22:36:00Z");
    o = P("{-4,-30");                   fmt_is("TimeZone as string", &o, true, "UTC-04:30");
    o = P("DW{4}");                     fmt_is("DayOfWeek as string", &o, true, "Thu");
    o = P("{1,0,3,0");                  fmt_is("Duration as string (zeros omitted)", &o, true, "1d 3m");
    o = P("DR{-1,-2,0,0}");             fmt_is("negative Duration", &o, true, "-1d 2h");
    o = P("{2024,3,14,15,36,1,-7,0");   fmt_is("ZonedDateTime raw", &o, false, "DZ{2024,3,14,15,36,1,-7,0}");

    /* epochs */
    o = P("{2024,3,14,15,36,1,-7,0");
    dt_to_epoch_seconds(&o, 0, &n);
    sprintf(msg, "%lld", (long long)n); ok("Unix epoch seconds = 1710455761", n == 1710455761LL, msg);
    dt_to_epoch_seconds(&o, dt_days_from_civil(1900, 1, 1), &n);
    sprintf(msg, "%lld", (long long)n); ok("NTP epoch seconds = 3919444561", n == 3919444561LL, msg);
    dt_from_epoch_seconds(1710455761LL, 0, OBJ_DZ, -28, &r);
    fmt_is("ES> back at UTC-07:00", &r, true, "2024-03-14 15:36:01-07:00");
    dt_from_epoch_seconds(1710455761LL, 0, OBJ_DATE, 0, &r);
    fmt_is("ES> as a Date (UTC)", &r, true, "2024-03-14");

    /* Reported CE arithmetic failure and wide seconds after the day helper
     * was narrowed: late dates must not overflow a 32-bit intermediate. */
    {
        dt_val_t born = OBJ("{1987,01,17"), today = OBJ("{2026,10,19"), days = NUM(15000);
        res_obj("1987-01-17 + 15000 days", dt_add(&born, &days, &res), &res, "D{2028,2,11}");
        res_num("birth date - today = -14520 days", dt_sub(&born, &today, &res), &res, -14520, 1);
        res_num("today - birth date = 14520 days", dt_sub(&today, &born, &res), &res, 14520, 1);
        o = P("{9999,12,31");
        dt_to_epoch_seconds(&o, 0, &n);
        ok("9999-12-31 keeps wide epoch seconds", n == 253402214400LL, NULL);
        o = P("{1,1,1");
        dt_to_epoch_seconds(&o, 0, &n);
        ok("0001-01-01 keeps negative epoch seconds", n == -62135596800LL, NULL);
    }

    /* RPN83P's arithmetic examples */
    dt_val_t d = OBJ("{2024,3,14"), num;
    num = NUM(20);  res_obj("2024-03-14 + 20 days", dt_add(&d, &num, &res), &res, "D{2024,4,3}");
    dt_val_t d2 = { .is_obj = true, .obj = res.obj };
    num = NUM(30);  res_obj("... - 30 days", dt_sub(&d2, &num, &res), &res, "D{2024,3,4}");
    dt_val_t xmas = OBJ("{2024,12,25");
    res_num("Christmas - March 14 = 286 days", dt_sub(&xmas, &d, &res), &res, 286, 1);
    num = NUM(20);  res_obj("20 + Date (either order)", dt_add(&num, &d, &res), &res, "D{2024,4,3}");
    ok("number - Date is invalid", dt_sub(&num, &d, &res) == DT_ERR_TYPE, NULL);
    num = NUM(1.5); ok("Date + 1.5 is a domain error", dt_add(&d, &num, &res) == DT_ERR_DOMAIN, NULL);

    dt_val_t t = OBJ("T{17,0,0}"), t2 = OBJ("T{15,36,1}");
    res_num("17:00:00 - 15:36:01 = 5039 s", dt_sub(&t, &t2, &res), &res, 5039, 1);
    num = NUM(-100000); res_obj("Time wraps around midnight", dt_add(&t2, &num, &res), &res, "T{11,49,21}");

    dt_val_t dta = OBJ("{2024,12,25,0,0,0"), dtb = OBJ("{2024,3,14,15,36,1");
    res_num("DateTime difference = 24654239 s", dt_sub(&dta, &dtb, &res), &res, 24654239LL, 1);
    dt_duration_from_seconds(24654239LL, &r);
    fmt_is("... as a Duration", &r, false, "DR{285,8,23,59}");

    dt_val_t tz = OBJ("TZ{-8,0}");
    num = NUM(1);   res_obj("UTC-08:00 + 1 hour", dt_add(&tz, &num, &res), &res, "TZ{-7,0}");
    dt_val_t tz1 = OBJ("TZ{1,0}");
    res_num("UTC+01:00 - UTC-08:00 = 9 h", dt_sub(&tz1, &tz, &res), &res, 36, 4);
    num = NUM(0.3); ok("TimeZone + 0.3 h is a domain error", dt_add(&tz, &num, &res) == DT_ERR_DOMAIN, NULL);

    dt_val_t dw = OBJ("DW{4}");
    num = NUM(6);   res_obj("Thursday + 6 days = Wednesday", dt_add(&dw, &num, &res), &res, "DW{3}");

    dt_val_t dr = OBJ("DR{0,2,33,0}"), tm = OBJ("T{12,58,32}");
    res_obj("12:58:32 + 2h 33m", dt_add(&tm, &dr, &res), &res, "T{15,31,32}");
    dt_val_t dr2 = OBJ("DR{30,0,0,37}"), dtc = OBJ("{2024,3,14,12,58,32");
    res_obj("DateTime + 30d 37s", dt_add(&dtc, &dr2, &res), &res, "DT{2024,4,13,12,59,9}");
    num = NUM(100); res_obj("100 - Duration is a Duration", dt_sub(&num, &dr2, &res), &res, "DR{-29,-23,-58,-57}");

    /* time zone conversion */
    o = P("{2024,3,14,15,36,1,-7,0");
    dt_convert_tz(&o, 13 * 4, &r);  fmt_is("CVTZ to UTC+13:00 (Auckland)", &r, true, "2024-03-15 11:36:01+13:00");
    dt_convert_tz(&o, 22, &r);      fmt_is("CVTZ to UTC+05:30 (India)", &r, true, "2024-03-15 04:06:01+05:30");
    dt_convert_tz(&o, -10, &r);     fmt_is("CVTZ to UTC-02:30 (Newfoundland)", &r, true, "2024-03-14 20:06:01-02:30");
    o = P("{2024,3,14");
    dt_convert_tz(&o, -28, &r);     fmt_is("CVTZ of a Date (UTC midnight)", &r, true, "2024-03-13 17:00:00-07:00");

    printf("%d failures\n", fails);
    return fails != 0;
}
