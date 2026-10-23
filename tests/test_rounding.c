/* Tests for exact decimal rounding, using the calculator's number layout:
 *   gcc -Itests/bcd -Isrc/core -o test_rounding tests/test_rounding.c \
 *       src/core/rounding.c && ./test_rounding
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ti/real.h>
#include "rounding.h"

/* "-123.456" -> TI real */
static real_t parse(const char *s)
{
    real_t r; memset(&r, 0, sizeof r);
    bool neg = (*s == '-'); if (neg) s++;
    char m[40]; int n = 0, point = -1, e10 = 0;
    for (; *s; s++) {
        if (*s == '.') point = n;
        else if (*s == 'E') { e10 = atoi(s + 1); break; }
        else m[n++] = *s;
    }
    if (point < 0) point = n;
    int f = 0; while (f < n && m[f] == '0') f++;
    r.exp = (int8_t)0x80;
    if (f == n) return r;
    uint8_t d[14] = { 0 };
    for (int i = 0; i < 14 && f + i < n; i++) d[i] = (uint8_t)(m[f + i] - '0');
    for (int i = 0; i < 7; i++) r.mant[i] = (uint8_t)(d[2 * i] << 4 | d[2 * i + 1]);
    r.exp = (int8_t)(uint8_t)(0x80 + point - f - 1 + e10);
    r.sign = neg ? (int8_t)0x80 : 0;
    return r;
}

/* TI real -> "d.dddEexp" (trailing zeros dropped), for comparing */
static void show(const real_t *r, char *out)
{
    uint8_t d[14];
    for (int i = 0; i < 7; i++) { d[2 * i] = r->mant[i] >> 4; d[2 * i + 1] = r->mant[i] & 15; }
    if (d[0] == 0) { strcpy(out, "0"); return; }
    int k = 14; while (k > 1 && d[k - 1] == 0) k--;
    char *p = out;
    if (r->sign & 0x80) *p++ = '-';
    *p++ = (char)('0' + d[0]);
    if (k > 1) { *p++ = '.'; for (int i = 1; i < k; i++) *p++ = (char)('0' + d[i]); }
    sprintf(p, "E%d", (int)(uint8_t)r->exp - 0x80);
}

static int fails = 0;
static void t(const char *what, const char *in, int n, int sig, const char *want)
{
    real_t v = parse(in);
    real_t r = sig ? round_significant(&v, n) : round_decimals(&v, n);
    char got[40]; show(&r, got);
    int ok = strcmp(got, want) == 0;
    if (!ok) fails++;
    printf("%s %s(%s, %d) = %s%s%s\n", ok ? "ok  " : "FAIL", what, in, n, got,
           ok ? "" : "   want ", ok ? "" : want);
}

int main(void)
{
    t("RNDN", "2.675", 2, 0, "2.68E0");             /* exact decimal, not 2.67 */
    t("RNDN", "3.14159265358979", 4, 0, "3.1416E0");
    t("RNDN", "-1.005", 2, 0, "-1.01E0");
    t("RNDN", "9.995", 2, 0, "1E1");
    t("RNDN", "1234.5", 0, 0, "1.235E3");
    t("RNDN", "0.004", 2, 0, "0");
    t("RNDN", "0.005", 2, 0, "1E-2");
    t("RNDN", "0.0049", 2, 0, "0");
    t("RNDN", "123.456", 13, 0, "1.23456E2");
    t("RNDG", "123456.789", 3, 1, "1.23E5");
    t("RNDG", "0.000987654", 2, 1, "9.9E-4");
    t("RNDG", "99.96", 3, 1, "1E2");
    t("RNDG", "-6.02214076E23", 4, 1, "-6.022E23");
    t("RNDG", "1.23456789012345", 14, 1, "1.2345678901234E0");
    t("RNDG", "0", 3, 1, "0");
    t("RNDN", "-0.005", 2, 0, "-1E-2");
    t("RNDN", "1E-99", 11, 0, "0");
    t("RNDG", "1E-99", 1, 1, "1E-99");
    t("RNDG", "9.9999999999999E-99", 13, 1, "1E-98");
    t("RNDG", "9.9999999999999E99", 13, 1, "9.9999999999999E99");
    t("RNDG", "-9.9999999999999E99", 1, 1, "-9.9999999999999E99");
    t("RNDG", "1.2345678901234E99", 13, 1, "1.234567890123E99");
    for (int exp = -99; exp <= 99; exp++) {
        char input[40], expected[40];
        snprintf(input, sizeof input, "-1.2345678901234E%d", exp);
        for (int digits = 1; digits <= 14; digits++) {
            /* Independent decimal half-up rounding of the integer mantissa. */
            uint64_t scale = 1;
            for (int k = digits; k < 14; k++) scale *= 10;
            uint64_t mantissa = ((UINT64_C(12345678901234) + scale / 2) / scale) * scale;
            char whole[16];
            snprintf(whole, sizeof whole, "%llu", (unsigned long long)mantissa);
            int last = 13;
            while (last > 0 && whole[last] == '0') last--;
            if (last == 0) snprintf(expected, sizeof expected, "-%cE%d", whole[0], exp);
            else snprintf(expected, sizeof expected, "-%c.%.*sE%d", whole[0], last, whole + 1, exp);
            t("RNDG", input, digits, 1, expected);
        }
    }
    printf("%d failures\n", fails);
    return fails != 0;
}
