/* Optimized-build checks with TI-OS arithmetic and a fixed test clock.
 * The count, pass flags and completion marker are published to VRAM. */
#include <string.h>
#include <stddef.h>
#include <ti/getcsc.h>
#include <ti/ui.h>
#include <sys/lcd.h>
#include <graphx.h>
#include "calc.h"
#include "format.h"
#include "dateobj.h"
#include "res.h"
#include "display.h"
#include "units.h"
#include "../checks.h"
#include "../../../src/sys/brightness.h"

_Static_assert(sizeof(real_t) == 9 && offsetof(calc_t, regs) == 131,
               "update the UI smoke test for the new CE value layout");
_Static_assert(NATIVE_CHECK_COUNT > 0 && NATIVE_CHECK_COUNT <= 126,
               "native results must fit before the completion marker");

/* Keep results outside VRAM while the real display renderer is exercised. */
static uint8_t results[128];
#define RESULTS results
static calc_t c;
static uint8_t checks;
static int clock_fields[6] = {2026, 10, 19, 0, 0, 0};
static unsigned clock_writes;
static void check(bool pass)
{
    if (checks < 126) RESULTS[++checks] = pass ? 1 : 0;
    else checks = 127;                       /* fail a count mismatch safely */
}
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
           !memcmp(&c.stack[0], &expected, sizeof expected);
}
static bool x_object(const char *text)
{
    char actual[DT_FORMAT_MAX];
    if (c.message || !obj_is(&c.stack[0])) return false;
    dt_obj_t o = obj_unpack(&c.stack[0]);
    dt_format(actual, &o, false);
    return !strcmp(actual, text);
}

static uint8_t unit_tag(const char *label)
{
    for (uint8_t i = 0; i < UNIT_COUNT; i++)
        if (!strcmp(unit_label(i), label)) return i + 1;
    return 0;
}

static bool x_close(const char *text)
{
    if (c.message || obj_is(&c.stack[0])) return false;
    real_t target = os_StrToReal(text, NULL), tolerance = os_StrToReal("1E-12", NULL);
    real_t zero = os_Int24ToReal(0);
    if (!memcmp(&c.stack[0], &target, sizeof target)) return true;
    if (os_RealCompare(&target, &zero) == 0)
        return os_RealCompare(&c.stack[0], &zero) == 0;
    int difference = (uint8_t)c.stack[0].exp - (uint8_t)target.exp;
    if (difference < -1 || difference > 1 || os_RealCompare(&c.stack[0], &zero) == 0)
        return false;
    real_t one = os_Int24ToReal(1);
    real_t ratio = os_RealDiv(&c.stack[0], &target);
    real_t delta = os_RealSub(&ratio, &one);
    if (os_RealCompare(&delta, &zero) < 0) delta = os_RealNeg(&delta);
    return os_RealCompare(&delta, &tolerance) <= 0;
}

static bool rejected(calc_cmd_t operation, const char *error);

