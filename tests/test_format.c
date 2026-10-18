/* Tests for the number formatter. Runs on a PC, not the calculator:
 *   gcc -o test_format tests/test_format.c src/core/format.c && ./test_format
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/core/format.h"

/* Build digits from a decimal string like "-123.456" (exact, no float). */
static void num(const char *s, bool *neg, int *exp, uint8_t d[14])
{
    memset(d, 0, 14);
    *neg = (*s == '-'); if (*neg) s++;
    char m[64]; int mi = 0, point = -1, e10 = 0;
    for (; *s; s++) {
        if (*s == '.') point = mi;
        else if (*s == 'E') { e10 = atoi(s + 1); break; }
        else m[mi++] = *s;
    }
    if (point < 0) point = mi;
    int first = 0; while (first < mi && m[first] == '0') first++;
    if (first == mi) { *exp = 0; return; }
    *exp = point - first - 1 + e10;
    for (int i = 0; i < 14 && first + i < mi; i++) d[i] = m[first + i] - '0';
}

static int fails = 0;
static int g_cap = 0;   /* exponent-digit cap for the next tests (0 = none) */
static void tg(const char *in, int style, int digits, int max, bool group, const char *want);
static void t(const char *in, int style, int digits, int max, const char *want)
{
    tg(in, style, digits, max, false, want);
}

static void tg(const char *in, int style, int digits, int max, bool group, const char *want)
{
    bool neg; int exp; uint8_t d[14]; char out[FORMAT_MAX];
    num(in, &neg, &exp, d);
    format_digits(out, neg, exp, d, style, digits, max, g_cap ? g_cap : max, group);
    const char *sn = style == DISP_SCI ? "SCI" : style == DISP_ENG ? "ENG" : (digits < 0 ? "ALL" : "FIX");
    int ok = strcmp(out, want) == 0;
    if (!ok) fails++;
    printf("%s %-24s %s %2d -> %-20s %s\n", ok ? "ok  " : "FAIL", in, sn, digits, out, ok ? "" : want);
}

