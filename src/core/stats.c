#include <ti/error.h>

#include "stats.h"
#include "mathx.h"

void stats_clear(stats_t *s)
{
    for (int i = 0; i < SUM_COUNT; i++)
        s->sum[i] = mx_zero;
    s->lnx_ok = s->lny_ok = !s->linear_only;
}

static void add_to(real_t *sum, const real_t *v, int sign)
{
    *sum = (sign > 0) ? os_RealAdd(sum, v) : os_RealSub(sum, v);
}

void stats_add(stats_t *s, const real_t *x, const real_t *y, int sign)
{
    real_t x2 = os_RealMul(x, x), y2 = os_RealMul(y, y), xy = os_RealMul(x, y);

    add_to(&s->sum[SUM_N], &mx_one, sign);
    add_to(&s->sum[SUM_X], x, sign);
    add_to(&s->sum[SUM_X2], &x2, sign);
    add_to(&s->sum[SUM_Y], y, sign);
    add_to(&s->sum[SUM_Y2], &y2, sign);
    add_to(&s->sum[SUM_XY], &xy, sign);

    if (s->linear_only)
        return;

    /* Log sums need x > 0 (and y > 0); one bad point rules out the models
     * that use them until the data is cleared. */
    bool x_pos = !mx_is_neg(x) && !mx_is_zero(x);
    bool y_pos = !mx_is_neg(y) && !mx_is_zero(y);
    if (!x_pos)
        s->lnx_ok = false;
    if (!y_pos)
        s->lny_ok = false;

    real_t lx = x_pos ? os_RealLog(x) : mx_zero;
    real_t ly = y_pos ? os_RealLog(y) : mx_zero;
    if (x_pos) {
        real_t lx2 = os_RealMul(&lx, &lx), ylx = os_RealMul(y, &lx);
        add_to(&s->sum[SUM_LX], &lx, sign);
        add_to(&s->sum[SUM_LX2], &lx2, sign);
        add_to(&s->sum[SUM_YLX], &ylx, sign);
    }
    if (y_pos) {
        real_t ly2 = os_RealMul(&ly, &ly), xly = os_RealMul(x, &ly);
        add_to(&s->sum[SUM_LY], &ly, sign);
        add_to(&s->sum[SUM_LY2], &ly2, sign);
        add_to(&s->sum[SUM_XLY], &xly, sign);
    }
    if (x_pos && y_pos) {
        real_t lxly = os_RealMul(&lx, &ly);
        add_to(&s->sum[SUM_LXLY], &lxly, sign);
    }
}

/* ---------- basic statistics ---------- */

static void need_points(const stats_t *s, int count)
{
    real_t k = os_Int24ToReal(count);
    if (os_RealCompare(&s->sum[SUM_N], &k) < 0)
        os_ThrowError(OS_E_STAT);
}

static real_t div_or_stat_error(const real_t *a, const real_t *b)
{
    if (mx_is_zero(b))
        os_ThrowError(OS_E_STAT);
    return os_RealDiv(a, b);
}

void stats_sums(const stats_t *s, real_t *x, real_t *y)
{
    *x = s->sum[SUM_X];
    *y = s->sum[SUM_Y];
}

void stats_mean(const stats_t *s, real_t *x, real_t *y)
{
    need_points(s, 1);
    *x = os_RealDiv(&s->sum[SUM_X], &s->sum[SUM_N]);
    *y = os_RealDiv(&s->sum[SUM_Y], &s->sum[SUM_N]);
}

/* WMN: x weighted by y (Sum xy / Sum y), and y weighted by x. */
void stats_wmean(const stats_t *s, real_t *x, real_t *y)
{
    need_points(s, 1);
    *x = div_or_stat_error(&s->sum[SUM_XY], &s->sum[SUM_Y]);
    *y = div_or_stat_error(&s->sum[SUM_XY], &s->sum[SUM_X]);
}

