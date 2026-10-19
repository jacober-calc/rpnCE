/* Runs the real core and TI-OS reals on the CE, with the release -Oz/LTO
 * settings. Results go to VRAM for CEmu's CRC check: count, pass flags,
 * zero padding and a completion marker. No RPNCE saved data is loaded or
 * written. A simulated clock makes NOW* and time-zone checks reproducible. */
#include <string.h>
#include <ti/getcsc.h>
#include "calc.h"
#include "dateobj.h"
#include "res.h"

#define RESULTS ((volatile uint8_t *)0xD40000)
static calc_t c;
static uint8_t checks;
static int clock_fields[6] = {2026, 10, 19, 0, 0, 0};
static unsigned clock_writes;
static void check(bool pass) { RESULTS[++checks] = pass ? 1 : 0; }
static void cmd(calc_cmd_t k, uint8_t arg) { calc_execute(&c, k, arg); }
static void number(int n)
{
    char s[16];
    int magnitude = n < 0 ? -n : n;
    unsigned len = 0;
    do { s[len++] = (char)('0' + magnitude % 10); magnitude /= 10; } while (magnitude);
    while (len) cmd(CMD_DIGIT, (uint8_t)(s[--len] - '0'));
    if (n < 0) cmd(CMD_CHS, 0);
}
static void record(const char *s)
{
    cmd(CMD_RECORD_OPEN, 0);
    for (; *s; s++) {
        if (*s == ',') cmd(CMD_EE, 0);
        else if (*s == '-') cmd(CMD_CHS, 0);
        else cmd(CMD_DIGIT, (uint8_t)(*s - '0'));
    }
    calc_finish_entry(&c);
}
static bool x_number(const char *text)
{
    real_t expected = os_StrToReal(text, NULL);
    return !c.message && !obj_is(&c.stack[0]) &&
           os_RealCompare(&c.stack[0], &expected) == 0;
}
static bool x_object(const char *text)
{
    char actual[DT_FORMAT_MAX];
    if (c.message || !obj_is(&c.stack[0])) return false;
    dt_obj_t o = obj_unpack(&c.stack[0]);
    dt_format(actual, &o, false);
    return !strcmp(actual, text);
}
bool sys_clock_read(int *y, int *m, int *d, int *h, int *mi, int *s)
{ *y = clock_fields[0]; *m = clock_fields[1]; *d = clock_fields[2];
  *h = clock_fields[3]; *mi = clock_fields[4]; *s = clock_fields[5]; return true; }
void sys_clock_write(int y, int m, int d, int h, int mi, int s)
{ (void)y; (void)m; (void)d; (void)h; (void)mi; (void)s; clock_writes++; }

