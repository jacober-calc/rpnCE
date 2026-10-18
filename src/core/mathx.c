#include <ti/error.h>

#include "mathx.h"
#include "rounding.h"

real_t mx_zero, mx_one, mx_two, mx_ten, mx_hundred;
static real_t mx_tenth;

void mathx_init(void)
{
    mx_zero = os_Int24ToReal(0);
    mx_one = os_Int24ToReal(1);
    mx_two = os_Int24ToReal(2);
    mx_ten = os_Int24ToReal(10);
    mx_hundred = os_Int24ToReal(100);
    mx_tenth = os_RealDiv(&mx_one, &mx_ten);
}

bool mx_is_zero(const real_t *v) { return os_RealCompare(v, &mx_zero) == 0; }
bool mx_is_neg(const real_t *v)  { return os_RealCompare(v, &mx_zero) < 0; }

real_t mx_abs(const real_t *v)
{
    return mx_is_neg(v) ? os_RealNeg(v) : *v;
}

/* For |x| < 1 the series x + x^2/2! + x^3/3! + ... keeps all the digits that
 * subtracting 1 from e^x would lose. */
real_t mx_expm1(const real_t *x)
{
    real_t ax = mx_abs(x);
    if (os_RealCompare(&ax, &mx_one) >= 0) {
        real_t e = os_RealExp(x);
        return os_RealSub(&e, &mx_one);
    }

    real_t sum = *x, term = *x;
    for (int n = 2; n < 40; n++) {
        real_t k = os_Int24ToReal(n);
        term = os_RealMul(&term, x);
        term = os_RealDiv(&term, &k);
        real_t next = os_RealAdd(&sum, &term);
        if (os_RealCompare(&next, &sum) == 0)
            break;                          /* the rest no longer matters */
        sum = next;
    }
    return sum;
}

/* For |x| < 0.1 the series x - x^2/2 + x^3/3 - ... keeps the digits that
 * forming 1 + x would lose. */
real_t mx_ln1p(const real_t *x)
{
    real_t ax = mx_abs(x);
    if (os_RealCompare(&ax, &mx_tenth) >= 0) {
        real_t u = os_RealAdd(&mx_one, x);
        return os_RealLog(&u);
    }

    real_t neg_x = os_RealNeg(x);
    real_t power = *x, sum = *x;
    for (int n = 2; n < 40; n++) {
        real_t k = os_Int24ToReal(n);
        power = os_RealMul(&power, &neg_x);     /* (-1)^(n+1) x^n */
        real_t term = os_RealDiv(&power, &k);
        real_t next = os_RealAdd(&sum, &term);
        if (os_RealCompare(&next, &sum) == 0)
            break;
        sum = next;
    }
    return sum;
}

/* ---------- hyperbolic functions ---------- */

static real_t with_sign(const real_t *v, bool negative)
{
    return negative ? os_RealNeg(v) : *v;
}

/* With u = e^|x| = 1 + m:  sinh = (m + m / (1 + m)) / 2,
 * cosh = (u + 1/u) / 2. Using m = e^|x| - 1 keeps tiny x exact. */
real_t mx_sinh(const real_t *x)
{
    real_t ax = mx_abs(x);
    real_t m = mx_expm1(&ax);
    real_t u = os_RealAdd(&mx_one, &m);
    real_t q = os_RealDiv(&m, &u);
    real_t s = os_RealAdd(&m, &q);
    s = os_RealDiv(&s, &mx_two);
    return with_sign(&s, mx_is_neg(x));
}

real_t mx_cosh(const real_t *x)
{
    real_t ax = mx_abs(x);
    real_t u = os_RealExp(&ax);
    real_t inv = os_RealInv(&u);
    real_t s = os_RealAdd(&u, &inv);
    return os_RealDiv(&s, &mx_two);
}

/* tanh = m / (m + 2) with m = e^(2|x|) - 1; it is 1 to 14 digits past 20. */
real_t mx_tanh(const real_t *x)
{
    real_t ax = mx_abs(x);
    real_t limit = os_Int24ToReal(20);
    real_t t;
    if (os_RealCompare(&ax, &limit) > 0) {
        t = mx_one;
    } else {
        real_t two_x = os_RealMul(&ax, &mx_two);
        real_t m = mx_expm1(&two_x);
        real_t d = os_RealAdd(&m, &mx_two);
        t = os_RealDiv(&m, &d);
    }
    return with_sign(&t, mx_is_neg(x));
}

/* ln(2 |x|), for very large arguments where x^2 would overflow */
static real_t ln_twice(const real_t *ax)
{
    real_t l = os_RealLog(ax);
    real_t l2 = os_RealLog(&mx_two);
    return os_RealAdd(&l, &l2);
}

