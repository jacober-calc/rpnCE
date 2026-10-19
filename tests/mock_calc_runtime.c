/* Shared host stand-ins for the core regression and equivalence tests. */
#include <stdio.h>
#include <ti/real.h>
#include <ti/error.h>
#include "calc.h"
#include "format.h"
#include "mock_calc_runtime.h"

real_t mock_fin[7];
jmp_buf mock_jmp;
unsigned mock_format_calls;
real_t os_RealDiv(const real_t *a, const real_t *b)
{ if (b->v == 0) os_ThrowError(OS_E_DIVBY0); return mk(a->v / b->v); }
real_t os_RealLog(const real_t *a)
{ if (a->v <= 0) os_ThrowError(OS_E_DOMAIN); return mk(log(a->v)); }
static long long g(long long a, long long b) { while (b) { long long t = a % b; a = b; b = t; } return a; }
real_t os_RealGcd(const real_t *a, const real_t *b) { return mk((double)g((long long)a->v, (long long)b->v)); }
real_t os_RealLcm(const real_t *a, const real_t *b)
{ long long x = (long long)a->v, y = (long long)b->v; return mk((double)(x / g(x, y) * y)); }
void format_real(char *out, const real_t *v, uint8_t style, int8_t digits, int maxd, int maxe, bool group)
{ (void)style; (void)digits; (void)maxd; (void)maxe; (void)group; mock_format_calls++; snprintf(out, FORMAT_MAX, "%.14g", v->v); }

/* decimal rounding stand-ins (the real ones work on the calculator's digits) */
real_t round_decimals(const real_t *v, int n) { double p = pow(10, n); return mk(round(v->v * p) / p); }
real_t round_significant(const real_t *v, int n)
{
    if (v->v == 0) return *v;
    int e = (int)floor(log10(fabs(v->v)));
    return round_decimals(v, n - 1 - e);
}

/* a stand-in hardware clock */
int mock_clock[6] = { 2026, 10, 18, 9, 30, 0 };
bool sys_clock_read(int *y, int *mo, int *d, int *h, int *mi, int *s)
{ *y = mock_clock[0]; *mo = mock_clock[1]; *d = mock_clock[2]; *h = mock_clock[3]; *mi = mock_clock[4]; *s = mock_clock[5]; return true; }
void sys_clock_write(int y, int mo, int d, int h, int mi, int s)
{ mock_clock[0] = y; mock_clock[1] = mo; mock_clock[2] = d; mock_clock[3] = h; mock_clock[4] = mi; mock_clock[5] = s; }

