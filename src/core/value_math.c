#include <string.h>
#include <ti/error.h>
#include <ti/flags.h>
#include "value_math.h"
#include "value_date.h"
#include "value_parse.h"
#include "datefn.h"
#include "format.h"
#include "mathx.h"
#include "base.h"
#include "units.h"
#include "rounding.h"

typedef enum { QUANTITY_OK, QUANTITY_TYPE, QUANTITY_MISMATCH } quantity_err_t;
static bool restricted(uint8_t tag)
{
    if (!tag)
        return false;
    uint8_t dim = unit_dim((uint8_t)(tag - 1));
    return dim == DIM_TEMP || dim == DIM_FUEL;
}

static quantity_err_t quantity_prepare(calc_cmd_t cmd, uint8_t uy,
                                    const real_t *x, uint8_t ux, real_t *converted_x,
                                    uint8_t *result_unit)
{
    *converted_x = *x;
    *result_unit = 0;
    if (uy > UNIT_COUNT || ux > UNIT_COUNT)
        return QUANTITY_TYPE;
    switch (cmd) {
        /* These operate on the displayed value, including TEMP and FUEL. */
        case CMD_CHS: case CMD_INT_PART: case CMD_FRAC_PART: case CMD_FLOOR: case CMD_CEIL:
        case CMD_NEAR: case CMD_ROUND_FIX: case CMD_ROUND_N: case CMD_ROUND_G:
            *result_unit = ux;
            return QUANTITY_OK;
        default:
            break;
    }
    if (restricted(uy) || restricted(ux))
        return QUANTITY_TYPE;
    switch (cmd) {
        case CMD_ABS:
            *result_unit = ux;
            return QUANTITY_OK;
        case CMD_SIGN:
            return QUANTITY_OK;
        case CMD_PERCENT:
            if (ux) return QUANTITY_TYPE;  /* X is a plain percentage */
            *result_unit = uy;
            return QUANTITY_OK;
        case CMD_MUL:
            if (uy && ux) return QUANTITY_TYPE; /* compound units aren't supported */
            *result_unit = uy ? uy : ux;
            return QUANTITY_OK;
        case CMD_DIV:
            if (uy && !ux) {
                *result_unit = uy;
                return QUANTITY_OK;
            }
            if (!uy) return QUANTITY_TYPE; /* no inverse-unit objects */
            break;
        case CMD_ADD: case CMD_SUB: case CMD_MOD: case CMD_MIN: case CMD_MAX:
        case CMD_PERCENT_CHANGE:
            *result_unit = cmd == CMD_PERCENT_CHANGE ? 0 : uy;
            break;
        default:
            return QUANTITY_TYPE;
    }
    if (!uy || !ux)
        return QUANTITY_TYPE;
    if (unit_dim((uint8_t)(uy - 1)) != unit_dim((uint8_t)(ux - 1)))
        return QUANTITY_MISMATCH;
    *converted_x = unit_convert(x, (uint8_t)(ux - 1), (uint8_t)(uy - 1));
    return QUANTITY_OK;
}

/* ---------- constants ---------- */

static real_t r_three, r_360;
static real_t r_ln10, r_ln2, r_pi, r_half_pi, r_quarter_pi;

void value_math_init(void)
{
    mathx_init();
    r_three = os_Int24ToReal(3);
    r_360 = os_Int24ToReal(360);
    r_ln10 = os_RealLog(&mx_ten);
    r_ln2 = os_RealLog(&mx_two);
    r_pi = os_StrToReal("3.1415926535898", NULL);
    r_half_pi = os_RealDiv(&r_pi, &mx_two);
    real_t four = os_Int24ToReal(4);
    r_quarter_pi = os_RealDiv(&r_pi, &four);
}

/* ---------- protected math ---------- */

typedef enum {
    ERR_NONE,
    ERR_DIV0,
    ERR_DOMAIN,
    ERR_OVERFLOW,
    ERR_UNDERFLOW,
    ERR_PRECISION,
    ERR_TVM_NOT_FOUND,
    ERR_STAT,
    ERR_TYPE,           /* an unsupported object or quantity operand */
    ERR_UNIT,
    ERR_OTHER,
} calc_err_t;

static const char *error_text(calc_err_t e)
{
    switch (e) {
        case ERR_DIV0:     return "Err: Division By 0";
        case ERR_DOMAIN:   return "Err: Domain";
        case ERR_OVERFLOW: return "Err: Overflow";
        case ERR_UNDERFLOW: return "Err: Underflow";
        case ERR_PRECISION: return "Err: Precision";
        case ERR_TVM_NOT_FOUND: return "TVM Not Found";
        case ERR_STAT:     return "Err: Stat";
        case ERR_TYPE:     return "Err: Data Type";
        case ERR_UNIT:     return "Err: Unit Mismatch";
        default:           return "Err: Math";
    }
}

static calc_err_t map_os_error(int err)
{
    switch (err & OS_E_MASK) {
        case OS_E_DIVBY0 & OS_E_MASK:   return ERR_DIV0;
        case OS_E_DOMAIN & OS_E_MASK:
        case OS_E_NONREAL & OS_E_MASK:  return ERR_DOMAIN;
        case OS_E_OVERFLOW & OS_E_MASK: return ERR_OVERFLOW;
        case MX_E_UNDERFLOW & OS_E_MASK: return ERR_UNDERFLOW;
        case MX_E_PRECISION & OS_E_MASK: return ERR_PRECISION;
        case OS_E_SIGNCHANGE & OS_E_MASK:
        case OS_E_ITERATIONS & OS_E_MASK: return ERR_TVM_NOT_FOUND;
        case OS_E_STAT & OS_E_MASK:       return ERR_STAT;
        default:                        return ERR_OTHER;
    }
}

/* Result of the last protected call. Static so it survives the error jump. */
static real_t g_result;
static real_t g_result2;        /* second answer (Y) for two-value stats */

static bool is_positive(const real_t *v) { return os_RealCompare(v, &mx_zero) > 0; }

static bool is_integer(const real_t *v)
{
    real_t f = os_RealFrac(v);
    return mx_is_zero(&f);
}