/* n Sum(ab) - Sum(a) Sum(b), the core of variances and covariances. */
static real_t spread(const real_t *n, const real_t *sab, const real_t *sa, const real_t *sb)
{
    real_t t1 = os_RealMul(n, sab);
    real_t t2 = os_RealMul(sa, sb);
    real_t d = os_RealSub(&t1, &t2);
    return mx_is_neg(&d) ? mx_zero : d;     /* rounding can't make it negative */
}

/* Denominator: n (n - 1) for sample statistics, n^2 for the population. */
static real_t denom(const stats_t *s, bool sample)
{
    need_points(s, sample ? 2 : 1);
    const real_t *n = &s->sum[SUM_N];
    real_t m = sample ? os_RealSub(n, &mx_one) : *n;
    return os_RealMul(n, &m);
}

void stats_sdev(const stats_t *s, bool sample, real_t *x, real_t *y)
{
    real_t d = denom(s, sample);
    const real_t *n = &s->sum[SUM_N];
    real_t vx = spread(n, &s->sum[SUM_X2], &s->sum[SUM_X], &s->sum[SUM_X]);
    real_t vy = spread(n, &s->sum[SUM_Y2], &s->sum[SUM_Y], &s->sum[SUM_Y]);
    vx = os_RealDiv(&vx, &d);
    vy = os_RealDiv(&vy, &d);
    *x = os_RealSqrt(&vx);
    *y = os_RealSqrt(&vy);
}

real_t stats_cov(const stats_t *s, bool sample)
{
    real_t d = denom(s, sample);
    const real_t *n = &s->sum[SUM_N];
    real_t t1 = os_RealMul(n, &s->sum[SUM_XY]);
    real_t t2 = os_RealMul(&s->sum[SUM_X], &s->sum[SUM_Y]);
    real_t c = os_RealSub(&t1, &t2);
    return os_RealDiv(&c, &d);
}

/* ---------- curve fitting ----------
 * Each model is a straight line v = a + b u in transformed variables:
 *   LIN: u = x,    v = y          LOG: u = ln x, v = y
 *   EXP: u = x,    v = ln y       PWR: u = ln x, v = ln y */

typedef struct { real_t su, su2, sv, sv2, suv; } fit_sums_t;

bool stats_model_ok(const stats_t *s, uint8_t model)
{
    bool need_lx = (model == FIT_LOG || model == FIT_PWR);
    bool need_ly = (model == FIT_EXP || model == FIT_PWR);
    if (s->linear_only && model != FIT_LIN)
        return false;
    return (!need_lx || s->lnx_ok) && (!need_ly || s->lny_ok);
}

static fit_sums_t fit_sums(const stats_t *s, uint8_t model)
{
    if (!stats_model_ok(s, model))
        os_ThrowError(OS_E_STAT);
    need_points(s, 2);

    fit_sums_t f;
    const real_t *z = s->sum;
    switch (model) {
        case FIT_LOG:
            f.su = z[SUM_LX]; f.su2 = z[SUM_LX2]; f.sv = z[SUM_Y];
            f.sv2 = z[SUM_Y2]; f.suv = z[SUM_YLX];
            break;
        case FIT_EXP:
            f.su = z[SUM_X]; f.su2 = z[SUM_X2]; f.sv = z[SUM_LY];
            f.sv2 = z[SUM_LY2]; f.suv = z[SUM_XLY];
            break;
        case FIT_PWR:
            f.su = z[SUM_LX]; f.su2 = z[SUM_LX2]; f.sv = z[SUM_LY];
            f.sv2 = z[SUM_LY2]; f.suv = z[SUM_LXLY];
            break;
        default:
            f.su = z[SUM_X]; f.su2 = z[SUM_X2]; f.sv = z[SUM_Y];
            f.sv2 = z[SUM_Y2]; f.suv = z[SUM_XY];
            break;
    }
    return f;
}

/* b = (n Suv - Su Sv) / (n Su2 - Su^2) */
static real_t line_slope(const stats_t *s, const fit_sums_t *f)
{
    const real_t *n = &s->sum[SUM_N];
    real_t t1 = os_RealMul(n, &f->suv);
    real_t t2 = os_RealMul(&f->su, &f->sv);
    real_t num = os_RealSub(&t1, &t2);
    real_t den = spread(n, &f->su2, &f->su, &f->su);
    return div_or_stat_error(&num, &den);   /* all x equal: no line */
}

