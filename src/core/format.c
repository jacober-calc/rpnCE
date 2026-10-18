#include <string.h>
#include "format.h"

/* Set for the duration of one format_digits call. */
static bool g_group;
static int g_exp_max;    /* most significant digits with an exponent */

/* Rounds d[0..13] to n significant digits (half away from zero) into r.
 * Returns 1 if rounding carried into a new leading digit (exp goes up). */
static int round_digits(const uint8_t d[14], int n, uint8_t r[14])
{
    memset(r, 0, 14);
    if (n >= 14) {
        memcpy(r, d, 14);
        return 0;
    }
    if (n <= 0)
        return 0;

    memcpy(r, d, n);
    if (d[n] < 5)
        return 0;

    for (int i = n - 1; i >= 0; i--) {
        if (++r[i] < 10)
            return 0;
        r[i] = 0;
    }
    /* 999.. rounded up to 1000..: shift right one place */
    memmove(r + 1, r, 13);
    r[0] = 1;
    return 1;
}

static bool all_zero(const uint8_t d[14])
{
    for (int i = 0; i < 14; i++)
        if (d[i])
            return false;
    return true;
}

/* Number of significant digits once trailing zeros are dropped (at least 1). */
static int significant(const uint8_t r[14], int max)
{
    int k = max;
    while (k > 1 && r[k - 1] == 0)
        k--;
    return k;
}

static char *put_exponent(char *p, int e)
{
    *p++ = 'E';
    if (e < 0) {
        *p++ = '-';
        e = -e;
    }
    if (e >= 10)
        *p++ = (char)('0' + e / 10);
    *p++ = (char)('0' + e % 10);
    return p;
}

/* Writes the digits r[0..sig-1] as a fixed-point number whose leading digit
 * is at power exp, with exactly `decimals` digits after the point. */
static char *put_fixed(char *p, const uint8_t r[14], int sig, int exp, int decimals)
{
    if (exp < 0) {
        *p++ = '0';
    } else {
        for (int pos = exp; pos >= 0; pos--) {
            int i = exp - pos;
            *p++ = (char)('0' + (i < sig ? r[i] : 0));
            if (g_group && pos > 0 && pos % 3 == 0)
                *p++ = ',';             /* 1,234,567 */
        }
    }
    *p++ = '.';
    for (int pos = -1; pos >= -decimals; pos--) {
        int i = exp - pos;
        *p++ = (char)('0' + (i >= 0 && i < sig ? r[i] : 0));
    }
    return p;
}

/* d.ddd with `sig` significant digits, then the exponent. */
static char *put_sci(char *p, const uint8_t r[14], int sig, int exp)
{
    *p++ = (char)('0' + r[0]);
    *p++ = '.';
    for (int i = 1; i < sig; i++)
        *p++ = (char)('0' + r[i]);
    return put_exponent(p, exp);
}

static char *format_sci(char *p, const uint8_t d[14], int exp, int sig)
{
    uint8_t r[14];
    if (sig > g_exp_max)
        sig = g_exp_max;
    exp += round_digits(d, sig, r);
    return put_sci(p, r, sig, exp);
}

static char *format_eng(char *p, const uint8_t d[14], int exp, int sig)
{
    uint8_t r[14];
    if (sig > g_exp_max)
        sig = g_exp_max;
    exp += round_digits(d, sig, r);

    /* exponent: largest multiple of 3 not above exp */
    int e3 = (exp >= 0) ? (exp / 3) * 3 : -(((-exp) + 2) / 3) * 3;
    int int_digits = exp - e3 + 1;              /* 1 to 3 */
    int decimals = sig - int_digits;
    if (decimals < 0)
        decimals = 0;

    p = put_fixed(p, r, sig, exp - e3, decimals);
    return put_exponent(p, e3);
}