int main(void)
{
    int M = DISPLAY_DIGITS;
    /* FIX */
    t("123456.1234", DISP_FIX, 4, M, "123456.1234");
    t("1234567.1234", DISP_FIX, 4, M, "1234567.1234");
    t("12345678.12345", DISP_FIX, 4, M, "12345678.1235");
    t("123456789012.3", DISP_FIX, 4, M, "123456789012.");
    t("12345678901.26", DISP_FIX, 4, M, "12345678901.3");
    t("1234567890123", DISP_FIX, 4, M, "1.2346E12");
    t("999999999999.6", DISP_FIX, 2, M, "1.00E12");
    t("3.14159265358979", DISP_FIX, 2, M, "3.14");
    t("3.14159265358979", DISP_FIX, 0, M, "3.");
    t("-2.5", DISP_FIX, 0, M, "-3.");
    t("0.00001234", DISP_FIX, 4, M, "1.2340E-5");
    t("0.00005", DISP_FIX, 4, M, "0.0001");
    t("0.00004999", DISP_FIX, 4, M, "4.9990E-5");
    t("0.5", DISP_FIX, 0, M, "1.");
    t("0.4", DISP_FIX, 0, M, "4.E-1");
    t("9.9999", DISP_FIX, 2, M, "10.00");
    t("0", DISP_FIX, 4, M, "0.0000");
    t("100", DISP_FIX, 2, M, "100.00");
    /* SCI */
    t("12345.678", DISP_SCI, 3, M, "1.235E4");
    t("0.000123", DISP_SCI, 2, M, "1.23E-4");
    t("9.9996", DISP_SCI, 3, M, "1.000E1");
    t("-6.02214076E23", DISP_SCI, 4, M, "-6.0221E23");
    t("0", DISP_SCI, 2, M, "0.00E0");
    /* ENG */
    t("12345.678", DISP_ENG, 2, M, "12.3E3");
    t("123456", DISP_ENG, 3, M, "123.5E3");
    t("0.000123", DISP_ENG, 2, M, "123.E-6");
    t("0.0012", DISP_ENG, 2, M, "1.20E-3");
    t("1234", DISP_ENG, 0, M, "1.E3");
    t("99999", DISP_ENG, 1, M, "100.E3");
    t("999999", DISP_ENG, 1, M, "1.0E6");
    /* ALL */
    t("3", -1 + 2, -1, M, "3.");
    t("0.66666666666667", DISP_FIX, -1, M, "0.666666666667");
    t("3.14159265358979", DISP_FIX, -1, M, "3.14159265359");
    t("123456789012", DISP_FIX, -1, M, "123456789012.");
    t("1234567890123", DISP_FIX, -1, M, "1.23456789012E12");
    t("0.00001", DISP_FIX, -1, M, "0.00001");
    t("0.0000000123456789", DISP_FIX, -1, M, "1.23456789E-8");
    t("1E-99", DISP_FIX, -1, M, "1.E-99");
    t("-12.5", DISP_FIX, -1, M, "-12.5");
    t("0", DISP_FIX, -1, M, "0.");
    /* SHOW (14 digits) */
    t("0.66666666666667", DISP_FIX, -1, SHOW_DIGITS, "0.66666666666667");
    t("3.1415926535898", DISP_FIX, -1, SHOW_DIGITS, "3.1415926535898");
    /* more edge cases */
    t("0.123456789012345", DISP_FIX, -1, M, "0.123456789012");
    t("0.0123456789012345", DISP_FIX, -1, M, "1.23456789012E-2");
    t("-0.000001", DISP_FIX, 9, M, "-0.000001000");
    t("0.0000000001", DISP_FIX, 9, M, "1.000000000E-10");
    t("99.995", DISP_FIX, 2, M, "100.00");
    t("12345678901.96", DISP_FIX, 2, M, "12345678902.0");
    t("99999999999.96", DISP_FIX, 2, M, "100000000000.");
    t("1E99", DISP_SCI, 9, M, "1.000000000E99");
    t("-9.87654321E-45", DISP_ENG, 4, M, "-9.8765E-45");
    t("5", DISP_SCI, 0, M, "5.E0");
    /* digit grouping */
    tg("1234567.1234", DISP_FIX, 4, M, true, "1,234,567.1234");
    tg("123456789012", DISP_FIX, -1, M, true, "123,456,789,012.");
    tg("-1000", DISP_FIX, 2, M, true, "-1,000.00");
    tg("999", DISP_FIX, -1, M, true, "999.");
    tg("12345.678", DISP_ENG, 2, M, true, "12.3E3");
    tg("12345.678", DISP_SCI, 3, M, true, "1.235E4");
    tg("0.5", DISP_FIX, 2, M, true, "0.50");
    tg("12345678901234", DISP_FIX, -1, SHOW_DIGITS, true, "12,345,678,901,234.");
    /* with an exponent, the stack shows at most 10 digits */
    g_cap = DISPLAY_EXP_DIGITS;
    t("1234567890123", DISP_FIX, -1, M, "1.23456789E12");
    t("-1.23456789012E-99", DISP_FIX, -1, M, "-1.23456789E-99");
    t("1.5E-20", DISP_FIX, -1, M, "1.5E-20");
    t("12345.678", DISP_SCI, 11, M, "1.234567800E4");
    t("6.02214076E23", DISP_SCI, 3, M, "6.022E23");
    t("123456789.123", DISP_ENG, 11, M, "123.4567891E6");
    t("1234567890123", DISP_FIX, 4, M, "1.2346E12");
    t("99999999999.6", DISP_FIX, -1, M, "99999999999.6");
    t("9.99999999999E20", DISP_FIX, -1, M, "1.E21");
    t("123456789012.5", DISP_FIX, 2, M, "123456789013.");
    g_cap = 0;
    printf("%d failures\n", fails);
    return fails != 0;
}