/* a = (Sv - b Su) / n */
static real_t line_intercept(const stats_t *s, const fit_sums_t *f, const real_t *b)
{
    real_t bsu = os_RealMul(b, &f->su);
    real_t d = os_RealSub(&f->sv, &bsu);
    return os_RealDiv(&d, &s->sum[SUM_N]);
}

/* Correlation of a model. Returns false (instead of raising an error) when
 * u or v doesn't vary, so BEST can skip that model. */
static bool corr_try(const stats_t *s, uint8_t model, real_t *r)
{
    fit_sums_t f = fit_sums(s, model);
    const real_t *n = &s->sum[SUM_N];
    real_t t1 = os_RealMul(n, &f.suv);
    real_t t2 = os_RealMul(&f.su, &f.sv);
    real_t num = os_RealSub(&t1, &t2);
    real_t du = spread(n, &f.su2, &f.su, &f.su);
    real_t dv = spread(n, &f.sv2, &f.sv, &f.sv);
    real_t prod = os_RealMul(&du, &dv);
    if (mx_is_zero(&prod))
        return false;
    real_t root = os_RealSqrt(&prod);
    *r = os_RealDiv(&num, &root);
    return true;
}

static real_t corr_of(const stats_t *s, uint8_t model)
{
    real_t r;
    if (!corr_try(s, model, &r))
        os_ThrowError(OS_E_STAT);
    return r;
}

real_t stats_slope(const stats_t *s)
{
    fit_sums_t f = fit_sums(s, s->model);
    return line_slope(s, &f);
}

/* The model's own intercept: e^a for EXP and PWR. */
real_t stats_intercept(const stats_t *s)
{
    fit_sums_t f = fit_sums(s, s->model);
    real_t b = line_slope(s, &f);
    real_t a = line_intercept(s, &f, &b);
    return (s->model == FIT_EXP || s->model == FIT_PWR) ? os_RealExp(&a) : a;
}

real_t stats_corr(const stats_t *s)
{
    return corr_of(s, s->model);
}

real_t stats_forecast_y(const stats_t *s, const real_t *x)
{
    fit_sums_t f = fit_sums(s, s->model);
    real_t b = line_slope(s, &f);
    real_t a = line_intercept(s, &f, &b);
    real_t u = (s->model == FIT_LOG || s->model == FIT_PWR) ? os_RealLog(x) : *x;
    real_t bu = os_RealMul(&b, &u);
    real_t v = os_RealAdd(&a, &bu);
    return (s->model == FIT_EXP || s->model == FIT_PWR) ? os_RealExp(&v) : v;
}

real_t stats_forecast_x(const stats_t *s, const real_t *y)
{
    fit_sums_t f = fit_sums(s, s->model);
    real_t b = line_slope(s, &f);
    real_t a = line_intercept(s, &f, &b);
    real_t v = (s->model == FIT_EXP || s->model == FIT_PWR) ? os_RealLog(y) : *y;
    real_t d = os_RealSub(&v, &a);
    real_t u = div_or_stat_error(&d, &b);   /* flat line: no single x */
    return (s->model == FIT_LOG || s->model == FIT_PWR) ? os_RealExp(&u) : u;
}

uint8_t stats_best(stats_t *s)
{
    uint8_t best = FIT_LIN;
    real_t best_r = mx_zero;
    bool found = false;

    need_points(s, 2);
    for (uint8_t m = 0; m < FIT_COUNT; m++) {
        real_t r;
        if (!stats_model_ok(s, m) || !corr_try(s, m, &r))
            continue;
        real_t ar = mx_abs(&r);
        if (!found || os_RealCompare(&ar, &best_r) > 0) {
            best = m;
            best_r = ar;
            found = true;
        }
    }
    if (!found)
        os_ThrowError(OS_E_STAT);
    s->model = best;
    return best;
}