/* Independent decimal references; error cases also protect X/Y/LASTX. */
static void numeric_boundaries(void)
{
    static const struct {
        const char *name;
        calc_cmd_t operation;
        const char *y, *x, *expected, *error;
        uint8_t argument;
    } cases[] = {
        {"decimal sum", CMD_ADD, "0.1", "0.2", "0.3", NULL, 0},
        {"last stored digit", CMD_SUB, "1.0000000000001", "1", "1E-13", NULL, 0},
        {"maximum value", CMD_ADD, "9.9999999999999E99", "0", "9.9999999999999E99", NULL, 0},
        {"minimum scale", CMD_ADD, "1E-99", "1E-99", "2E-99", NULL, 0},
        {"underflow range", CMD_DIV, "1E-99", "10", NULL, NULL, 0},
        {"addition overflow", CMD_ADD, "9E99", "9E99", NULL, "Err: Overflow", 0},
        {"multiplication overflow", CMD_MUL, "1E99", "10", NULL, "Err: Overflow", 0},
        {"division overflow", CMD_DIV, "1E99", "0.1", NULL, "Err: Overflow", 0},
        {"reciprocal at minimum", CMD_RECIP, "0", "1E-99", "1E99", NULL, 0},
        {"positive tiny expm1", CMD_EXP_MINUS_ONE, "0", "1E-99", "1E-99", NULL, 0},
        {"negative tiny expm1", CMD_EXP_MINUS_ONE, "0", "-1E-99", "-1E-99", NULL, 0},
        {"tiny ln1p", CMD_LN_ONE_PLUS, "0", "1E-99", "1E-99", NULL, 0},
        {"tiny sinh", CMD_SINH, "0", "1E-99", "1E-99", NULL, 0},
        {"tiny asinh", CMD_ASINH, "0", "1E-99", "1E-99", NULL, 0},
        {"tiny tanh", CMD_TANH, "0", "1E-99", "1E-99", NULL, 0},
        {"tiny atanh", CMD_ATANH, "0", "1E-99", "1E-99", NULL, 0},
        {"tiny cosh", CMD_COSH, "0", "1E-99", "1", NULL, 0},
        {"atanh upper domain", CMD_ATANH, "0", "1", NULL, "Err: Domain", 0},
        {"atanh lower domain", CMD_ATANH, "0", "-1", NULL, "Err: Domain", 0},
        {"acosh outside domain", CMD_ACOSH, "0", "0.9", NULL, "Err: Domain", 0},
        {"ln1p outside domain", CMD_LN_ONE_PLUS, "0", "-1", NULL, "Err: Domain", 0},
        {"asin outside domain", CMD_ASIN, "0", "1.01", NULL, "Err: Domain", 0},
        {"log at minimum", CMD_LN, "0", "1E-99", "-227.95592420641", NULL, 0},
        {"division by zero", CMD_RECIP, "0", "0", NULL, "Err: Division By 0", 0},
        {"negative square root", CMD_SQRT, "0", "-1", NULL, "Err: Domain", 0},
        {"large square", CMD_SQUARE, "0", "1E49", "1E98", NULL, 0},
        {"small square", CMD_SQUARE, "0", "1E-49", "1E-98", NULL, 0},
        {"large polar radius", CMD_TO_POL, "3E90", "4E90", "5E90", NULL, 0},
        {"small polar radius", CMD_TO_POL, "3E-90", "4E-90", "5E-90", NULL, 0},
        {"representable large sinh", CMD_SINH, "0", "230.5", "6.3657280768316E99", NULL, 0},
        {"acosh next to one", CMD_ACOSH, "0", "1.0000000000001", "4.4721359549995E-7", NULL, 0},
        {"atanh next to one", CMD_ATANH, "0", "0.99999999999999", "16.464669241238", NULL, 0},
        {"ln1p next to minus one", CMD_LN_ONE_PLUS, "0", "-0.99999999999999", "-32.236191301917", NULL, 0},
        {"large asinh", CMD_ASINH, "0", "9E99", "230.84629596431", NULL, 0},
        {"factorial 69", CMD_FACTORIAL, "0", "69", "1.7112245242814E98", NULL, 0},
        {"factorial 70", CMD_FACTORIAL, "0", "70", NULL, "Err: Domain", 0},
        {"decimal halfway rounding", CMD_ROUND_N, "0", "2.675", "2.68", NULL, 2},
    };
    for (unsigned i = 0; i < sizeof cases / sizeof *cases; i++) {
        calc_init(&c);
        c.stack[1] = os_StrToReal(cases[i].y, NULL);
        c.stack[0] = os_StrToReal(cases[i].x, NULL);
        if (cases[i].error) {
            check(rejected(cases[i].operation, cases[i].error));
        } else {
            cmd(cases[i].operation, cases[i].argument);
            if (cases[i].expected) check(x_close(cases[i].expected));
            else {
                int exponent = (uint8_t)c.stack[0].exp - 128;
                check(c.message || (!obj_is(&c.stack[0]) &&
                      exponent >= -99 && exponent <= 99));
            }
        }
    }
    real_t finance[TVM_VAR_COUNT];
    for (uint8_t i = 0; i < TVM_VAR_COUNT; i++) finance[i] = *tvm_var(i);
    const char *rates[] = {"0", "1E-10", "-1E-10"};
    const char *payments[] = {"-100", "-100.00000000055", "-99.99999999945"};
    for (unsigned i = 0; i < 3; i++) {
        calc_init(&c);
        os_Fin_N = os_Int24ToReal(10); os_Fin_I = os_StrToReal(rates[i], NULL);
        os_Fin_PV = os_Int24ToReal(1000); os_Fin_PMT = os_Fin_FV = os_Int24ToReal(0);
        os_Fin_PY = os_Fin_CY = os_Int24ToReal(1);
        c.last_cmd = CMD_TVM_VAR;
        cmd(CMD_TVM_VAR, TVM_PMT);
        bool solved = c.message && !strcmp(c.message, "TVM Calculated");
        c.message = NULL;
        check(solved && x_close(payments[i]));
    }
    for (uint8_t i = 0; i < TVM_VAR_COUNT; i++) *tvm_var(i) = finance[i];

    calc_init(&c);
    c.stats.linear_only = true;
    const char *offset_data[] = {"100000001", "100000002", "100000003"};
    for (unsigned i = 0; i < 3; i++) {
        c.stack[0] = c.stack[1] = os_StrToReal(offset_data[i], NULL);
        cmd(CMD_STAT_PLUS, 0);
    }
    cmd(CMD_STAT_SDEV, 0);
    check(x_close("1"));

    uint8_t meter = 0, km = 0;
    for (uint8_t i = 0; i < UNIT_COUNT; i++) {
        if (!strcmp(unit_label(i), "m")) meter = i + 1;
        if (!strcmp(unit_label(i), "km")) km = i + 1;
    }
    calc_init(&c);
    c.stack[0] = os_StrToReal("1E99", NULL); c.unit[0] = km;
    real_t before = c.stack[0];
    cmd(CMD_UNIT, meter - 1);
    check(km && meter && c.message && !strcmp(c.message, "Err: Overflow") &&
          !memcmp(&before, &c.stack[0], sizeof before) && c.unit[0] == km);
    calc_init(&c);
    c.stack[0] = os_StrToReal("1E-99", NULL); c.unit[0] = meter;
    cmd(CMD_UNIT, km - 1);
    int exponent = (uint8_t)c.stack[0].exp - 128;
    check(km && meter && (c.message || (exponent >= -99 && exponent <= 99)));
}

