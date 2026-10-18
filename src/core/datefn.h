#ifndef RPNCE_CORE_DATEFN_H
#define RPNCE_CORE_DATEFN_H

/* DATE menu functions (the argument of CMD_DATE_FN). */
typedef enum {
    /* epoch days and seconds */
    DF_TO_ED, DF_ED_TO_D, DF_ED_TO_DT, DF_ED_TO_DZ,
    DF_TO_ES, DF_ES_TO_D, DF_ES_TO_DT, DF_ES_TO_DZ, DF_ES_TO_DZ_APP,
    DF_LEAP, DF_DOW, DF_CVTZ,
    /* components */
    DF_YEAR, DF_MONTH, DF_DAY, DF_HOUR, DF_MINUTE, DF_SECOND,
    DF_TZ_HOUR, DF_TZ_MINUTE,
    DF_GET_DATE, DF_GET_TIME, DF_GET_DT, DF_GET_TZ,
    /* casting */
    DF_SHRINK, DF_EXTEND, DF_CUT, DF_LINK,
    /* Time, TimeZone, Duration, DayOfWeek conversions */
    DF_TIME_TO_S, DF_S_TO_TIME, DF_TZ_TO_HR, DF_HR_TO_TZ,
    DF_DAYS_TO_DR, DF_HOURS_TO_DR, DF_MINUTES_TO_DR, DF_SECONDS_TO_DR, DF_DR_TO_S,
    DF_DW_TO_ISO, DF_ISO_TO_DW, DF_DW_TO_UNIX, DF_UNIX_TO_DW,
    /* epochs */
    DF_EPOCH_UNIX, DF_EPOCH_NTP, DF_EPOCH_GPS, DF_EPOCH_TIOS, DF_EPOCH_Y2K,
    DF_EPOCH_CUSTOM, DF_EPOCH_SET_CUSTOM, DF_EPOCH_GET_CUSTOM,
    /* clock */
    DF_NOW, DF_NOW_DATE, DF_NOW_TIME, DF_NOW_DZ, DF_NOW_DZ_UTC,
    DF_SET_APP_TZ, DF_GET_APP_TZ, DF_SET_CLOCK_TZ, DF_GET_CLOCK_TZ, DF_SET_CLOCK,
    /* display */
    DF_FORMAT_RAW, DF_FORMAT_STRING,
} date_fn_t;

/* Epoch choices. */
enum { EPOCH_UNIX, EPOCH_NTP, EPOCH_GPS, EPOCH_TIOS, EPOCH_Y2K, EPOCH_CUSTOM };

/* The hardware clock, provided by the system layer (sys/clock.c on the
 * calculator, a stand-in in tests). Fields are in the clock's own zone. */
bool sys_clock_read(int *year, int *month, int *day, int *hour, int *minute, int *second);
void sys_clock_write(int year, int month, int day, int hour, int minute, int second);

#endif
