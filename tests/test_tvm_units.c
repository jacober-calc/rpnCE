/* Tests for the TVM solver and unit conversions. Runs on a PC with
 * stand-ins for the calculator's number routines:
 *   gcc -Itests/mock -Isrc/core -o test_tvm tests/test_tvm_units.c \
 *       src/core/tvm.c src/core/mathx.c src/core/units.c src/core/units_data.c -lm
 *   ./test_tvm
 */
#include <stdio.h>
#include <string.h>
#include <ti/real.h>
#include <ti/error.h>
#include "tvm.h"
#include "mathx.h"
#include "units.h"

real_t mock_fin[7];
jmp_buf mock_jmp;
real_t round_decimals(const real_t *v, int n)   /* decimal rounding stand-in */
{ double p = pow(10, n); return mk(round(v->v * p) / p); }
real_t os_RealDiv(const real_t *a, const real_t *b)
{
    if (b->v == 0) os_ThrowError(OS_E_DIVBY0);
    return mk(a->v / b->v);
}
real_t os_RealLog(const real_t *a)
{
    if (a->v <= 0) os_ThrowError(OS_E_DOMAIN);
    return mk(log(a->v));
}

static int fails = 0;
static tvm_settings_t S;

static void set(double n, double i, double pv, double pmt, double fv, double py, double cy, int beg)
{
    double v[7] = { n, i, pv, pmt, fv, py, cy };
    for (int k = 0; k < 7; k++) mock_fin[k] = mk(v[k]);
    S.begin = beg;
}

static void check(const char *name, int which, double want, double reltol)
{
    int err = setjmp(mock_jmp);
    if (err) {
        printf("FAIL %-40s error %d\n", name, err);
        fails++;
        return;
    }
    real_t r = tvm_solve((uint8_t)which, &S);
    double rel = want == 0 ? fabs(r.v) : fabs((r.v - want) / want);
    int ok = rel <= reltol;
    if (!ok) fails++;
    printf("%s %-40s %.12g (want %.12g)\n", ok ? "ok  " : "FAIL", name, r.v, want);
}

static void check_error(const char *name, int which)
{
    int err = setjmp(mock_jmp);
    if (err) { printf("ok   %-40s error %d as expected\n", name, err); return; }
    real_t r = tvm_solve((uint8_t)which, &S);
    printf("FAIL %-40s expected an error, got %.12g\n", name, r.v);
    fails++;
}

static void unit(const char *name, double v, int from, int to, double want)
{
    int err = setjmp(mock_jmp);
    if (err) { printf("FAIL %s error %d\n", name, err); fails++; return; }
    real_t x = mk(v);
    real_t r = unit_convert(&x, (uint8_t)from, (uint8_t)to);
    double rel = want == 0 ? fabs(r.v) : fabs((r.v - want) / want);
    int ok = rel <= 1e-12;
    if (!ok) fails++;
    printf("%s %-40s %.12g (want %.12g)\n", ok ? "ok  " : "FAIL", name, r.v, want);
}

/* finds a unit by label (and factor prefix, where labels repeat) */
static int find_unit_f(const char *label, const char *factor)
{
    for (int i = 0; i < UNIT_COUNT; i++)
        if (strcmp(unit_table[i].label, label) == 0 &&
            (!factor || strncmp(unit_table[i].factor, factor, strlen(factor)) == 0))
            return i;
    printf("no unit %s\n", label);
    return 0;
}

static int find_unit(const char *label)
{
    for (int i = 0; i < UNIT_COUNT; i++)
        if (strcmp(unit_table[i].label, label) == 0) return i;
    printf("no unit %s\n", label);
    return 0;
}