static void pair(int y, uint8_t uy, int x, uint8_t ux)
{
    calc_init(&c);
    c.stack[1] = os_Int24ToReal(y); c.unit[1] = uy;
    c.stack[0] = os_Int24ToReal(x); c.unit[0] = ux;
}

static bool rejected(calc_cmd_t operation, const char *error)
{
    real_t x = c.stack[0], y = c.stack[1], lastx = c.lastx;
    uint8_t ux = c.unit[0], uy = c.unit[1], ul = c.unit_lastx;
    bool lift = c.lift;
    cmd(operation, 0);
    return c.message && !strcmp(c.message, error) &&
           !memcmp(&x, &c.stack[0], sizeof x) && !memcmp(&y, &c.stack[1], sizeof y) &&
           !memcmp(&lastx, &c.lastx, sizeof lastx) && c.unit[0] == ux &&
           c.unit[1] == uy && c.unit_lastx == ul && c.lift == lift;
}

static void calculator_regressions(void)
{
    pair(0, 0, 1981, 0); cmd(CMD_PRIME, 0);
    check(x_number("7") && os_RealToInt24(&c.stack[1]) == 1981);
    cmd(CMD_DIV, 0); cmd(CMD_PRIME, 0);
    check(x_number("1") && os_RealToInt24(&c.stack[1]) == 283);
    pair(6, 0, 8, 0); cmd(CMD_LCM, 0); check(x_number("24"));
    pair(21, 0, 6, 0); cmd(CMD_LCM, 0); check(x_number("42"));
    pair(0, 0, 6, 0); cmd(CMD_LCM, 0); check(x_number("0"));
    pair(6, 0, 0, 0); cmd(CMD_LCM, 0); check(x_number("0"));

    bool formats_ok = true;
    const uint8_t radices[] = {2, 8, 16};
    base_state_t base = {0};
    for (uint8_t bits = 1; bits <= BASE_MAX_BITS; bits++)
        for (uint8_t r = 0; r < 3; r++) {
            char text[48];
            base.bits = bits; base.radix = radices[r];
            base_word_t value = base_mask(&base), parsed = 0;
            base_format(text, &base, value);
            for (const char *p = text; *p; p++)
                parsed = parsed * base.radix + (base_word_t)base_digit_value(&base, *p);
            formats_ok = formats_ok && parsed == value && text[0] != '0';
        }
    check(formats_ok);

    uint8_t ft = unit_tag("ft"), in = unit_tag("in");
    uint8_t kwh = unit_tag("kWh"), kcal = unit_tag("kcal"), kw = unit_tag("kW");
    pair(2, kwh, 2000, kcal); cmd(CMD_PERCENT_CHANGE, 0);
    check(kwh && kcal && x_close("16.222222222222") && !c.unit[0] && c.unit[1] == kwh);
    pair(6, ft, 2, in); cmd(CMD_ADD, 0); cmd(CMD_UNIT, in - 1);
    check(ft && in && x_close("74") && c.unit[0] == in);
    pair(6, ft, 2, in); cmd(CMD_DIV, 0);
    check(x_close("36") && !c.unit[0]);
    pair(2, kw, 2000, kcal);
    check(kw && rejected(CMD_PERCENT_CHANGE, "Err: Unit Mismatch"));
    pair(6, ft, 2, 0); check(rejected(CMD_ADD, "Err: Data Type"));
    pair(0, 0, 5, ft); cmd(CMD_STO, REG_A); cmd(CMD_CLEAR, 0); cmd(CMD_RCL, REG_A);
    check(x_number("5") && c.unit[0] == ft);
    pair(0, 0, 6, ft); cmd(CMD_STO, REG_A);
    number(2); cmd(CMD_UNIT, in - 1); cmd(CMD_STO_ADD, REG_A); cmd(CMD_RCL, REG_A);
    check(x_close("6.1666666666667") && c.unit[0] == ft);
    pair(0, 0, 6, ft); cmd(CMD_STO, REG_A);
    number(2); cmd(CMD_UNIT, in - 1); cmd(CMD_XCH, REG_A);
    check(x_number("6") && c.unit[0] == ft && c.reg_unit[0] == in &&
          os_RealToInt24(&c.regs[0]) == 2);

    uint8_t cel = unit_tag("\x09" "C"), kel = unit_tag("\x09" "K");
    pair(0, 0, 0, cel); cmd(CMD_UNIT, kel - 1);
    check(cel && kel && x_close("273.15") && c.unit[0] == kel);
    pair(10, cel, 20, cel); check(rejected(CMD_ADD, "Err: Data Type"));
    pair(0, 0, 0, cel); cmd(CMD_UNIT, kel - 1); cmd(CMD_ROUND_N, 0);
    check(x_number("273") && c.unit[0] == kel);
    uint8_t mpg = unit_tag("mpg"), lkm = unit_tag("Lkm");
    pair(0, 0, 30, mpg); cmd(CMD_UNIT, lkm - 1);
    check(mpg && lkm && x_close("7.840486111111") && c.unit[0] == lkm);
    pair(2, 0, 30, mpg); check(rejected(CMD_MUL, "Err: Data Type"));
    pair(0, 0, 30, mpg); cmd(CMD_UNIT, lkm - 1); cmd(CMD_ROUND_N, 0);
    check(x_number("8") && c.unit[0] == lkm);

    pair(2, 0, 1, 0); cmd(CMD_STAT_PLUS, 0);
    c.stack[0] = os_Int24ToReal(3); c.stack[1] = os_Int24ToReal(4);
    cmd(CMD_STAT_PLUS, 0); cmd(CMD_STAT_MEAN, 0);
    check(x_number("2") && os_RealToInt24(&c.stack[1]) == 3);

    pair(2, 0, 1, 0); cmd(CMD_STAT_PLUS, 0);
    c.stack[0] = os_Int24ToReal(2); c.stack[1] = os_Int24ToReal(4);
    cmd(CMD_STAT_PLUS, 0);
    for (uint8_t model = 0; model < FIT_COUNT; model++) {
        cmd(CMD_FIT_MODEL, model);
        c.stack[0] = os_Int24ToReal(model == FIT_LOG ? 4 : 3);
        cmd(CMD_FIT_Y, 0);
        check(x_close(model == FIT_EXP ? "8" : "6"));
    }

    real_t finance[TVM_VAR_COUNT];
    for (uint8_t i = 0; i < TVM_VAR_COUNT; i++) finance[i] = *tvm_var(i);
    os_Fin_N = os_Int24ToReal(10); os_Fin_I = os_Int24ToReal(0);
    os_Fin_PV = os_Int24ToReal(1000); os_Fin_PMT = os_Int24ToReal(0);
    os_Fin_FV = os_Int24ToReal(0); os_Fin_PY = os_Fin_CY = os_Int24ToReal(12);
    c.last_cmd = CMD_TVM_VAR; cmd(CMD_TVM_VAR, TVM_PMT);
    check(c.message && !strcmp(c.message, "TVM Calculated") &&
          os_RealToInt24(&c.stack[0]) == -100);
    catalog_t catalog = {0};
    bool vars_ok = catalog_open(&catalog, &c) && catalog.items[catalog.count - 1] == CAT_LASTX;
    for (uint8_t item = CAT_TVM_FIRST; item <= CAT_TVM_LAST; item++) {
        bool found = false;
        for (uint8_t i = 0; i < catalog.count; i++) found |= catalog.items[i] == item;
        vars_ok &= found;
    }
    check(vars_ok);
    for (uint8_t i = 0; i < TVM_VAR_COUNT; i++) *tvm_var(i) = finance[i];

    bool units_ok = true;
    for (uint8_t i = 0; i < UNIT_COUNT; i++) {
        calc_init(&c);
        c.stack[0] = os_StrToReal("123.25", NULL); c.unit[0] = i + 1;
        cmd(CMD_UNIT, unit_base_of(unit_dim(i))); cmd(CMD_UNIT, i);
        units_ok &= x_close("123.25") && c.unit[0] == i + 1;
    }
    check(units_ok);
}
bool sys_clock_read(int *y, int *m, int *d, int *h, int *mi, int *s)
{ *y = clock_fields[0]; *m = clock_fields[1]; *d = clock_fields[2];
  *h = clock_fields[3]; *mi = clock_fields[4]; *s = clock_fields[5]; return true; }
