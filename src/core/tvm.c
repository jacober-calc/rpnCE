#include <ti/error.h>

#include "tvm.h"
#include "mathx.h"

#define DEFAULT_IYR1      0       /* starting guesses for I%YR, in % */
#define DEFAULT_IYR2      100
#define DEFAULT_ITER_MAX  50

real_t *tvm_var(uint8_t which)
{
    switch (which) {
        case TVM_N:   return &os_Fin_N;
        case TVM_IYR: return &os_Fin_I;
        case TVM_PV:  return &os_Fin_PV;
        case TVM_PMT: return &os_Fin_PMT;
        case TVM_FV:  return &os_Fin_FV;
        case TVM_PYR: return &os_Fin_PY;
        default:      return &os_Fin_CY;
    }
}

void tvm_settings_reset(tvm_settings_t *s)
{
    s->iyr1 = os_Int24ToReal(DEFAULT_IYR1);
    s->iyr2 = os_Int24ToReal(DEFAULT_IYR2);
    s->iter_max = DEFAULT_ITER_MAX;
    s->iyr1_set = s->iyr2_set = s->iter_max_set = false;
}

void tvm_clear(tvm_settings_t *s)
{
    real_t zero = os_Int24ToReal(0), twelve = os_Int24ToReal(12);
    for (uint8_t v = TVM_N; v <= TVM_FV; v++)
        *tvm_var(v) = zero;
    os_Fin_PY = twelve;
    os_Fin_CY = twelve;
    s->begin = false;
}

/* ---------- the equation ---------- */

typedef struct {
    real_t n, pv, pmt, fv;
    real_t py, cy;
    bool begin;
} tvm_t;

static void load(tvm_t *t, const tvm_settings_t *s)
{
    t->n = os_Fin_N;
    t->pv = os_Fin_PV;
    t->pmt = os_Fin_PMT;
    t->fv = os_Fin_FV;
    t->py = os_Fin_PY;
    t->cy = os_Fin_CY;
    t->begin = s->begin;
    if (!mx_is_neg(&t->py) && !mx_is_zero(&t->py) &&
        !mx_is_neg(&t->cy) && !mx_is_zero(&t->cy))
        return;
    os_ThrowError(OS_E_DOMAIN);         /* P/YR and C/YR must be positive */
}

/* Rate per payment period from an annual percentage, and back:
 *   i = (1 + I/(100 CY))^(CY/PY) - 1,   I = 100 CY ((1 + i)^(PY/CY) - 1) */
static real_t periodic_rate(const tvm_t *t, const real_t *iyr)
{
    real_t hcy = os_RealMul(&mx_hundred, &t->cy);
    real_t r = os_RealDiv(iyr, &hcy);
    real_t neg_one = os_RealNeg(&mx_one);
    if (os_RealCompare(&r, &neg_one) <= 0)
        os_ThrowError(OS_E_DOMAIN);
    real_t l = mx_ln1p(&r);
    real_t ratio = os_RealDiv(&t->cy, &t->py);
    real_t e = os_RealMul(&ratio, &l);
    return mx_expm1(&e);
}

static real_t annual_rate(const tvm_t *t, const real_t *i)
{
    real_t l = mx_ln1p(i);
    real_t ratio = os_RealDiv(&t->py, &t->cy);
    real_t e = os_RealMul(&ratio, &l);
    real_t g = mx_expm1(&e);
    real_t hcy = os_RealMul(&mx_hundred, &t->cy);
    return os_RealMul(&hcy, &g);
}

/* For rate i: v = (1+i)^-N, a = (1 - v) / i (= N when i = 0), and
 * b = 1 + i for BEG mode, else 1. Computed with ln1p/expm1 so small rates
 * keep their digits. */
typedef struct { real_t v, a, b; } factors_t;