static char *format_all(char *p, const uint8_t d[14], int exp, int max_digits)
{
    uint8_t r[14];
    int e = exp + round_digits(d, max_digits, r);
    int k = significant(r, max_digits);

    /* Fixed form needs e+1 places for a big number, or the zeros after the
     * point plus the digits for a small one (the "0." before the point
     * doesn't count, as on HP calculators). */
    int places = (e >= 0) ? ((e + 1 > k) ? e + 1 : k) : (-e - 1 + k);
    if (places <= max_digits) {
        int decimals = k - e - 1;
        if (decimals < 0)
            decimals = 0;
        return put_fixed(p, r, k, e, decimals);
    }

    /* Otherwise SCI, with as many digits as fit beside the exponent. */
    int sig = (max_digits < g_exp_max) ? max_digits : g_exp_max;
    e = exp + round_digits(d, sig, r);
    return put_sci(p, r, significant(r, sig), e);
}

static char *format_fix(char *p, const uint8_t d[14], int exp, int n, int max_digits)
{
    uint8_t r[14];
    const int sci_sig = (n + 1 > max_digits) ? max_digits : n + 1;

    if (exp >= max_digits)                      /* too big for fixed */
        return format_sci(p, d, exp, sci_sig);

    /* Use fewer decimals if the integer part needs the room. */
    int int_places = (exp >= 0) ? exp + 1 : 1;
    if (int_places + n > max_digits)
        n = max_digits - int_places;

    int sig = exp + 1 + n;                      /* digits kept by rounding */
    if (sig < 0 || (sig == 0 && d[0] < 5))      /* would show as 0.000 */
        return format_sci(p, d, exp, sci_sig);

    int new_exp = exp;
    if (sig == 0) {                             /* rounds up to 10^-n */
        memset(r, 0, 14);
        r[0] = 1;
        new_exp = -n;
        sig = 1;
    } else {
        new_exp += round_digits(d, sig, r);
        if (new_exp != exp) {
            if (new_exp >= max_digits)          /* rounded up out of range */
                return format_sci(p, d, exp, sci_sig);
            sig++;
            int need = new_exp + 1 + n;
            if (need > max_digits && n > 0) {   /* lost room for a decimal */
                n--;
            }
        }
    }
    return put_fixed(p, r, sig, new_exp, n);
}

void format_digits(char *out, bool negative, int exp, const uint8_t d[14],
                   uint8_t style, int8_t digits, int max_digits, int max_exp_digits,
                   bool group)
{
    char *p = out;

    g_group = group;
    g_exp_max = (max_exp_digits < 1) ? 1 : max_exp_digits;

    if (max_digits > 14)
        max_digits = 14;
    if (max_digits < 1)
        max_digits = 1;

    if (all_zero(d)) {
        uint8_t z[14] = { 0 };
        int n = (digits < 0) ? 0 : digits;
        if (n > max_digits - 1)
            n = max_digits - 1;
        if (style == DISP_FIX)
            p = put_fixed(p, z, 1, 0, n);
        else if (style == DISP_SCI || style == DISP_ENG)
            p = put_sci(p, z, n + 1, 0);
        else
            p = put_fixed(p, z, 1, 0, 0);
        *p = '\0';
        return;
    }

    if (negative)
        *p++ = '-';

    int n = (digits < 0) ? -1 : digits;
    if (n > max_digits - 1)
        n = max_digits - 1;

    if (style == DISP_SCI)
        p = format_sci(p, d, exp, n < 0 ? max_digits : n + 1);
    else if (style == DISP_ENG)
        p = format_eng(p, d, exp, n < 0 ? max_digits : n + 1);
    else if (n < 0)
        p = format_all(p, d, exp, max_digits);
    else
        p = format_fix(p, d, exp, n, max_digits);

    *p = '\0';
}

#ifdef __ez80__
void format_real(char *out, const real_t *v, uint8_t style, int8_t digits,
                 int max_digits, int max_exp_digits, bool group)
{
    uint8_t d[14];
    for (int i = 0; i < 7; i++) {
        d[2 * i] = v->mant[i] >> 4;
        d[2 * i + 1] = v->mant[i] & 0x0F;
    }
    int exp = (int)(uint8_t)v->exp - 0x80;
    format_digits(out, (v->sign & 0x80) != 0, exp, d, style, digits,
                  max_digits, max_exp_digits, group);
}
#endif