/* Test parity without narrowing a TI real to a native integer. */
static bool is_odd_integer(const real_t *v)
{
    if (!is_integer(v))
        return false;
    real_t half = os_RealDiv(v, &mx_two);
    return !is_integer(&half);
}

/* Checks inputs the OS would choke on (some crash instead of erroring). */
static calc_err_t precheck(calc_cmd_t cmd, const real_t *y, const real_t *x)
{
    real_t neg_one, max_fact;
    switch (cmd) {
        case CMD_DIV:
        case CMD_RECIP:
        case CMD_MOD:
            return mx_is_zero(x) ? ERR_DIV0 : ERR_NONE;
        /* Negative bases require integer powers or odd integer roots.
         * Other exponents can make the OS return a complex result's real part. */
        case CMD_XROOT_Y:
            if (mx_is_zero(x))
                return ERR_DIV0;
            if (mx_is_zero(y) && mx_is_neg(x))
                return ERR_DOMAIN;
            return (mx_is_neg(y) && !is_odd_integer(x)) ? ERR_DOMAIN : ERR_NONE;
        case CMD_POW:
            return (mx_is_neg(y) && !is_integer(x)) ? ERR_DOMAIN : ERR_NONE;
        case CMD_PERCENT_CHANGE:
            return mx_is_zero(y) ? ERR_DIV0 : ERR_NONE;
        case CMD_SQRT:
            return mx_is_neg(x) ? ERR_DOMAIN : ERR_NONE;
        case CMD_LOG:
        case CMD_LN:
        case CMD_LOG2:
            return is_positive(x) ? ERR_NONE : ERR_DOMAIN;
        case CMD_LOG_BASE:
            if (!is_positive(x) || !is_positive(y))
                return ERR_DOMAIN;
            return os_RealCompare(x, &mx_one) == 0 ? ERR_DIV0 : ERR_NONE;
        case CMD_ASIN:
        case CMD_ACOS:
            neg_one = os_RealNeg(&mx_one);
            if (os_RealCompare(x, &mx_one) > 0 || os_RealCompare(x, &neg_one) < 0)
                return ERR_DOMAIN;
            return ERR_NONE;
        case CMD_LN_ONE_PLUS:
            neg_one = os_RealNeg(&mx_one);
            if (os_RealCompare(x, &neg_one) == 0 && !mx_equal(x, &neg_one))
                return ERR_PRECISION;
            return os_RealCompare(x, &neg_one) > 0 ? ERR_NONE : ERR_DOMAIN;
        case CMD_FACTORIAL:
            max_fact = os_Int24ToReal(69);
            if (mx_is_neg(x) || !is_integer(x) || os_RealCompare(x, &max_fact) > 0)
                return ERR_DOMAIN;
            return ERR_NONE;
        case CMD_GCD:
        case CMD_LCM:
            if (!is_integer(x) || !is_integer(y) || mx_is_neg(x) || mx_is_neg(y))
                return ERR_DOMAIN;
            return ERR_NONE;
        case CMD_COMB:
        case CMD_PERM:
            /* Y items taken X at a time: whole numbers with X <= Y */
            if (!is_integer(x) || !is_integer(y) || mx_is_neg(x) || mx_is_neg(y) ||
                os_RealCompare(x, y) > 0)
                return ERR_DOMAIN;
            return ERR_NONE;
        default:
            return ERR_NONE;
    }
}

/* Angle helpers. In DEG mode the angle is first reduced to [0, 360) so the
 * exact quarter turns (0, 90, 180, 270) give exact answers. */
static int quarter_turn(const real_t *deg)
{
    for (int q = 0; q < 4; q++) {
        real_t a = os_Int24ToReal(q * 90);
        if (os_RealCompare(deg, &a) == 0)
            return q;
    }
    return -1;
}

static real_t reduce_degrees(const real_t *x)
{
    real_t q = os_RealDiv(x, &r_360);
    real_t f = os_RealFloor(&q);
    real_t m = os_RealMul(&f, &r_360);
    return os_RealSub(x, &m);
}

static bool g_degrees;

/* Inputs for unit conversions and TVM solving run inside compute(). */
static uint8_t g_unit_to;
static uint8_t g_tvm_which;
static const tvm_settings_t *g_tvm_settings;
static stats_t *g_stats;
static int g_round_digits;          /* RNDN / RNDG */
static uint8_t g_disp_style;        /* RNDF */
static int8_t g_disp_digits;

static real_t trig(calc_cmd_t cmd, const real_t *x)
{
    real_t a = *x;
    if (g_degrees) {
        real_t d = reduce_degrees(x);
        int q = quarter_turn(&d);
        if (q >= 0) {
            static const int8_t sin_q[4] = { 0, 1, 0, -1 };
            static const int8_t cos_q[4] = { 1, 0, -1, 0 };
            if (cmd == CMD_SIN) return os_Int24ToReal(sin_q[q]);
            if (cmd == CMD_COS) return os_Int24ToReal(cos_q[q]);
            if (q == 0 || q == 2) return mx_zero;          /* tan */
            os_ThrowError(OS_E_DOMAIN);                    /* tan(90), tan(270) */
        }
        a = os_RealDegToRad(&d);
    }
    if (cmd == CMD_SIN) return os_RealSinRad(&a);
    if (cmd == CMD_COS) return os_RealCosRad(&a);
    return os_RealTanRad(&a);
}

static real_t inverse_trig(calc_cmd_t cmd, const real_t *x)
{
    real_t r;
    if (cmd == CMD_ASIN)
        r = os_RealAsinRad(x);
    else if (cmd == CMD_ACOS)
        r = os_RealAcosRad(x);
    else
        r = os_RealAtanRad(x);
    return g_degrees ? os_RealRadToDeg(&r) : r;
}

static real_t factorial(const real_t *x)
{
    int n = os_RealToInt24(x);
    real_t r = mx_one;
    for (int i = 2; i <= n; i++) {
        real_t k = os_Int24ToReal(i);
        r = os_RealMul(&r, &k);
    }
    return r;
}

/* PERM: n! / (n-r)! = n (n-1) ... (n-r+1). Overflow is caught by the
 * error handler in compute(), which also bounds the loop. */
