/* PC stand-in for the calculator's real-number routines (doubles), so the
 * TVM and unit code can be tested without a calculator. */
#ifndef MOCK_TI_REAL_H
#define MOCK_TI_REAL_H
#include <math.h>
#include <stdlib.h>
typedef struct { double v; } real_t;
static inline real_t mk(double v) { real_t r = { v }; return r; }
static inline real_t os_Int24ToReal(int v) { return mk(v); }
static inline real_t os_StrToReal(const char *s, char **e) { return mk(strtod(s, e)); }
static inline int os_RealCompare(const real_t *a, const real_t *b) { return (a->v > b->v) - (a->v < b->v); }
static inline real_t os_RealAdd(const real_t *a, const real_t *b) { return mk(a->v + b->v); }
static inline real_t os_RealSub(const real_t *a, const real_t *b) { return mk(a->v - b->v); }
static inline real_t os_RealMul(const real_t *a, const real_t *b) { return mk(a->v * b->v); }
real_t os_RealDiv(const real_t *a, const real_t *b);
static inline real_t os_RealNeg(const real_t *a) { return mk(-a->v); }
real_t os_RealLog(const real_t *a);
static inline real_t os_RealExp(const real_t *a) { return mk(exp(a->v)); }
static inline real_t os_RealSqrt(const real_t *a) { return mk(sqrt(a->v)); }
static inline real_t os_RealInv(const real_t *a) { return mk(1 / a->v); }
static inline real_t os_RealFrac(const real_t *a) { return mk(a->v - trunc(a->v)); }
static inline real_t os_RealInt(const real_t *a) { return mk(trunc(a->v)); }
static inline real_t os_RealFloor(const real_t *a) { return mk(floor(a->v)); }
static inline real_t os_RealRoundInt(const real_t *a) { return mk(round(a->v)); }
static inline int os_RealToInt24(const real_t *a) { return (int)a->v; }
static inline real_t os_RealPow(const real_t *a, const real_t *b) { return mk(pow(a->v, b->v)); }
static inline real_t os_RealMax(const real_t *a, const real_t *b) { return mk(a->v > b->v ? a->v : b->v); }
static inline real_t os_RealMin(const real_t *a, const real_t *b) { return mk(a->v < b->v ? a->v : b->v); }
static inline real_t os_RealSinRad(const real_t *a) { return mk(sin(a->v)); }
static inline real_t os_RealCosRad(const real_t *a) { return mk(cos(a->v)); }
static inline real_t os_RealTanRad(const real_t *a) { return mk(tan(a->v)); }
static inline real_t os_RealAsinRad(const real_t *a) { return mk(asin(a->v)); }
static inline real_t os_RealAcosRad(const real_t *a) { return mk(acos(a->v)); }
static inline real_t os_RealAtanRad(const real_t *a) { return mk(atan(a->v)); }
static inline real_t os_RealDegToRad(const real_t *a) { return mk(a->v * M_PI / 180); }
static inline real_t os_RealRadToDeg(const real_t *a) { return mk(a->v * 180 / M_PI); }
real_t os_RealGcd(const real_t *a, const real_t *b);
real_t os_RealLcm(const real_t *a, const real_t *b);
extern real_t mock_fin[7];
#define os_Fin_N   mock_fin[0]
#define os_Fin_I   mock_fin[1]
#define os_Fin_PV  mock_fin[2]
#define os_Fin_PMT mock_fin[3]
#define os_Fin_FV  mock_fin[4]
#define os_Fin_PY  mock_fin[5]
#define os_Fin_CY  mock_fin[6]
#endif
