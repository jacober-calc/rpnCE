#ifndef RPNCE_CORE_DATETIME_H
#define RPNCE_CORE_DATETIME_H

/*
 * DATE objects, following RPN83P's design: seven record types on the
 * proleptic Gregorian calendar (years 1 to 9999), with fixed UTC offsets.
 *
 *   Date           D{year,month,day}
 *   Time           T{hour,minute,second}
 *   DateTime       DT{year,month,day,hour,minute,second}
 *   TimeZone       TZ{hour,minute}                  (multiples of 15 minutes)
 *   ZonedDateTime  DZ{year,month,day,hour,minute,second,tzhour,tzminute}
 *   DayOfWeek      DW{dow}                          (ISO: Monday 1 ... Sunday 7)
 *   Duration       DR{days,hours,minutes,seconds}   (all the same sign)
 *
 * This module is plain integer C with no calculator routines, so it can be
 * tested on a PC. Times are counted in seconds from an epoch date (Unix,
 * 1970-01-01, unless another is chosen), using 64-bit integers.
 */

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    OBJ_NONE = 0,       /* a plain number */
    OBJ_DATE, OBJ_TIME, OBJ_DATETIME, OBJ_TZ, OBJ_DZ, OBJ_DW, OBJ_DR,
    OBJ_COUNT
} obj_type_t;

/* One record. Which fields are used depends on the type; a Duration keeps
 * its days in `year`, a DayOfWeek its ISO number in `day`. */
typedef struct {
    uint8_t type;
    int16_t year;
    int8_t month, day, hour, minute, second;
    int8_t tz_hour, tz_minute;
} dt_obj_t;

typedef int64_t dt_int;

typedef enum {
    DT_OK = 0,
    DT_ERR_SYNTAX,      /* can't be read as a record */
    DT_ERR_INVALID,     /* a field out of range (Feb 30, 23:59:60, ...) */
    DT_ERR_TYPE,        /* the operation doesn't apply to these types */
    DT_ERR_DOMAIN,      /* a number that won't do (not whole, too big) */
} dt_err_t;

/* ---------- calendar ---------- */

bool dt_is_leap(int year);
int dt_days_in_month(int year, int month);
dt_int dt_days_from_civil(int year, int month, int day);   /* 1970-01-01 is 0 */
void dt_civil_from_days(dt_int days, int *year, int *month, int *day);
int dt_iso_dow(dt_int days);                                /* Monday 1 ... Sunday 7 */

/* ---------- objects ---------- */

dt_err_t dt_validate(const dt_obj_t *o);

/* Reads typed text: an optional type tag (D, T, DT, TZ, DZ, DR, DW), then
 * {fields separated by commas}, the closing brace optional. Without a tag
 * the type comes from the number of fields: 2 TZ, 3 D, 4 DR, 6 DT, 8 DZ.
 * Also reads compact durations such as 1D2H, 3M or 45S. */
dt_err_t dt_parse(const char *text, dt_obj_t *out);

/* Whether typed text is a record or a compact duration (rather than a
 * number). */
bool dt_text_is_object(const char *text);

/* Formats an object: raw ("D{2024,3,14}") or as a string ("2024-03-14",
 * "2024-03-14 15:36:01-07:00", "UTC-07:00", "Thu", "1d 2h 3m 4s"). */
void dt_format(char *out, const dt_obj_t *o, bool as_string);
#define DT_FORMAT_MAX 32

/* ---------- conversions ---------- */

/* Seconds since the epoch (epoch_days days after 1970-01-01). Date and
 * DateTime are taken as UTC; a ZonedDateTime uses its own offset. */
dt_err_t dt_to_epoch_seconds(const dt_obj_t *o, dt_int epoch_days, dt_int *out);

/* Builds a Date, DateTime or ZonedDateTime (at UTC offset tz_quarters x 15
 * minutes) from epoch seconds. */
dt_err_t dt_from_epoch_seconds(dt_int secs, dt_int epoch_days, obj_type_t type,
                               int tz_quarters, dt_obj_t *out);

/* Offsets: TimeZone <-> quarter hours (TZ{5,30} is 22). */
int dt_tz_quarters(const dt_obj_t *tz);
dt_err_t dt_tz_from_quarters(dt_int quarters, dt_obj_t *out);

/* Time <-> seconds after midnight; Duration <-> seconds. */
dt_int dt_time_seconds(const dt_obj_t *t);
dt_err_t dt_time_from_seconds(dt_int secs, dt_obj_t *out);
dt_int dt_duration_seconds(const dt_obj_t *d);
dt_err_t dt_duration_from_seconds(dt_int secs, dt_obj_t *out);

/* CVTZ: a Date, DateTime (both UTC) or ZonedDateTime as a ZonedDateTime at
 * another offset. */
dt_err_t dt_convert_tz(const dt_obj_t *o, int tz_quarters, dt_obj_t *out);

/* ---------- arithmetic ---------- */

/* An operand: an object, or a number (a whole number in `whole`, or for
 * TimeZone arithmetic hours as quarter hours in `quarters`). */
typedef struct {
    bool is_obj;
    dt_obj_t obj;
    bool is_whole;          /* the number is a whole number */
    dt_int whole;
    bool is_quarter;        /* the number is a multiple of 0.25 */
    dt_int quarters;        /* the number times 4 */
} dt_val_t;

/* The result: an object, or a number equal to num / den (den 1 or 4). */
typedef struct {
    bool is_obj;
    dt_obj_t obj;
    dt_int num;
    int den;
} dt_result_t;

/* y + x and y - x, following RPN83P's tables: dates count in days, times,
 * DateTimes and ZonedDateTimes in seconds, TimeZones in hours (multiples of
 * 0.25), DayOfWeeks in days; a Duration works wherever a number would. */
dt_err_t dt_add(const dt_val_t *y, const dt_val_t *x, dt_result_t *out);
dt_err_t dt_sub(const dt_val_t *y, const dt_val_t *x, dt_result_t *out);

#endif