static real_t permutations(const real_t *n, const real_t *r)
{
    real_t result = mx_one;
    real_t k = *n;
    real_t count = *r;
    while (os_RealCompare(&count, &mx_zero) > 0) {
        result = os_RealMul(&result, &k);
        k = os_RealSub(&k, &mx_one);
        count = os_RealSub(&count, &mx_one);
    }
    return result;
}

/* COMB: n! / (r! (n-r)!), built up one factor at a time so every step is an
 * exact whole number: c = c * (n - k + i) / i for i = 1..k, k = min(r, n-r). */
static real_t combinations(const real_t *n, const real_t *r)
{
    real_t k = os_RealSub(n, r);
    if (os_RealCompare(r, &k) < 0)
        k = *r;

    real_t base = os_RealSub(n, &k);            /* n - k */
    real_t result = mx_one;
    real_t i = mx_one;
    while (os_RealCompare(&i, &k) <= 0) {
        real_t factor = os_RealAdd(&base, &i);
        result = os_RealMul(&result, &factor);
        result = os_RealDiv(&result, &i);
        i = os_RealAdd(&i, &mx_one);
    }
    return result;
}

/* ATN2: the angle of the point (X, Y), from -180 to 180 degrees (or -pi to
 * pi), correct in every quadrant. Computed from the smaller of |x|/|y| and
 * |y|/|x| so it never overflows, with exact answers on the axes and at 45
 * degrees. ATN2 of (0, 0) is 0. */
static real_t atan2_real(const real_t *y, const real_t *x)
{
    real_t ax = mx_abs(x), ay = mx_abs(y);
    int cmp = os_RealCompare(&ay, &ax);
    real_t a;               /* angle of (|x|, |y|): 0 to 90 degrees */

    if (mx_is_zero(&ay)) {
        a = mx_zero;
    } else if (mx_is_zero(&ax)) {
        a = g_degrees ? os_Int24ToReal(90) : r_half_pi;
    } else if (cmp == 0) {
        a = g_degrees ? os_Int24ToReal(45) : r_quarter_pi;
    } else {
        if (cmp < 0) {
            real_t q = os_RealDiv(&ay, &ax);
            a = os_RealAtanRad(&q);
        } else {
            real_t q = os_RealDiv(&ax, &ay);
            real_t t = os_RealAtanRad(&q);
            a = os_RealSub(&r_half_pi, &t);
        }
        if (g_degrees)
            a = os_RealRadToDeg(&a);
    }

    if (mx_is_neg(x)) {                        /* left half: 180 - a */
        real_t half_turn = g_degrees ? os_Int24ToReal(180) : r_pi;
        a = os_RealSub(&half_turn, &a);
    }
    if (mx_is_neg(y))                          /* below the x axis */
        a = os_RealNeg(&a);
    return a;
}

/* PRIM: the smallest prime factor of a whole number from 2 to 2^32 - 1, or
 * 1 if the number is itself prime (RPN83P's convention, handy for factoring:
 * divide out each factor until PRIM gives 1). Trial division by 2, 3 and
 * then 6k +/- 1; at most 65,536 is ever tried. */
static real_t smallest_prime_factor(const real_t *x)
{
    uint32_t n;
    if (!base_u32(x, &n) || n < 2)
        os_ThrowError(OS_E_DOMAIN);
    if (n % 2 == 0)
        return n == 2 ? mx_one : mx_two;
    if (n % 3 == 0)
        return n == 3 ? mx_one : r_three;
    for (uint32_t k = 5; k <= n / k; k += 6) {
        if (n % k == 0)
            return base_real(k);
        if (n % (k + 2) == 0)
            return base_real(k + 2);
    }
    return mx_one;
}

/* RNDF: round to what the display shows. */
static real_t round_to_display(const real_t *x)
{
    if (g_disp_digits == DISP_FLOAT)
        return *x;
    if (g_disp_style == DISP_FIX)
        return round_decimals(x, g_disp_digits);
    return round_significant(x, g_disp_digits + 1);
}

static real_t cube_root(const real_t *x)
{
    real_t third = os_RealDiv(&mx_one, &r_three);
    if (mx_is_neg(x)) {
        real_t a = os_RealNeg(x);
        real_t r = os_RealPow(&a, &third);
        return os_RealNeg(&r);
    }
    return os_RealPow(x, &third);
}

/* Runs one math operation under the OS error handler. y is ignored by
 * one-argument operations. On success the answer is in g_result. */
/* Which operands an operation reads, so a DATE object never reaches the
 * real-number routines. */
static bool reads_x(calc_cmd_t cmd)
{
    switch (cmd) {
        case CMD_PI: case CMD_E: case CMD_TVM_VAR: case CMD_STAT_SUM: case CMD_STAT_MEAN: case CMD_STAT_WMEAN:
        case CMD_STAT_SDEV: case CMD_STAT_PDEV: case CMD_STAT_SCOV: case CMD_STAT_PCOV:
        case CMD_FIT_SLOPE: case CMD_FIT_INTERCEPT: case CMD_FIT_CORR: case CMD_FIT_BEST:
            return false;
        default:
            return true;
    }
}

static bool reads_y(calc_cmd_t cmd)
{
    return (cmd != CMD_COMPLEX_LINK && value_binary(cmd)) || cmd == CMD_TO_POL || cmd == CMD_TO_REC ||
           cmd == CMD_STAT_PLUS || cmd == CMD_STAT_MINUS ||
           cmd == CMD_PERCENT || cmd == CMD_PERCENT_CHANGE;
}

/* precheck() permits integer powers and odd roots of negative bases.
 * A root's reciprocal exponent can be fractional; its sign stays negative. */
static real_t signed_power(const real_t *y, const real_t *e)
{
    if (!mx_is_neg(y))
        return os_RealPow(y, e);
    real_t mag = os_RealNeg(y);
    real_t r = os_RealPow(&mag, e);
    bool negative = is_integer(e) ? is_odd_integer(e) : true;
    return negative ? os_RealNeg(&r) : r;
}