int main(void)
{
    memset((void *)RESULTS, 0, 128);
    check(res_open());
    calc_init(&c);
    record("1987,01,17"); number(15000); cmd(CMD_ADD, 0);
    check(x_object("D{2028,2,11}"));
    cmd(CMD_CLEAR_STACK, 0);
    record("1987,01,17"); cmd(CMD_DATE_FN, DF_NOW_DATE); cmd(CMD_SUB, 0);
    check(x_number("-14520"));
    cmd(CMD_CLEAR_STACK, 0);
    cmd(CMD_DATE_FN, DF_NOW_DATE); record("1987,01,17"); cmd(CMD_SUB, 0);
    check(x_number("14520"));
    cmd(CMD_CLEAR_STACK, 0);
    record("1987,01,17"); number(1); cmd(CMD_ADD, 0);
    check(x_object("D{1987,1,18}"));
    number(15000); cmd(CMD_ENTER, 0); record("1987,01,17"); cmd(CMD_ADD, 0);
    check(x_object("D{2028,2,11}"));
    record("1987,01,17"); cmd(CMD_DATE_FN, DF_TO_ES);
    check(x_number("537840000"));
    record("9999,12,31"); cmd(CMD_DATE_FN, DF_TO_ES);
    check(x_number("253402214400"));
    record("1,1,1"); cmd(CMD_DATE_FN, DF_TO_ES);
    check(x_number("-62135596800"));
    cmd(CMD_DATE_FN, DF_ES_TO_D);
    check(x_object("D{1,1,1}"));
    record("2024,3,14,15,36,1,-7,0"); cmd(CMD_DATE_FN, DF_TO_ES);
    check(x_number("1710455761"));
    cmd(CMD_DATE_FN, DF_EPOCH_NTP);
    record("2024,3,14,15,36,1,-7,0"); cmd(CMD_DATE_FN, DF_TO_ES);
    check(x_number("3919444561"));
    cmd(CMD_DATE_FN, DF_EPOCH_UNIX);
    record("2024,12,25,0,0,0"); record("2024,3,14,15,36,1"); cmd(CMD_SUB, 0);
    check(x_number("24654239"));
    record("2038,1,19,3,14,7"); number(1); cmd(CMD_ADD, 0);
    check(x_object("DT{2038,1,19,3,14,8}"));
    record("2024,2,28"); number(1); cmd(CMD_ADD, 0);
    check(x_object("D{2024,2,29}"));
    number(1); cmd(CMD_ADD, 0);
    check(x_object("D{2024,3,1}"));
    record("1969,12,31"); number(1); cmd(CMD_ADD, 0);
    check(x_object("D{1970,1,1}"));
    record("2024,3,14"); cmd(CMD_STO, REG_A); number(10); cmd(CMD_STO_ADD, REG_A); cmd(CMD_RCL, REG_A);
    check(x_object("D{2024,3,24}"));

    const int local[6] = {2026, 12, 31, 23, 30, 0};
    memcpy(clock_fields, local, sizeof local);
    number(-7); cmd(CMD_DATE_FN, DF_SET_CLOCK_TZ);
    number(-7); cmd(CMD_DATE_FN, DF_SET_APP_TZ);
    cmd(CMD_DATE_FN, DF_NOW_TIME);
    check(x_object("T{23,30,0}"));
    cmd(CMD_DATE_FN, DF_NOW_DZ);
    check(x_object("DZ{2026,12,31,23,30,0,-7,0}"));
    cmd(CMD_DATE_FN, DF_NOW_DZ_UTC);
    check(x_object("DZ{2027,1,1,6,30,0,0,0}"));
    cmd(CMD_DATE_FN, DF_NOW);
    check(x_number("1798785000"));
    number(9); cmd(CMD_DATE_FN, DF_SET_APP_TZ);
    cmd(CMD_DATE_FN, DF_NOW_TIME);
    check(x_object("T{15,30,0}"));
    cmd(CMD_DATE_FN, DF_NOW_DATE);
    check(x_object("D{2027,1,1}"));
    cmd(CMD_DATE_FN, DF_NOW_DZ);
    check(x_object("DZ{2027,1,1,15,30,0,9,0}"));
    cmd(CMD_DATE_FN, DF_NOW);
    check(x_number("1798785000"));
    dt_obj_t now;
    calc_t before = c;
    check(calc_clock_now(&c, false, &now) && now.year == 2027 &&
          now.month == 1 && now.day == 1 && now.hour == 15 && now.minute == 30 &&
          now.tz_hour == 9 && !memcmp(&before, &c, sizeof c));
    check(!clock_writes && !memcmp(clock_fields, local, sizeof local) && c.clock_tz == -28);
    const int fractional[6] = {2027, 1, 1, 0, 15, 0};
    memcpy(clock_fields, fractional, sizeof fractional);
    record("5,30"); cmd(CMD_DATE_FN, DF_SET_CLOCK_TZ);
    record("-3,-30"); cmd(CMD_DATE_FN, DF_SET_APP_TZ);
    cmd(CMD_DATE_FN, DF_NOW_DZ);
    check(x_object("DZ{2026,12,31,15,15,0,-3,-30}"));
    cmd(CMD_DATE_FN, DF_NOW_DZ_UTC);
    check(x_object("DZ{2026,12,31,18,45,0,0,0}"));
    check(!clock_writes && !memcmp(clock_fields, fractional, sizeof fractional));
    clock_fields[1] = 0;
    check(!calc_clock_now(&c, false, &now));
    cmd(CMD_DATE_FN, DF_NOW_DZ);
    check(c.message && !strcmp(c.message, "Err: Clock"));
    cmd(CMD_RECORD_TAG, OBJ_TIME); record("12,47,0"); cmd(CMD_ENTER, 0);
    cmd(CMD_RECORD_TAG, OBJ_TIME); record("20,17,0"); cmd(CMD_SUB, 0);
    check(x_number("-27000"));
    cmd(CMD_DATE_FN, DF_SECONDS_TO_DR);
    check(x_object("DR{0,-7,-30,0}"));
    cmd(CMD_RECORD_TAG, OBJ_TIME); record("20,17,0"); cmd(CMD_ENTER, 0);
    cmd(CMD_RECORD_TAG, OBJ_TIME); record("12,47,0"); cmd(CMD_SUB, 0);
    check(x_number("27000"));
    cmd(CMD_DATE_FN, DF_SECONDS_TO_DR);
    check(x_object("DR{0,7,30,0}"));
    RESULTS[0] = checks;
    RESULTS[127] = 0xAA;
    while (!os_GetCSC()) { }
    return 0;
}
