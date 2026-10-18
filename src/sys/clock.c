#include <stdbool.h>
#include <sys/rtc.h>

#include "../core/datefn.h"

/* The CE's real-time clock. Its date and time are in the clock's own zone
 * (DATE > CLK > CTZ); TI-OS shows the same clock in MODE. */
bool sys_clock_read(int *year, int *month, int *day, int *hour, int *minute, int *second)
{
    uint8_t d, mo, h, mi, s;
    uint16_t y;
    boot_GetDate(&d, &mo, &y);
    boot_GetTime(&s, &mi, &h);
    *year = y;
    *month = mo;
    *day = d;
    *hour = h;
    *minute = mi;
    *second = s;
    return y >= 1 && mo >= 1 && mo <= 12 && d >= 1;
}

/* Time first, then date (boot_SetDate after boot_SetTime avoids a known
 * problem with setting them the other way round). */
void sys_clock_write(int year, int month, int day, int hour, int minute, int second)
{
    boot_SetTime((uint8_t)second, (uint8_t)minute, (uint8_t)hour);
    while (rtc_IsBusy())
        ;
    boot_SetDate((uint8_t)day, (uint8_t)month, (uint16_t)year);
}