static calc_err_t compute_real(calc_cmd_t cmd, const real_t *y, const real_t *x,
                          uint8_t uy, uint8_t ux, uint8_t *result_unit)
{
    if (result_unit) *result_unit = 0;

    real_t converted_x;
    uint8_t tag = 0;
    if (!reads_x(cmd)) ux = 0;
    if (!reads_y(cmd)) uy = 0;
    if (reads_x(cmd)) mx_check_range(x);
    if (reads_y(cmd)) mx_check_range(y);
    if ((uy || ux) && cmd != CMD_UNIT) {
        quantity_err_t qe = quantity_prepare(cmd, uy, x, ux, &converted_x, &tag);
        if (qe != QUANTITY_OK) {
            return qe == QUANTITY_MISMATCH ? ERR_UNIT : ERR_TYPE;
        }
        x = &converted_x;
        mx_check_range(x);
    }
    calc_err_t pre = precheck(cmd, y, x);
    if (pre != ERR_NONE) {
        return pre;
    }

    switch (cmd) {
        case CMD_PI: g_result = r_pi; break;
        case CMD_E: g_result = os_RealExp(&mx_one); break;
        case CMD_CHS:    g_result = os_RealNeg(x); tag = ux; break;
        case CMD_ADD:    g_result = mx_add(y, x); break;
        case CMD_SUB:    g_result = mx_sub(y, x); break;
        case CMD_MUL:    g_result = os_RealMul(y, x); break;
        case CMD_DIV:    g_result = os_RealDiv(y, x); break;
        case CMD_POW:    g_result = signed_power(y, x); break;
        case CMD_SQUARE: g_result = os_RealMul(x, x); break;
        case CMD_SQRT:   g_result = os_RealSqrt(x); break;
        case CMD_RECIP:  g_result = os_RealInv(x); break;
        case CMD_LN:     g_result = os_RealLog(x); break;
        case CMD_LOG: {
            real_t ln = os_RealLog(x);
            g_result = os_RealDiv(&ln, &r_ln10);
            break;
        }
        case CMD_EXP:    g_result = os_RealExp(x); break;
        case CMD_ALOG:   g_result = os_RealPow(&mx_ten, x); break;
        case CMD_SIN:
        case CMD_COS:
        case CMD_TAN:    g_result = trig(cmd, x); break;
        case CMD_ASIN:
        case CMD_ACOS:
        case CMD_ATAN:   g_result = inverse_trig(cmd, x); break;

        case CMD_XROOT_Y: {
            real_t inv = os_RealInv(x);
            g_result = signed_power(y, &inv);
            break;
        }
        case CMD_LOG_BASE: {
            real_t ly = os_RealLog(y);
            real_t lx = os_RealLog(x);
            g_result = os_RealDiv(&ly, &lx);
            break;
        }
        case CMD_GCD:    g_result = os_RealGcd(y, x); break;
        case CMD_LCM: {
            /* Avoid the OS LCM entry point; divide first to keep the
             * intermediate product small, and define lcm(0,n) as zero. */
            if (mx_is_zero(y) || mx_is_zero(x)) {
                g_result = mx_zero;
            } else {
                real_t gcd = os_RealGcd(y, x);
                real_t q = os_RealDiv(y, &gcd);
                g_result = os_RealMul(&q, x);
            }
            break;
        }
        case CMD_MOD: {
            /* y - x * floor(y / x): result takes the sign of x */
            real_t q = os_RealDiv(y, x);
            if (uy && ux && uy != ux) {
                /* Conversion can leave a quotient just below an integer.
                 * Refuse an ambiguous floor instead of returning a full divisor. */
                real_t nearest = os_RealRoundInt(&q);
                if (!mx_equal(&q, &nearest)) {
                    real_t distance = mx_sub(&q, &nearest);
                    real_t fraction = os_RealDiv(&distance, &q);
                    fraction = mx_abs(&fraction);
                    real_t limit = os_StrToReal("1E-12", NULL);
                    if (os_RealCompare(&fraction, &limit) <= 0)
                        os_ThrowError(MX_E_PRECISION);
                }
            }
            real_t f = os_RealFloor(&q);
            real_t m = os_RealMul(x, &f);
            g_result = mx_sub(y, &m);
            break;
        }
        case CMD_MIN:    g_result = os_RealMin(y, x); break;
        case CMD_MAX:    g_result = os_RealMax(y, x); break;
        case CMD_COMB:   g_result = combinations(y, x); break;
        case CMD_PERM:   g_result = permutations(y, x); break;
        case CMD_PERCENT: {
            real_t m = os_RealMul(y, x);
            g_result = os_RealDiv(&m, &mx_hundred);
            break;
        }
        case CMD_PERCENT_CHANGE: {
            real_t d = os_RealSub(x, y);
            real_t m = os_RealMul(&d, &mx_hundred);
            g_result = os_RealDiv(&m, y);
            break;
        }
        case CMD_CUBE: {
            real_t sq = os_RealMul(x, x);
            g_result = os_RealMul(&sq, x);
            break;
        }
        case CMD_CUBE_ROOT: g_result = cube_root(x); break;
        case CMD_ATAN2:     g_result = atan2_real(y, x); break;
        case CMD_EXP_MINUS_ONE: g_result = mx_expm1(x); break;
        case CMD_LN_ONE_PLUS:   g_result = mx_ln1p(x); break;
        case CMD_TWO_POW:   g_result = os_RealPow(&mx_two, x); break;
        case CMD_LOG2: {
            real_t ln = os_RealLog(x);
            g_result = os_RealDiv(&ln, &r_ln2);
            break;
        }
        case CMD_INT_PART:  g_result = os_RealInt(x); break;
        case CMD_FRAC_PART: g_result = os_RealFrac(x); break;
        case CMD_FLOOR:     g_result = os_RealFloor(x); break;
        case CMD_CEIL: {
            real_t n = os_RealNeg(x);
            real_t f = os_RealFloor(&n);
            g_result = os_RealNeg(&f);
            break;
        }
        case CMD_NEAR:      g_result = os_RealRoundInt(x); break;
        case CMD_ABS:       g_result = mx_is_neg(x) ? os_RealNeg(x) : *x; break;
        case CMD_SIGN:
            g_result = os_Int24ToReal(mx_is_zero(x) ? 0 : (mx_is_neg(x) ? -1 : 1));
            break;
        case CMD_FACTORIAL: g_result = factorial(x); break;
        case CMD_UNIT:
            if (g_unit_to >= UNIT_COUNT) { return ERR_TYPE; }
            if (ux && unit_dim((uint8_t)(ux - 1)) != unit_dim(g_unit_to)) {
                return ERR_UNIT;
            }
            g_result = ux ? unit_convert(x, (uint8_t)(ux - 1), g_unit_to) : *x;
            tag = (uint8_t)(g_unit_to + 1);
            break;
        case CMD_PRIME:     g_result = smallest_prime_factor(x); break;
        case CMD_ROUND_FIX: g_result = round_to_display(x); break;
        case CMD_ROUND_N:   g_result = round_decimals(x, g_round_digits); break;
        case CMD_ROUND_G:   g_result = round_significant(x, g_round_digits); break;
        case CMD_SINH:      g_result = mx_sinh(x); break;
        case CMD_COSH:      g_result = mx_cosh(x); break;
        case CMD_TANH:      g_result = mx_tanh(x); break;
        case CMD_ASINH:     g_result = mx_asinh(x); break;
        case CMD_ACOSH:     g_result = mx_acosh(x); break;
        case CMD_ATANH:     g_result = mx_atanh(x); break;
        case CMD_TO_HOURS:  g_result = mx_hms_to_hours(x); break;
        case CMD_TO_HMS:    g_result = mx_hours_to_hms(x); break;
        case CMD_HMS_ADD:
        case CMD_HMS_SUB: {
            real_t hy = mx_hms_to_hours(y), hx = mx_hms_to_hours(x);
            real_t h = (cmd == CMD_HMS_ADD) ? os_RealAdd(&hy, &hx) : os_RealSub(&hy, &hx);
            g_result = mx_hours_to_hms(&h);
            break;
        }
        case CMD_TO_POL: {          /* X = x, Y = y  ->  X = r, Y = angle */
            real_t x2 = os_RealMul(x, x), y2 = os_RealMul(y, y);
            if ((mx_is_zero(&x2) && !mx_is_zero(x)) ||
                (mx_is_zero(&y2) && !mx_is_zero(y)))
                os_ThrowError(MX_E_UNDERFLOW);
            mx_check_range(&x2);
            mx_check_range(&y2);
            real_t s = os_RealAdd(&x2, &y2);
            g_result = os_RealSqrt(&s);
            g_result2 = atan2_real(y, x);
            break;
        }
        case CMD_TO_REC: {          /* X = r, Y = angle  ->  X = x, Y = y */
            real_t cs = trig(CMD_COS, y), sn = trig(CMD_SIN, y);
            g_result = os_RealMul(x, &cs);
            g_result2 = os_RealMul(x, &sn);
            break;
        }
        /* statistics: x is X, y is Y */
        case CMD_STAT_PLUS:
        case CMD_STAT_MINUS:
            stats_add(g_stats, x, y, cmd == CMD_STAT_PLUS ? 1 : -1);
            g_result = g_stats->sum[SUM_N];
            break;
        case CMD_STAT_SUM:   stats_sums(g_stats, &g_result, &g_result2); break;
        case CMD_STAT_MEAN:  stats_mean(g_stats, &g_result, &g_result2); break;
        case CMD_STAT_WMEAN: stats_wmean(g_stats, &g_result, &g_result2); break;
        case CMD_STAT_SDEV:  stats_sdev(g_stats, true, &g_result, &g_result2); break;
        case CMD_STAT_PDEV:  stats_sdev(g_stats, false, &g_result, &g_result2); break;
        case CMD_STAT_SCOV:  g_result = stats_cov(g_stats, true); break;
        case CMD_STAT_PCOV:  g_result = stats_cov(g_stats, false); break;
        case CMD_FIT_X:      g_result = stats_forecast_x(g_stats, x); break;
        case CMD_FIT_Y:      g_result = stats_forecast_y(g_stats, x); break;
        case CMD_FIT_SLOPE:  g_result = stats_slope(g_stats); break;
        case CMD_FIT_INTERCEPT: g_result = stats_intercept(g_stats); break;
        case CMD_FIT_CORR:   g_result = stats_corr(g_stats); break;
        case CMD_FIT_BEST:   stats_best(g_stats); break;
        case CMD_TVM_VAR:   g_result = tvm_solve(g_tvm_which, g_tvm_settings); break;
        case CMD_TO_DEG:    g_result = os_RealRadToDeg(x); break;
        case CMD_TO_RAD:    g_result = os_RealDegToRad(x); break;
        default: return ERR_TYPE;
    }

    if ((cmd == CMD_MUL || cmd == CMD_DIV || cmd == CMD_PERCENT) && mx_is_zero(&g_result) &&
        !mx_is_zero(x) && !mx_is_zero(y)) os_ThrowError(MX_E_UNDERFLOW);
    if ((cmd == CMD_SQUARE || cmd == CMD_RECIP || cmd == CMD_CUBE) && mx_is_zero(&g_result) &&
        !mx_is_zero(x)) os_ThrowError(MX_E_UNDERFLOW);
    if ((cmd == CMD_POW || cmd == CMD_XROOT_Y) && mx_is_zero(&g_result) &&
        !mx_is_zero(y)) os_ThrowError(MX_E_UNDERFLOW);
    if ((cmd == CMD_EXP || cmd == CMD_ALOG || cmd == CMD_TWO_POW) &&
        mx_is_zero(&g_result)) os_ThrowError(MX_E_UNDERFLOW);
    if (cmd != CMD_FIT_BEST) mx_check_range(&g_result);
    if (cmd == CMD_STAT_PLUS || cmd == CMD_STAT_MINUS)
        for (uint8_t i = 0; i < SUM_COUNT; i++) mx_check_range(&g_stats->sum[i]);
    if (cmd == CMD_TO_POL || cmd == CMD_TO_REC || cmd == CMD_STAT_SUM ||
        cmd == CMD_STAT_MEAN || cmd == CMD_STAT_WMEAN ||
        cmd == CMD_STAT_SDEV || cmd == CMD_STAT_PDEV)
        mx_check_range(&g_result2);
    if (result_unit) *result_unit = tag;
    return ERR_NONE;
}