/* asinh |x| = ln1p(|x| + x^2 / (1 + sqrt(1 + x^2))) */
real_t mx_asinh(const real_t *x)
{
    real_t ax = mx_abs(x);
    real_t big = os_StrToReal("1E10", NULL);
    real_t r;
    if (os_RealCompare(&ax, &big) > 0) {
        r = ln_twice(&ax);
    } else {
        real_t x2 = os_RealMul(&ax, &ax);
        real_t s = os_RealAdd(&mx_one, &x2);
        s = os_RealSqrt(&s);
        s = os_RealAdd(&mx_one, &s);
        real_t q = os_RealDiv(&x2, &s);
        q = os_RealAdd(&ax, &q);
        r = mx_ln1p(&q);
    }
    return with_sign(&r, mx_is_neg(x));
}

/* acosh x = ln1p(t + sqrt(2t + t^2)) with t = x - 1 */
real_t mx_acosh(const real_t *x)
{
    if (os_RealCompare(x, &mx_one) < 0)
        os_ThrowError(OS_E_DOMAIN);
    real_t big = os_StrToReal("1E10", NULL);
    if (os_RealCompare(x, &big) > 0)
        return ln_twice(x);
    real_t t = os_RealSub(x, &mx_one);
    real_t t2 = os_RealMul(&t, &t);
    real_t two_t = os_RealMul(&t, &mx_two);
    real_t s = os_RealAdd(&two_t, &t2);
    s = os_RealSqrt(&s);
    s = os_RealAdd(&t, &s);
    return mx_ln1p(&s);
}

/* atanh |x| = ln1p(2|x| / (1 - |x|)) / 2 */
real_t mx_atanh(const real_t *x)
{
    real_t ax = mx_abs(x);
    if (os_RealCompare(&ax, &mx_one) >= 0)
        os_ThrowError(OS_E_DOMAIN);
    real_t num = os_RealMul(&ax, &mx_two);
    real_t den = os_RealSub(&mx_one, &ax);
    real_t q = os_RealDiv(&num, &den);
    real_t r = mx_ln1p(&q);
    r = os_RealDiv(&r, &mx_two);
    return with_sign(&r, mx_is_neg(x));
}

/* ---------- hours, minutes, seconds ---------- */

/* H.MMSS -> hours: h + mm/60 + ss.s/3600. The digits are decimal, so
 * splitting off minutes and seconds is exact. */
real_t mx_hms_to_hours(const real_t *hms)
{
    real_t a = mx_abs(hms);
    real_t sixty = os_Int24ToReal(60), s3600 = os_Int24ToReal(3600);
    real_t h = os_RealInt(&a);
    real_t f = os_RealSub(&a, &h);
    f = os_RealMul(&f, &mx_hundred);            /* mm.ssss */
    f = round_decimals(&f, 10);                 /* guard against 14.99999... */
    real_t m = os_RealInt(&f);
    real_t sec = os_RealSub(&f, &m);
    sec = os_RealMul(&sec, &mx_hundred);        /* ss.ss */
    real_t mh = os_RealDiv(&m, &sixty);
    real_t sh = os_RealDiv(&sec, &s3600);
    real_t r = os_RealAdd(&h, &mh);
    r = os_RealAdd(&r, &sh);
    return with_sign(&r, mx_is_neg(hms));
}

/* hours -> H.MMSS. Seconds are rounded to 8 decimals (so 59.9999999999 s
 * becomes the next minute rather than an impossible 60 s). */
real_t mx_hours_to_hms(const real_t *hours)
{
    real_t a = mx_abs(hours);
    real_t sixty = os_Int24ToReal(60), s10000 = os_Int24ToReal(10000);
    real_t h = os_RealInt(&a);
    real_t rem = os_RealSub(&a, &h);
    rem = os_RealMul(&rem, &sixty);
    real_t m = os_RealInt(&rem);
    real_t sec = os_RealSub(&rem, &m);
    sec = os_RealMul(&sec, &sixty);
    sec = round_decimals(&sec, 8);
    if (os_RealCompare(&sec, &sixty) >= 0) {
        sec = os_RealSub(&sec, &sixty);
        m = os_RealAdd(&m, &mx_one);
    }
    if (os_RealCompare(&m, &sixty) >= 0) {
        m = os_RealSub(&m, &sixty);
        h = os_RealAdd(&h, &mx_one);
    }
    real_t mm = os_RealDiv(&m, &mx_hundred);
    real_t ss = os_RealDiv(&sec, &s10000);
    real_t r = os_RealAdd(&h, &mm);
    r = os_RealAdd(&r, &ss);
    return with_sign(&r, mx_is_neg(hours));
}
