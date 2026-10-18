/* Tests for hyperbolic functions and H.MMSS conversions, on a PC:
 *   gcc -Itests/mock -Isrc/core -o test_mathx tests/test_mathx.c \
 *       src/core/mathx.c -lm && ./test_mathx
 */
#include <stdio.h>
#include <ti/real.h>
#include <ti/error.h>
#include "mathx.h"
#include "rounding.h"

real_t mock_fin[7];
jmp_buf mock_jmp;
real_t os_RealDiv(const real_t *a, const real_t *b)
{ if (b->v == 0) os_ThrowError(OS_E_DIVBY0); return mk(a->v / b->v); }
real_t os_RealLog(const real_t *a)
{ if (a->v <= 0) os_ThrowError(OS_E_DOMAIN); return mk(log(a->v)); }
real_t round_decimals(const real_t *v, int n)   /* decimal rounding stand-in */
{ double p = pow(10, n); return mk(round(v->v * p) / p); }

static int fails = 0;
static void rel(const char *name, double got, double want, double tol)
{
    double e = want == 0 ? fabs(got) : fabs((got - want) / want);
    int ok = e <= tol;
    if (!ok) fails++;
    printf("%s %-30s %.15g (want %.15g)\n", ok ? "ok  " : "FAIL", name, got, want);
}
static real_t arg_;
#define F(fn, x) (arg_ = mk(x), fn(&arg_).v)

int main(void)
{
    mathx_init();
    if (setjmp(mock_jmp)) { printf("FAIL unexpected error\n"); return 1; }
    double xs[] = { 1e-12, 1e-6, 0.3, -0.3, 1, -2.5, 10, 50 };
    for (unsigned i = 0; i < sizeof xs / sizeof *xs; i++) {
        double x = xs[i];
        char n[40];
        sprintf(n, "sinh(%g)", x);  rel(n, F(mx_sinh, x), sinh(x), 1e-13);
        sprintf(n, "cosh(%g)", x);  rel(n, F(mx_cosh, x), cosh(x), 1e-13);
        sprintf(n, "tanh(%g)", x);  rel(n, F(mx_tanh, x), tanh(x), 1e-13);
        sprintf(n, "asinh(%g)", x); rel(n, F(mx_asinh, x), asinh(x), 1e-13);
        if (fabs(x) < 1) { sprintf(n, "atanh(%g)", x); rel(n, F(mx_atanh, x), atanh(x), 1e-13); }
        if (x >= 1) { sprintf(n, "acosh(%g)", x); rel(n, F(mx_acosh, x), acosh(x), 1e-13); }
    }
    rel("acosh(1 + 1e-10)", F(mx_acosh, 1 + 1e-10), acosh(1 + 1e-10), 1e-6);
    rel("asinh(1e30)", F(mx_asinh, 1e30), asinh(1e30), 1e-13);

    rel("1.3045 HMS -> hours", F(mx_hms_to_hours, 1.3045), 1.5125, 1e-13);
    rel("-2.15 HMS -> hours", F(mx_hms_to_hours, -2.15), -2.25, 1e-13);
    rel("1.5125 hours -> HMS", F(mx_hours_to_hms, 1.5125), 1.3045, 1e-13);
    rel("2.999999999999 h -> HMS", F(mx_hours_to_hms, 2.999999999999), 3.0, 1e-13);
    rel("0.1 hours -> HMS", F(mx_hours_to_hms, 0.1), 0.06, 1e-13);

    int err = setjmp(mock_jmp);
    if (err) printf("ok   atanh(1) is a domain error\n");
    else { (void)F(mx_atanh, 1.0); printf("FAIL atanh(1) should fail\n"); fails++; }

    printf("%d failures\n", fails);
    return fails != 0;
}