bool value_binary(calc_cmd_t cmd)
{
    switch (cmd) {
        case CMD_ADD: case CMD_SUB: case CMD_MUL: case CMD_DIV: case CMD_POW:
        case CMD_XROOT_Y: case CMD_LOG_BASE: case CMD_GCD: case CMD_LCM:
        case CMD_MOD: case CMD_MIN: case CMD_MAX: case CMD_COMB: case CMD_PERM:
        case CMD_ATAN2: case CMD_HMS_ADD: case CMD_HMS_SUB: case CMD_COMPLEX_LINK:
            return true;
        default:
            return false;
    }
}


/* Complex kernels use the same decimal real arithmetic as the rest of the
 * calculator. These helpers never move stack values or mutate registers. */
static real_t magnitude(const real_t *a, const real_t *b)
{
    real_t hi = mx_abs(a), lo = mx_abs(b);
    if (os_RealCompare(&lo, &hi) > 0) { real_t t = hi; hi = lo; lo = t; }
    if (mx_is_zero(&hi)) return mx_zero;
    real_t q = os_RealDiv(&lo, &hi);
    real_t q2 = os_RealMul(&q, &q);
    real_t sum = os_RealAdd(&mx_one, &q2);
    real_t root = os_RealSqrt(&sum);
    return os_RealMul(&hi, &root);
}

