#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "rounding.h"

/* TI number layout: sign byte (bit 7 = negative), exponent byte (0x80 =
 * 10^0), then 14 BCD digits with the first one before the decimal point. */
typedef struct {
    int exp;
    uint8_t d[14];
} digits_t;

static void unpack(const real_t *v, digits_t *u)
{
    for (int i = 0; i < 7; i++) {
        u->d[2 * i] = v->mant[i] >> 4;
        u->d[2 * i + 1] = v->mant[i] & 0x0F;
    }
    u->exp = (int)(uint8_t)v->exp - 0x80;
}

static real_t pack(const real_t *orig, const digits_t *u)
{
    real_t r = *orig;
    for (int i = 0; i < 7; i++)
        r.mant[i] = (uint8_t)((u->d[2 * i] << 4) | u->d[2 * i + 1]);
    r.exp = (int8_t)(uint8_t)(u->exp + 0x80);
    return r;
}

static real_t zero_like(const real_t *orig)
{
    real_t r = *orig;
    r.sign = 0;
    r.exp = (int8_t)0x80;
    memset(r.mant, 0, sizeof r.mant);
    return r;
}

real_t round_significant(const real_t *v, int digits)
{
    digits_t u;
    unpack(v, &u);
    if (u.d[0] == 0 || digits >= 14)
        return *v;                          /* zero, or nothing to round */

    if (digits <= 0) {
        /* everything rounds away, except a leading 5-9 that rounds up to
         * one unit in the next place */
        if (digits < 0 || u.d[0] < 5)
            return zero_like(v);
        memset(u.d, 0, sizeof u.d);
        u.d[0] = 1;
        u.exp += 1;
        if (u.exp > 99)
            return *v;
        return pack(v, &u);
    }

    bool up = u.d[digits] >= 5;
    memset(u.d + digits, 0, 14 - digits);
    if (up) {
        int i = digits - 1;
        while (i >= 0 && u.d[i] == 9)
            u.d[i--] = 0;
        if (i >= 0) {
            u.d[i]++;
        } else {                            /* 9.99 -> 10.0 */
            u.d[0] = 1;
            u.exp += 1;
            if (u.exp > 99)
                return *v;                  /* would overflow: leave it */
        }
    }
    return pack(v, &u);
}

real_t round_decimals(const real_t *v, int decimals)
{
    digits_t u;
    unpack(v, &u);
    if (u.d[0] == 0)
        return *v;
    return round_significant(v, u.exp + 1 + decimals);
}