static factors_t factors(const tvm_t *t, const real_t *i)
{
    factors_t f;
    if (mx_is_zero(i)) {
        f.v = mx_one;
        f.a = t->n;
        f.b = mx_one;
        return f;
    }
    real_t neg_one = os_RealNeg(&mx_one);
    if (os_RealCompare(i, &neg_one) <= 0)
        os_ThrowError(OS_E_DOMAIN);
    real_t l = mx_ln1p(i);
    real_t nl = os_RealMul(&t->n, &l);
    real_t neg_nl = os_RealNeg(&nl);
    f.v = os_RealExp(&neg_nl);
    real_t em = mx_expm1(&neg_nl);          /* v - 1 */
    real_t one_minus_v = os_RealNeg(&em);
    f.a = os_RealDiv(&one_minus_v, i);
    f.b = t->begin ? os_RealAdd(&mx_one, i) : mx_one;
    return f;
}

/* The equation's value at rate i; zero at the solution. */
static real_t residual(const tvm_t *t, const real_t *i)
{
    factors_t f = factors(t, i);
    real_t bp = os_RealMul(&f.b, &t->pmt);
    real_t bpa = os_RealMul(&bp, &f.a);
    real_t fvv = os_RealMul(&t->fv, &f.v);
    real_t s = os_RealAdd(&t->pv, &bpa);
    return os_RealAdd(&s, &fvv);
}

/* ---------- solving ---------- */

static real_t solve_fv(const tvm_t *t, const real_t *i)
{
    factors_t f = factors(t, i);
    real_t bp = os_RealMul(&f.b, &t->pmt);
    real_t bpa = os_RealMul(&bp, &f.a);
    real_t s = os_RealAdd(&t->pv, &bpa);
    real_t q = os_RealDiv(&s, &f.v);
    return os_RealNeg(&q);
}

static real_t solve_pv(const tvm_t *t, const real_t *i)
{
    factors_t f = factors(t, i);
    real_t bp = os_RealMul(&f.b, &t->pmt);
    real_t bpa = os_RealMul(&bp, &f.a);
    real_t fvv = os_RealMul(&t->fv, &f.v);
    real_t s = os_RealAdd(&bpa, &fvv);
    return os_RealNeg(&s);
}

static real_t solve_pmt(const tvm_t *t, const real_t *i)
{
    factors_t f = factors(t, i);
    real_t ba = os_RealMul(&f.b, &f.a);
    if (mx_is_zero(&ba))
        os_ThrowError(OS_E_DIVBY0);         /* N = 0 */
    real_t fvv = os_RealMul(&t->fv, &f.v);
    real_t s = os_RealAdd(&t->pv, &fvv);
    real_t q = os_RealDiv(&s, &ba);
    return os_RealNeg(&q);
}

static real_t solve_n(const tvm_t *t, const real_t *i)
{
    if (mx_is_zero(i)) {
        if (mx_is_zero(&t->pmt))
            os_ThrowError(OS_E_SIGNCHANGE);
        real_t s = os_RealAdd(&t->pv, &t->fv);
        real_t q = os_RealDiv(&s, &t->pmt);
        real_t n = os_RealNeg(&q);
        if (mx_is_neg(&n))
            os_ThrowError(OS_E_SIGNCHANGE);
        return n;
    }
    /* With k = b PMT / i:  v^N = (PV + k) / (k - FV),  N = -ln(that) / ln(1+i) */
    real_t b = t->begin ? os_RealAdd(&mx_one, i) : mx_one;
    real_t bp = os_RealMul(&b, &t->pmt);
    real_t k = os_RealDiv(&bp, i);
    real_t num = os_RealAdd(&t->pv, &k);
    real_t den = os_RealSub(&k, &t->fv);
    if (mx_is_zero(&den))
        os_ThrowError(OS_E_SIGNCHANGE);
    real_t ratio = os_RealDiv(&num, &den);
    if (mx_is_neg(&ratio) || mx_is_zero(&ratio))
        os_ThrowError(OS_E_SIGNCHANGE);     /* no number of payments works */
    real_t ln_r = os_RealLog(&ratio);
    real_t l = mx_ln1p(i);
    real_t q = os_RealDiv(&ln_r, &l);
    real_t n = os_RealNeg(&q);
    if (mx_is_neg(&n))
        os_ThrowError(OS_E_SIGNCHANGE);     /* a negative number of payments */
    return n;
}