static value_t complex_promote(const value_t *v)
{
    return v->type == VALUE_COMPLEX ? *v : value_complex(v->real, mx_zero);
}
static bool complex_zero(const value_t *v)
{
    return mx_is_zero(&v->real) && mx_is_zero(&v->imag);
}
static value_t complex_add(const value_t *a, const value_t *b, bool subtract)
{
    real_t re = subtract ? mx_sub(&a->real, &b->real) : mx_add(&a->real, &b->real);
    real_t im = subtract ? mx_sub(&a->imag, &b->imag) : mx_add(&a->imag, &b->imag);
    return value_complex(re, im);
}
/* Required complex intermediates must be representable. In particular, a
 * silently flushed component must not turn a nonzero answer into zero. */
static real_t checked_product(const real_t *a, const real_t *b)
{
    real_t r = os_RealMul(a, b);
    if (mx_is_zero(&r) && !mx_is_zero(a) && !mx_is_zero(b)) os_ThrowError(MX_E_UNDERFLOW);
    mx_check_range(&r);
    return r;
}
static real_t checked_quotient(const real_t *a, const real_t *b)
{
    real_t r = os_RealDiv(a, b);
    if (mx_is_zero(&r) && !mx_is_zero(a)) os_ThrowError(MX_E_UNDERFLOW);
    mx_check_range(&r);
    return r;
}
static value_t complex_mul(const value_t *a, const value_t *b)
{
    real_t ac = checked_product(&a->real, &b->real), bd = checked_product(&a->imag, &b->imag);
    real_t ad = checked_product(&a->real, &b->imag), bc = checked_product(&a->imag, &b->real);
    return value_complex(mx_sub(&ac, &bd), mx_add(&ad, &bc));
}
static value_t complex_div(const value_t *a, const value_t *b)
{
    if (complex_zero(b)) os_ThrowError(OS_E_DIVBY0);
    real_t ar = mx_abs(&a->real), ai = mx_abs(&a->imag);
    real_t br = mx_abs(&b->real), bi = mx_abs(&b->imag);
    real_t top = os_RealMax(&ar, &ai), bottom = os_RealMax(&br, &bi);
    if (mx_is_zero(&top)) return value_complex(mx_zero, mx_zero);
    /* Normalize both operands: no denominator squares at the original scale. */
    real_t ap = checked_quotient(&a->real, &top), aq = checked_quotient(&a->imag, &top);
    real_t bp = checked_quotient(&b->real, &bottom), bq = checked_quotient(&b->imag, &bottom);
    real_t p2 = checked_product(&bp, &bp), q2 = checked_product(&bq, &bq);
    real_t den = os_RealAdd(&p2, &q2);
    real_t ac = checked_product(&ap, &bp), bd = checked_product(&aq, &bq);
    real_t bc = checked_product(&aq, &bp), ad = checked_product(&ap, &bq);
    real_t re = mx_add(&ac, &bd), im = mx_sub(&bc, &ad);
    re = checked_quotient(&re, &den); im = checked_quotient(&im, &den);
    re = checked_product(&re, &top); im = checked_product(&im, &top);
    re = checked_quotient(&re, &bottom); im = checked_quotient(&im, &bottom);
    return value_complex(re, im);
}
static value_t complex_ln(const value_t *z)
{
    if (complex_zero(z)) os_ThrowError(OS_E_DOMAIN);
    /* log(hypot) without forming a potentially overflowing magnitude. */
    real_t ar = mx_abs(&z->real), ai = mx_abs(&z->imag);
    real_t hi = os_RealMax(&ar, &ai), lo = os_RealMin(&ar, &ai);
    real_t q = os_RealDiv(&lo, &hi), q2 = os_RealMul(&q, &q);
    real_t correction = mx_ln1p(&q2);
    correction = os_RealDiv(&correction, &mx_two);
    real_t loghi = os_RealLog(&hi);
    bool saved = g_degrees; g_degrees = false;
    real_t angle = atan2_real(&z->imag, &z->real);
    g_degrees = saved;
    if (mx_is_zero(&angle) && !mx_is_zero(&z->imag)) os_ThrowError(MX_E_UNDERFLOW);
    return value_complex(mx_add(&loghi, &correction), angle);
}
static value_t complex_exp(const value_t *z)
{
    real_t mag = os_RealExp(&z->real);
    real_t cs = os_RealCosRad(&z->imag), sn = os_RealSinRad(&z->imag);
    if (mx_is_zero(&mag)) os_ThrowError(MX_E_UNDERFLOW);
    return value_complex(checked_product(&mag, &cs), checked_product(&mag, &sn));
}
static value_t complex_pow(const value_t *a, const value_t *b)
{
    if (complex_zero(b)) return value_complex(mx_one, mx_zero);
    if (complex_zero(a)) {
        if (!mx_is_zero(&b->imag) || mx_is_neg(&b->real)) os_ThrowError(OS_E_DOMAIN);
        return value_complex(mx_zero, mx_zero);
    }
    /* Integer powers avoid polar roundoff and preserve exact cancellation. */
    real_t limit = os_Int24ToReal(32767), ab = mx_abs(&b->real);
    if (mx_is_zero(&b->imag) && is_integer(&b->real) && os_RealCompare(&ab, &limit) <= 0) {
        unsigned n = (unsigned)os_RealToInt24(&ab);
        value_t base = *a, result = value_complex(mx_one, mx_zero);
        if (mx_is_neg(&b->real)) {
            value_t one = result;
            base = complex_div(&one, &base);
        }
        while (n) {
            if (n & 1) result = complex_mul(&result, &base);
            n >>= 1;
            if (n) base = complex_mul(&base, &base);
        }
        return result;
    }
    value_t l = complex_ln(a), exponent = complex_mul(&l, b);
    return complex_exp(&exponent);
}
static value_t complex_sqrt(const value_t *z)
{
    if (complex_zero(z)) return *z;
    real_t ar = mx_abs(&z->real), ai = mx_abs(&z->imag);
    real_t scale = os_RealMax(&ar, &ai);
    real_t a = os_RealDiv(&ar, &scale), b = os_RealDiv(&ai, &scale);
    real_t r = magnitude(&a, &b);
    real_t sum = os_RealAdd(&r, &a), half = os_RealDiv(&sum, &mx_two);
    real_t t = os_RealSqrt(&half), root_scale = os_RealSqrt(&scale);
    t = os_RealMul(&root_scale, &t);
    real_t other = os_RealDiv(&z->imag, &t);
    other = os_RealDiv(&other, &mx_two);
    if (mx_is_zero(&other) && !mx_is_zero(&z->imag)) os_ThrowError(MX_E_UNDERFLOW);
    if (!mx_is_neg(&z->real)) return value_complex(t, other);
    real_t im = mx_is_neg(&z->imag) ? os_RealNeg(&t) : t;
    real_t re = mx_abs(&other);
    return value_complex(re, im);
}