void sys_clock_write(int y, int m, int d, int h, int mi, int s)
{ (void)y; (void)m; (void)d; (void)h; (void)mi; (void)s; clock_writes++; }

static uint32_t screen_hash(int x, int y, int w, int h)
{
    const uint8_t *screen = (const uint8_t *)lcd_UpBase;
    uint32_t hash = UINT32_C(2166136261);
    for (int row = y; row < y + h; row++)
        for (int col = x; col < x + w; col++)
            hash = (hash ^ screen[row * 320 + col]) * UINT32_C(16777619);
    return hash;
}

static void message_bounds(int *width, int *height)
{
    const uint8_t *screen = (const uint8_t *)lcd_UpBase;
    int left = 320, right = -1, top = 48, bottom = -1;
    for (int y = 18; y < 48; y++)
        for (int x = 6; x < 290; x++)
            if (screen[y * 320 + x] == 1) { /* COL_INK; excludes menu arrows */
                if (x < left) left = x;
                if (x > right) right = x;
                if (y < top) top = y;
                if (y > bottom) bottom = y;
            }
    *width = right < left ? 0 : right - left + 1;
    *height = bottom < top ? 0 : bottom - top + 1;
}

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
    cmd(CMD_DECIMAL_COMMA, 0);
    number(1234567); cmd(CMD_DOT, 0); number(89);
    char grouped[64];
    format_entry(grouped, c.entry, c.decimal_comma);
    check(!strcmp(grouped, "1.234.567,89") && !strcmp(c.entry, "1234567.89"));
    cmd(CMD_SHOW, 0);
    check(c.message && !strcmp(c.message, "1.234.567,89"));
    real_t expected = os_StrToReal("1234567.89", NULL);
    check(os_RealCompare(&c.stack[0], &expected) == 0);
    number(2); cmd(CMD_ADD, 0);
    check(x_number("1234569.89"));
    cmd(CMD_DECIMAL_POINT, 0); cmd(CMD_SHOW, 0);
    check(c.message && !strcmp(c.message, "1,234,569.89"));
    cmd(CMD_DECIMAL_COMMA, 0);
    record("9999,12,31,23,59,59,-23,-45"); cmd(CMD_SHOW, 0);
    check(c.message && !strcmp(c.message, "DZ{9999,12,31,23,59,59,-23,-45}"));

    uint8_t saved_lcd = lcd_BacklightLevel, saved_os = os_Brightness;
    lcd_BacklightLevel = 100;
    sys_brightness_adjust(true);
    check(lcd_BacklightLevel == 92 && os_Brightness == 92);
    sys_brightness_adjust(false);
    check(lcd_BacklightLevel == 100 && os_Brightness == 100);
    lcd_BacklightLevel = 0;
    sys_brightness_adjust(true);
    check(lcd_BacklightLevel == 0 && os_Brightness == 0);
    lcd_BacklightLevel = 224;
    sys_brightness_adjust(false);
    check(lcd_BacklightLevel == 224 && os_Brightness == 224);
    lcd_BacklightLevel = saved_lcd;
    os_Brightness = saved_os;

    /* Real GraphX clipped text ignores scaling: host mocks cannot prove
     * that warning glyphs actually grow on a CE. Inspect the visible LCD. */
    calc_init(&c);
    menu_t menu;
    catalog_t catalog = {0};
    menu_home(&menu);
    display_open();
    display_draw(&c, &menu, false, false, NULL, false, &catalog);
    uint32_t status = screen_hash(0, 0, 320, 18);
    uint32_t arrows = screen_hash(302, 25, 12, 16);
    c.message = "Err: Domain";
    display_draw(&c, &menu, false, false, NULL, false, &catalog);
    int width, height;
    message_bounds(&width, &height);
    check(height == 14); /* seven ink rows, rendered twice instead of once */
    check(width > 0 && width <= (int)gfx_GetStringWidth(c.message));
    check(screen_hash(0, 0, 320, 18) == status);
    check(screen_hash(302, 25, 12, 16) == arrows);
    c.message = "TVM Stored";
    display_draw(&c, &menu, false, false, NULL, false, &catalog);
    message_bounds(&width, &height);
    check(height == 14); /* notices use the same size as errors */
    cmd(CMD_CLEAR, 0); cmd(CMD_CLEAR, 0);
    check(c.message && !strcmp(c.message, "Stack cleared"));
    display_draw(&c, &menu, false, false, NULL, false, &catalog);
    message_bounds(&width, &height);
    check(height == 14 && width > 0 && width <= (int)gfx_GetStringWidth(c.message));
    display_close();
    calculator_regressions();
    numeric_boundaries();
    RESULTS[0] = checks;
    if (checks != NATIVE_CHECK_COUNT) RESULTS[0] = 0;
    RESULTS[127] = 0xAA;
    memcpy((void *)0xD40000, results, sizeof results);
    while (!os_GetCSC()) { }
    return 0;
}