/* I%YR: finds the rate where the residual is zero, starting from the two
 * guesses. Uses false position (Illinois variant) when the guesses bracket
 * the answer, which always converges, and the secant method otherwise. */
static real_t solve_iyr(const tvm_t *t, const tvm_settings_t *s)
{
    real_t a = periodic_rate(t, &s->iyr1);
    real_t b = periodic_rate(t, &s->iyr2);
    real_t fa = residual(t, &a);
    real_t fb = residual(t, &b);
    real_t tol = os_StrToReal("1E-12", NULL);
    real_t tiny = os_StrToReal("1E-14", NULL);
    bool bracketed = mx_is_neg(&fa) != mx_is_neg(&fb);
    int side = 0;

    for (int iter = 0; iter < s->iter_max; iter++) {
        if (mx_is_zero(&fb))
            return annual_rate(t, &b);
        if (mx_is_zero(&fa))
            return annual_rate(t, &a);

        /* c = b - fb (b - a) / (fb - fa) */
        real_t dfx = os_RealSub(&fb, &fa);
        if (mx_is_zero(&dfx))
            os_ThrowError(OS_E_ITERATIONS);
        real_t dx = os_RealSub(&b, &a);
        real_t step = os_RealMul(&fb, &dx);
        step = os_RealDiv(&step, &dfx);
        real_t c = os_RealSub(&b, &step);

        /* stay above -100% per period */
        real_t neg_one = os_RealNeg(&mx_one);
        if (os_RealCompare(&c, &neg_one) <= 0) {
            real_t half = os_RealAdd(&b, &neg_one);
            half = os_RealDiv(&half, &mx_two);
            c = half;
        }
        /* Converged when the step is tiny relative to the rate, when the new
         * estimate repeats an old one (the arithmetic can't get closer), or
         * when the bracket around the answer has shrunk to nothing. */
        real_t ac = mx_abs(&c);
        real_t lim = os_RealMul(&ac, &tol);
        lim = os_RealAdd(&lim, &tiny);
        real_t ad = mx_abs(&step);
        if (os_RealCompare(&ad, &lim) <= 0 ||
            os_RealCompare(&c, &a) == 0 || os_RealCompare(&c, &b) == 0)
            return annual_rate(t, &c);
        if (bracketed) {
            real_t width = os_RealSub(&b, &a);
            width = mx_abs(&width);
            if (os_RealCompare(&width, &lim) <= 0)
                return annual_rate(t, &c);
        }

        real_t fc = residual(t, &c);

        if (bracketed) {
            if (mx_is_neg(&fc) == mx_is_neg(&fb)) {
                b = c; fb = fc;
                if (side == -1)
                    fa = os_RealDiv(&fa, &mx_two);   /* Illinois step */
                side = -1;
            } else {
                a = c; fa = fc;
                if (side == 1)
                    fb = os_RealDiv(&fb, &mx_two);
                side = 1;
            }
        } else {
            a = b; fa = fb;
            b = c; fb = fc;
            bracketed = mx_is_neg(&fa) != mx_is_neg(&fb);
        }
    }
    os_ThrowError(OS_E_ITERATIONS);
    return mx_zero;
}

real_t tvm_solve(uint8_t which, const tvm_settings_t *s)
{
    tvm_t t;
    load(&t, s);

    if (which == TVM_IYR)
        return solve_iyr(&t, s);

    real_t i = periodic_rate(&t, &os_Fin_I);
    switch (which) {
        case TVM_N:   return solve_n(&t, &i);
        case TVM_PV:  return solve_pv(&t, &i);
        case TVM_PMT: return solve_pmt(&t, &i);
        default:      return solve_fv(&t, &i);
    }
}