static calc_err_t compute_complex(calc_cmd_t cmd, const value_t *y, const value_t *x,
                                   value_t *out)
{
    value_t a = complex_promote(y), b = complex_promote(x);
    value_t one = value_complex(mx_one, mx_zero), r;
    switch (cmd) {
        case CMD_ADD: r = complex_add(&a, &b, false); break;
        case CMD_SUB: r = complex_add(&a, &b, true); break;
        case CMD_MUL: r = complex_mul(&a, &b); break;
        case CMD_DIV: r = complex_div(&a, &b); break;
        case CMD_CHS: r = value_complex(os_RealNeg(&b.real), os_RealNeg(&b.imag)); break;
        case CMD_RECIP: r = complex_div(&one, &b); break;
        case CMD_SQUARE: r = complex_mul(&b, &b); break;
        case CMD_CUBE: { value_t sq = complex_mul(&b, &b); r = complex_mul(&sq, &b); break; }
        case CMD_SQRT: r = complex_sqrt(&b); break;
        case CMD_POW: r = complex_pow(&a, &b); break;
        case CMD_XROOT_Y: { value_t inv = complex_div(&one, &b); r = complex_pow(&a, &inv); break; }
        case CMD_CUBE_ROOT: {
            value_t third = value_complex(os_RealDiv(&mx_one, &r_three), mx_zero);
            r = complex_pow(&b, &third); break;
        }
        case CMD_LN: r = complex_ln(&b); break;
        case CMD_LOG: case CMD_LOG2: {
            value_t l = complex_ln(&b);
            value_t den = value_complex(cmd == CMD_LOG ? r_ln10 : r_ln2, mx_zero);
            r = complex_div(&l, &den); break;
        }
        case CMD_LOG_BASE: {
            value_t ly = complex_ln(&a), lx = complex_ln(&b);
            r = complex_div(&ly, &lx); break;
        }
        case CMD_EXP: r = complex_exp(&b); break;
        case CMD_ALOG: case CMD_TWO_POW: {
            value_t l = value_complex(cmd == CMD_ALOG ? r_ln10 : r_ln2, mx_zero);
            value_t exponent = complex_mul(&b, &l);
            r = complex_exp(&exponent); break;
        }
        default: return ERR_TYPE;
    }
    if ((cmd == CMD_MUL || cmd == CMD_DIV) && complex_zero(&r) &&
        !complex_zero(&a) && !complex_zero(&b)) os_ThrowError(MX_E_UNDERFLOW);
    mx_check_range(&r.real); mx_check_range(&r.imag);
    *out = r;
    return ERR_NONE;
}

static bool complex_result_needed(calc_cmd_t cmd, const real_t *y, const real_t *x)
{
    switch (cmd) {
        case CMD_SQRT: case CMD_LN: case CMD_LOG: case CMD_LOG2: return mx_is_neg(x);
        case CMD_POW: return mx_is_neg(y) && !is_integer(x);
        case CMD_XROOT_Y: return mx_is_neg(y);
        case CMD_LOG_BASE: return mx_is_neg(y) || mx_is_neg(x);
        default: return false;
    }
}