int main(void)
{
    mathx_init();
    tvm_settings_reset(&S);

    /* 30-year mortgage: $200,000 at 6.5%, monthly */
    set(360, 6.5, 200000, 0, 0, 12, 12, 0);
    check("mortgage PMT", TVM_PMT, -1264.136046985, 1e-11);
    set(360, 0, 200000, -1264.136046985, 0, 12, 12, 0);
    check("mortgage I%YR", TVM_IYR, 6.5, 1e-10);
    set(0, 6.5, 200000, -1264.136046985, 0, 12, 12, 0);
    check("mortgage N", TVM_N, 360, 1e-10);
    set(360, 6.5, 0, -1264.136046985, 0, 12, 12, 0);
    check("mortgage PV", TVM_PV, 200000, 1e-11);
    /* balance after 5 years */
    set(60, 6.5, 200000, -1264.136046985, 0, 12, 12, 0);
    check("mortgage balance after 60 payments", TVM_FV, -187221.95488319, 1e-10);

    /* $1000 at 5% for 10 years, yearly */
    set(10, 5, -1000, 0, 0, 1, 1, 0);
    check("savings FV", TVM_FV, 1628.894626777, 1e-11);
    set(10, 0, -1000, 0, 1628.894626777, 1, 1, 0);
    check("savings I%YR", TVM_IYR, 5, 1e-10);

    /* annuity due: $100 at the start of each month for a year, 12% */
    set(12, 12, 0, -100, 0, 12, 12, 1);
    check("annuity due FV (BEG)", TVM_FV, 1280.9328043, 1e-9);
    set(12, 0, 0, -100, 1280.9328043, 12, 12, 1);
    check("annuity due I%YR (BEG)", TVM_IYR, 12, 1e-8);

    /* Canadian mortgage: monthly payments, compounded twice a year */
    set(300, 5, 300000, 0, 0, 12, 2, 0);
    check("semiannual compounding PMT", TVM_PMT, -1744.8149551110, 1e-10);

    /* zero interest */
    set(10, 0, 1000, 0, 0, 12, 12, 0);
    check("zero rate PMT", TVM_PMT, -100, 1e-12);
    set(0, 0, 1000, -100, 0, 12, 12, 0);
    check("zero rate N", TVM_N, 10, 1e-12);

    /* car lease with a residual, payments in advance */
    set(36, 0, 30000, -450, -15000, 12, 12, 1);
    check("lease I%YR (BEG, residual)", TVM_IYR, 1.7916799247318, 1e-9);

    /* round trips: compute PMT from a rate, then solve the rate back */
    {
        static const double rates[] = { 0.01, 0.5, 3.25, 7, 18.9, 45, 150 };
        static const double terms[] = { 1, 12, 60, 360, 600 };
        static const double comp[][2] = { { 12, 12 }, { 1, 1 }, { 12, 2 }, { 52, 365 } };
        int total = 0, bad = 0;
        for (unsigned r = 0; r < sizeof rates / sizeof *rates; r++)
        for (unsigned n = 0; n < sizeof terms / sizeof *terms; n++)
        for (unsigned c = 0; c < 4; c++)
        for (int beg = 0; beg < 2; beg++)
        for (int with_fv = 0; with_fv < 2; with_fv++) {
            /* one payment at the very start, nothing at the end: the loan is
             * repaid at once, so every rate fits; nothing to solve */
            if (terms[n] == 1 && beg && !with_fv)
                continue;
            set(terms[n], rates[r], 10000, 0, with_fv ? -2500 : 0,
                comp[c][0], comp[c][1], beg);
            if (setjmp(mock_jmp)) { bad++; total++; continue; }
            real_t p = tvm_solve(TVM_PMT, &S);
            mock_fin[3] = p;
            mock_fin[1] = mk(0);
            if (setjmp(mock_jmp)) {
                printf("     round trip failed: N=%g I=%g PY=%g CY=%g beg=%d fv=%d\n",
                       terms[n], rates[r], comp[c][0], comp[c][1], beg, with_fv);
                bad++; total++; continue;
            }
            real_t i = tvm_solve(TVM_IYR, &S);
            total++;
            if (fabs(i.v - rates[r]) > 1e-8 * rates[r] + 1e-9) {
                printf("     round trip off: N=%g I=%g PY=%g CY=%g beg=%d fv=%d got %.12g\n", terms[n], rates[r], comp[c][0], comp[c][1], beg, with_fv, i.v);
                bad++;
            }
        }
        printf("%s I%%YR round trips: %d of %d solved back exactly\n",
               bad ? "FAIL" : "ok  ", total - bad, total);
        fails += bad != 0;
    }

    /* no solution */
    set(0, 5, 1000, 100, 0, 12, 12, 0);
    check_error("N with payments the wrong sign", TVM_N);
    set(10, 0, 1000, 100, 500, 12, 12, 0);
    check_error("I%YR with all cash flows positive", TVM_IYR);
    set(0, 5, 1000, -100, 0, 12, 12, 0);
    mock_fin[0] = mk(0);
    check_error("PMT with N = 0", TVM_PMT);

    /* units */
    unit("1 mi -> km", 1, find_unit("mi"), find_unit("km"), 1.609344);
    unit("100 F -> C", 100, find_unit("\x09""F"), find_unit("\x09""C"), 37.777777777778);
    unit("-40 C -> F", -40, find_unit("\x09""C"), find_unit("\x09""F"), -40);
    unit("0 C -> K", 0, find_unit("\x09""C"), find_unit("\x09""K"), 273.15);
    unit("30 mpg -> L/100km", 30, find_unit("mpg"), find_unit("Lkm"), 7.840486111111);
    unit("1 US gal -> L", 1, find_unit_f("gal", "3.785"), find_unit("L"), 3.785411784);
    unit("1 imp gal -> US gal", 1, find_unit_f("gal", "4.546"), find_unit_f("gal", "3.785"), 1.2009499255049);
    unit("1 atm -> psi", 1, find_unit("atm"), find_unit("psi"), 14.695948775514);
    unit("1 hp -> kW", 1, find_unit("hp"), find_unit("kW"), 0.74569987158227);
    unit("1 lb -> kg", 1, find_unit("lb"), find_unit("kg"), 0.45359237);
    unit("1 acre -> m^2", 1, find_unit("acre"), find_unit("m\x01"), 4046.8564224);
    unit("1 kWh -> Btu", 1, find_unit("kWh"), find_unit("Btu"), 3412.1416331279);
    unit("60 mph -> kph", 60, find_unit("mph"), find_unit("kph"), 96.56064);
    unit("1 ly -> pc", 1, find_unit("ly"), find_unit("pc"), 0.30660139378555);

    printf("%d failures\n", fails);
    return fails != 0;
}
