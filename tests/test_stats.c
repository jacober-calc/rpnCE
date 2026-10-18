/* Tests for statistics and curve fitting, on a PC:
 *   gcc -Itests/mock -Isrc/core -o test_stats tests/test_stats.c \
 *       src/core/stats.c src/core/mathx.c -lm && ./test_stats
 */
#include <stdio.h>
#include <ti/real.h>
#include <ti/error.h>
#include "stats.h"
#include "mathx.h"

real_t mock_fin[7];
jmp_buf mock_jmp;
real_t round_decimals(const real_t *v, int n)   /* decimal rounding stand-in */
{ double p = pow(10, n); return mk(round(v->v * p) / p); }
real_t os_RealDiv(const real_t *a, const real_t *b)
{ if (b->v == 0) os_ThrowError(OS_E_DIVBY0); return mk(a->v / b->v); }
real_t os_RealLog(const real_t *a)
{ if (a->v <= 0) os_ThrowError(OS_E_DOMAIN); return mk(log(a->v)); }

static int fails = 0;
static void near(const char *name, double got, double want)
{
    double tol = 1e-10 * (fabs(want) > 1 ? fabs(want) : 1);
    int ok = fabs(got - want) <= tol;
    if (!ok) fails++;
    printf("%s %-28s %.12g (want %.12g)\n", ok ? "ok  " : "FAIL", name, got, want);
}

static stats_t S;
static void load(const double *x, const double *y, int n)
{
    stats_clear(&S);
    for (int i = 0; i < n; i++) {
        real_t a = mk(x[i]), b = mk(y[i]);
        stats_add(&S, &a, &b, +1);
    }
}

int main(void)
{
    mathx_init();
    real_t rx, ry, r;
    S.linear_only = false;
    S.model = FIT_LIN;

    /* data: y = 2x + 1 with noise */
    double x[] = { 1, 2, 3, 4, 5 }, y[] = { 3.1, 4.9, 7.2, 8.8, 11.1 };
    if (setjmp(mock_jmp)) { printf("FAIL unexpected error\n"); return 1; }
    load(x, y, 5);
    stats_mean(&S, &rx, &ry);   near("mean x", rx.v, 3);  near("mean y", ry.v, 7.02);
    stats_sdev(&S, true, &rx, &ry);
    near("sample sdev x", rx.v, 1.58113883008);
    near("sample sdev y", ry.v, 3.15071420475);
    stats_sdev(&S, false, &rx, &ry);
    near("population sdev x", rx.v, 1.41421356237);
    r = stats_cov(&S, true);    near("sample covariance", r.v, 4.975);
    r = stats_cov(&S, false);   near("population covariance", r.v, 3.98);
    stats_wmean(&S, &rx, &ry);  near("weighted mean x", rx.v, 3.56695156695);
    r = stats_slope(&S);        near("LIN slope", r.v, 1.99);
    r = stats_intercept(&S);    near("LIN intercept", r.v, 1.05);
    r = stats_corr(&S);         near("LIN correlation", r.v, 0.998651755569);
    real_t six = mk(6), y13 = mk(13);
    r = stats_forecast_y(&S, &six); near("LIN forecast y(6)", r.v, 12.99);
    r = stats_forecast_x(&S, &y13); near("LIN forecast x(13)", r.v, 6.00502512563);

    /* exponential data y = 3 e^(0.5 x) is fit exactly by EXP, and BEST finds it */
    double ex[] = { 0, 1, 2, 3, 4 }, ey[5];
    for (int i = 0; i < 5; i++) ey[i] = 3 * exp(0.5 * ex[i]);
    load(ex, ey, 5);
    S.model = FIT_EXP;
    r = stats_slope(&S);        near("EXP slope", r.v, 0.5);
    r = stats_intercept(&S);    near("EXP intercept", r.v, 3);
    r = stats_corr(&S);         near("EXP correlation", r.v, 1);
    S.model = FIT_LIN;
    printf("%s BEST picks EXP over %s\n",
           stats_best(&S) == FIT_EXP ? "ok  " : "FAIL", "the others");
    if (S.model != FIT_EXP) fails++;
    printf("%s x = 0 rules out LOG and PWR\n",
           !stats_model_ok(&S, FIT_LOG) && !stats_model_ok(&S, FIT_PWR) ? "ok  " : "FAIL");

    /* power data y = 2 x^1.5 */
    double px[] = { 1, 2, 4, 8 }, py[4];
    for (int i = 0; i < 4; i++) py[i] = 2 * pow(px[i], 1.5);
    load(px, py, 4);
    S.model = FIT_PWR;
    r = stats_slope(&S);        near("PWR slope", r.v, 1.5);
    r = stats_intercept(&S);    near("PWR intercept", r.v, 2);
    real_t p9 = mk(9);
    r = stats_forecast_y(&S, &p9); near("PWR forecast y(9)", r.v, 54);

    /* log data y = 1 + 2 ln x */
    double lx[] = { 1, 2, 3, 10 }, ly[4];
    for (int i = 0; i < 4; i++) ly[i] = 1 + 2 * log(lx[i]);
    load(lx, ly, 4);
    S.model = FIT_LOG;
    r = stats_slope(&S);        near("LOG slope", r.v, 2);
    r = stats_intercept(&S);    near("LOG intercept", r.v, 1);
    real_t l5 = mk(1 + 2 * log(5));
    r = stats_forecast_x(&S, &l5); near("LOG forecast x", r.v, 5);

    /* Sigma- undoes Sigma+ */
    load(x, y, 5);
    real_t a = mk(5), b = mk(11.1);
    stats_add(&S, &a, &b, -1);
    near("n after Sigma-", S.sum[SUM_N].v, 4);
    stats_mean(&S, &rx, &ry);   near("mean x after Sigma-", rx.v, 2.5);

    /* too little data */
    stats_clear(&S);
    real_t one = mk(1);
    stats_add(&S, &one, &one, +1);
    int err = setjmp(mock_jmp);
    if (err) {
        printf("ok   one point: sample sdev gives a stat error (%d)\n", err);
    } else {
        stats_sdev(&S, true, &rx, &ry);
        printf("FAIL one point: sample sdev should fail\n");
        fails++;
    }

    printf("%d failures\n", fails);
    return fails != 0;
}