const char *value_eval(const value_context_t *ctx, calc_cmd_t cmd,
                       const value_t *y, const value_t *x, value_result_t *out)
{
    bool date_only = cmd == CMD_DATE_VALUE;
    if (date_only) {
        switch (ctx->arg) {
            case DF_CVTZ: cmd = CMD_MUL; break;
            case DF_SHRINK: cmd = CMD_SQRT; break;
            case DF_EXTEND: cmd = CMD_SQUARE; break;
            case DF_CUT: cmd = CMD_RECIP; break;
            case DF_LINK: cmd = CMD_COMPLEX_LINK; break;
            default: return "Err: Data Type";
        }
        if (!value_is_date(x) && !(ctx->arg == DF_CVTZ && value_is_date(y)))
            return "Err: Data Type";
    }
    bool rx = reads_x(cmd);
    bool ry = (cmd == CMD_BASE_OP ? base_is_binary((base_op_t)ctx->arg) : reads_y(cmd)) || (cmd == CMD_COMPLEX_LINK && x->type != VALUE_COMPLEX);
    if ((rx && !value_valid(x)) || (ry && !value_valid(y)))
        return "Err: Data Type";
    g_degrees = ctx->degrees;
    g_round_digits = cmd == CMD_ROUND_G && !ctx->arg ? 1 : ctx->arg;
    g_disp_style = ctx->disp_style; g_disp_digits = ctx->disp_digits;
    g_unit_to = ctx->arg; g_tvm_which = ctx->arg;
    g_tvm_settings = ctx->tvm; g_stats = ctx->stats;
    os_ResetFlag(ON, INTERRUPT);
    int error = os_PushErrorHandler();
    if (error) return error_text(map_os_error(error));
    memset(out, 0, sizeof *out);
    out->drop = value_binary(cmd);
    if ((rx && value_is_date(x)) || (ry && value_is_date(y))) {
        dt_err_t e = value_date_eval(cmd, y, x, out);
        os_PopErrorHandler();
        return value_date_error(e);
    }
    if (rx) { mx_check_range(&x->real); if (x->type == VALUE_COMPLEX) mx_check_range(&x->imag); }
    if (ry) { mx_check_range(&y->real); if (y->type == VALUE_COMPLEX) mx_check_range(&y->imag); }
    if (cmd == CMD_BASE_OP) {
        if (!ctx->base || !value_is_plain(x) || (ry && !value_is_plain(y))) {
            os_PopErrorHandler(); return "Err: Data Type";
        }
        base_word_t bx, by = 0, r, r2 = 0;
        base_fit_t fx = base_fit(ctx->base, &x->real, &bx);
        base_fit_t fy = ry ? base_fit(ctx->base, &y->real, &by) : BW_OK;
        if (fx != BW_OK || fy != BW_OK) {
            os_PopErrorHandler();
            return fx == BW_TOO_BIG || fy == BW_TOO_BIG ? "Err: Word Size" : "Err: Domain";
        }
        if (!base_apply(ctx->base, (base_op_t)ctx->arg, by, bx, &r, &r2)) {
            os_PopErrorHandler();
            return ((base_op_t)ctx->arg == BOP_DIV || (base_op_t)ctx->arg == BOP_DIV2) && !bx ? "Err: Division By 0" : "Err: Domain";
        }
        out->pair = (base_op_t)ctx->arg == BOP_DIV2;
        out->drop = ry && !out->pair;
        out->x = value_real(base_real(out->pair ? r2 : r));
        if (out->pair) out->y = value_real(base_real(r));
    } else if (cmd == CMD_COMPLEX_LINK) {
        if (x->type == VALUE_COMPLEX && !x->unit) {
            real_t first = x->real, second = x->imag;
            if (ctx->complex_mode != COMPLEX_RECT) {
                first = magnitude(&x->real, &x->imag);
                g_degrees = ctx->complex_mode == COMPLEX_DEG;
                second = atan2_real(&x->imag, &x->real);
            }
            out->y = value_real(first); out->x = value_real(second); out->pair = out->split = true;
            out->drop = false;
        } else if (value_is_plain(x) && value_is_plain(y)) {
            out->x = value_complex(y->real, x->real);
            if (ctx->complex_mode != COMPLEX_RECT) {
                g_degrees = ctx->complex_mode == COMPLEX_DEG;
                real_t cs = trig(CMD_COS, &x->real), sn = trig(CMD_SIN, &x->real);
                out->x = value_complex(os_RealMul(&y->real, &cs), os_RealMul(&y->real, &sn));
            }
        } else { os_PopErrorHandler(); return "Err: Data Type"; }
    } else if (cmd >= CMD_COMPLEX_REAL && cmd <= CMD_COMPLEX_ANGLE) {
        if (x->unit) { os_PopErrorHandler(); return "Err: Data Type"; }
        value_t z = complex_promote(x);
        switch (cmd) {
            case CMD_COMPLEX_REAL: out->x = value_real(z.real); break;
            case CMD_COMPLEX_IMAG: out->x = value_real(z.imag); break;
            case CMD_COMPLEX_CONJ:
                out->x = *x;
                if (x->type == VALUE_COMPLEX) out->x.imag = os_RealNeg(&x->imag);
                break;
            case CMD_COMPLEX_ABS: out->x = value_real(magnitude(&z.real, &z.imag)); break;
            case CMD_COMPLEX_ANGLE:
                out->x = value_real(value_is_real(x) ? mx_zero : atan2_real(&z.imag, &z.real)); break;
            default: break;
        }
    } else if ((rx && x->type == VALUE_COMPLEX) || (ry && y->type == VALUE_COMPLEX) ||
               (ctx->complex_results && value_is_plain(x) && (!ry || value_is_plain(y)) &&
                complex_result_needed(cmd, &y->real, &x->real))) {
        if ((rx && x->unit) || (ry && y->unit)) { os_PopErrorHandler(); return "Err: Data Type"; }
        calc_err_t e = compute_complex(cmd, y, x, &out->x);
        if (e != ERR_NONE) { os_PopErrorHandler(); return error_text(e); }
        if (value_is_real(x) && (!ry || value_is_real(y)) && mx_is_zero(&out->x.imag))
            out->x = value_real(out->x.real);
    } else {
        uint8_t tag;
        calc_err_t e = compute_real(cmd, &y->real, &x->real, ry ? y->unit : 0, rx ? x->unit : 0, &tag);
        if (e != ERR_NONE) { os_PopErrorHandler(); return error_text(e); }
        out->x = value_real(g_result); out->x.unit = tag;
        out->y = value_real(g_result2);
        out->pair = cmd == CMD_TO_POL || cmd == CMD_TO_REC || cmd == CMD_STAT_SUM ||
                    cmd == CMD_STAT_MEAN || cmd == CMD_STAT_WMEAN ||
                    cmd == CMD_STAT_SDEV || cmd == CMD_STAT_PDEV;
    }
    if (!value_is_date(&out->x)) {
        mx_check_range(&out->x.real);
        if (out->x.type == VALUE_COMPLEX) mx_check_range(&out->x.imag);
    }
    os_PopErrorHandler();
    return NULL;
}

const char *value_polar(const value_t *v, bool degrees, real_t *r, real_t *angle)
{
    value_context_t ctx = {0};
    ctx.complex_mode = degrees ? COMPLEX_DEG : COMPLEX_RAD;
    value_result_t result;
    const char *e = value_eval(&ctx, CMD_COMPLEX_LINK, v, v, &result);
    if (!e) { *r = result.y.real; *angle = result.x.real; }
    return e;
}
