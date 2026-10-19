#include <string.h>
#include <ti/error.h>
#include <ti/flags.h>
#include <sys/util.h>

#include "calc.h"
#include "format.h"
#include "mathx.h"
#include "units.h"
#include "rounding.h"
#include "dateobj.h"

/* ---------- constants ---------- */

static real_t r_zero, r_one, r_two, r_three, r_ten, r_hundred, r_360;
static real_t r_ln10, r_ln2, r_pi, r_e, r_half_pi, r_quarter_pi, r_tenth;

static void init_constants(void)
{
    mathx_init();
    r_zero = os_Int24ToReal(0);
    r_one = os_Int24ToReal(1);
    r_two = os_Int24ToReal(2);
    r_three = os_Int24ToReal(3);
    r_ten = os_Int24ToReal(10);
    r_hundred = os_Int24ToReal(100);
    r_360 = os_Int24ToReal(360);
    r_ln10 = os_RealLog(&r_ten);
    r_ln2 = os_RealLog(&r_two);
    r_pi = os_StrToReal("3.1415926535898", NULL);
    r_e = os_RealExp(&r_one);
    r_half_pi = os_RealDiv(&r_pi, &r_two);
    real_t four = os_Int24ToReal(4);
    r_quarter_pi = os_RealDiv(&r_pi, &four);
    r_tenth = os_RealDiv(&r_one, &r_ten);
}

/* ---------- stack primitives ---------- */

/* Moves every level up one (the top level falls off). X is left as is. */
static void lift_levels(calc_t *c)
{
    for (int k = c->depth - 1; k > 0; k--) {
        c->stack[k] = c->stack[k - 1];
        c->unit[k] = c->unit[k - 1];
    }
}

/* Removes level `from`, moving the levels above it down one. The top level
 * keeps its value, so it is duplicated, as on HP calculators. */
static void drop_levels(calc_t *c, int from)
{
    for (int k = from; k < c->depth - 1; k++) {
        c->stack[k] = c->stack[k + 1];
        c->unit[k] = c->unit[k + 1];
    }
}

/* Lifts the stack and puts v (with unit tag u) in X. */
static void push_unit(calc_t *c, const real_t *v, uint8_t u)
{
    lift_levels(c);
    c->stack[0] = *v;
    c->unit[REG_X] = u;
}

static void push(calc_t *c, const real_t *v)
{
    push_unit(c, v, 0);
}

/* Puts a new value in X, lifting the stack first if lift is enabled. */
static void enter_value_unit(calc_t *c, const real_t *v, uint8_t u)
{
    if (c->lift) {
        push_unit(c, v, u);
    } else {
        c->stack[0] = *v;
        c->unit[REG_X] = u;
    }
    c->lift = true;
}

static void enter_value(calc_t *c, const real_t *v)
{
    enter_value_unit(c, v, 0);
}

static void clear_units(calc_t *c)
{
    for (int i = 0; i < STACK_MAX; i++)
        c->unit[i] = 0;
}

static void clear_stack(calc_t *c)
{
    for (int i = 0; i < STACK_MAX; i++)
        c->stack[i] = r_zero;
    clear_units(c);
}

/* ---------- number entry ---------- */

static int find_char(const calc_t *c, char ch)
{
    for (int i = 0; i < c->entry_len; i++)
        if (c->entry[i] == ch)
            return i;
    return -1;
}

static void insert_char(calc_t *c, int pos, char ch)
{
    if (c->entry_len >= ENTRY_MAX)
        return;
    memmove(&c->entry[pos + 1], &c->entry[pos], c->entry_len - pos);
    c->entry[pos] = ch;
    c->entry_len++;
    c->entry[c->entry_len] = '\0';
}

static void remove_char(calc_t *c, int pos)
{
    memmove(&c->entry[pos], &c->entry[pos + 1], c->entry_len - pos - 1);
    c->entry_len--;
    c->entry[c->entry_len] = '\0';
}

static void append_char(calc_t *c, char ch)
{
    insert_char(c, c->entry_len, ch);
}

static void begin_entry(calc_t *c)
{
    if (c->editing)
        return;
    /* Lift now so Y shows the old X while typing; X is replaced at the end. */
    if (c->lift)
        push(c, &c->stack[0]);
    c->lift = true;
    c->editing = true;
    c->entry_len = 0;
    c->entry[0] = '\0';
}

static bool base_mode(const calc_t *c)
{
    return c->base.radix != 10;
}

/* The typed digits as a word, in the current radix. */
static base_word_t entry_word(const calc_t *c)
{
    base_word_t w = 0;
    for (int i = 0; i < c->entry_len; i++)
        w = w * c->base.radix + (base_word_t)base_digit_value(&c->base, c->entry[i]);
    return w;
}

/* Turns the typed text into the X value and leaves edit mode. */
static void finish_entry(calc_t *c)
{
    if (!c->editing)
        return;
    if (dt_text_is_object(c->entry)) {
        dt_obj_t o;
        if (dt_parse(c->entry, &o) == DT_OK)
            c->stack[0] = obj_pack(&o);
        else
            c->stack[0] = r_zero;           /* only when leaving mid-record */
    } else if (base_mode(c)) {
        c->stack[0] = base_real(entry_word(c));
    } else {
        c->stack[0] = os_StrToReal(c->entry, NULL);
    }
    c->unit[REG_X] = 0;
    c->editing = false;
}

static void cancel_entry_to_zero(calc_t *c)
{
    c->editing = false;
    c->entry_len = 0;
    c->entry[0] = '\0';
    c->stack[0] = r_zero;
    c->unit[REG_X] = 0;
    c->lift = false;
}

static int mantissa_digits(const calc_t *c)
{
    int n = 0;
    for (int i = 0; i < c->entry_len && c->entry[i] != 'E'; i++)
        if (c->entry[i] >= '0' && c->entry[i] <= '9')
            n++;
    return n;
}

/* A digit in HEX, OCT or BIN: only the radix's digits, and only while the
 * number still fits the word size. */
static void entry_base_digit(calc_t *c, uint8_t d)
{
    if (d >= c->base.radix)
        return;
    begin_entry(c);
    base_word_t w = entry_word(c) * c->base.radix + d;   /* at most 48 bits */
    if (w > base_mask(&c->base) || c->entry_len >= ENTRY_MAX)
        return;
    if (c->entry_len == 1 && c->entry[0] == '0')
        c->entry_len = 0;                   /* no leading zeros */
    append_char(c, (char)(d < 10 ? '0' + d : 'A' + d - 10));
}

static bool entry_has(const calc_t *c, char ch)
{
    return find_char(c, ch) >= 0;
}

/* Typing a DATE record or a compact duration rather than a number. */
static bool entry_is_object(const calc_t *c)
{
    return c->editing && dt_text_is_object(c->entry);
}

/* Inside an open record: after the { and before any }. */
static bool in_record(const calc_t *c)
{
    return c->editing && entry_has(c, '{') && !entry_has(c, '}');
}

static void entry_digit(calc_t *c, uint8_t d)
{
    if (entry_is_object(c)) {
        if (d <= 9 && !entry_has(c, '}'))
            append_char(c, (char)('0' + d));
        return;
    }
    if (base_mode(c)) {
        entry_base_digit(c, d);
        return;
    }
    if (d > 9)
        return;                             /* A-F only mean something in HEX */
    begin_entry(c);
    int e = find_char(c, 'E');
    if (e >= 0) {
        int exp_digits = c->entry_len - e - 1;
        if (exp_digits > 0 && c->entry[e + 1] == '-')
            exp_digits--;
        if (exp_digits >= 2)
            return;
    } else if (mantissa_digits(c) >= DISPLAY_DIGITS) {   /* 12, as on the HP-32SII */
        return;
    }
    append_char(c, (char)('0' + d));
}

static void entry_dot(calc_t *c)
{
    if (base_mode(c) || entry_is_object(c))
        return;                             /* whole numbers only */
    begin_entry(c);
    if (find_char(c, '.') >= 0 || find_char(c, 'E') >= 0)
        return;
    if (mantissa_digits(c) == 0)
        append_char(c, '0');
    append_char(c, '.');
}

static void entry_ee(calc_t *c)
{
    /* Inside a record the , key separates fields; elsewhere it's EE. */
    if (in_record(c)) {
        char last = c->entry[c->entry_len - 1];
        if (last != ',' && last != '{' && last != '-')
            append_char(c, ',');
        return;
    }
    if (base_mode(c) || entry_is_object(c))
        return;
    begin_entry(c);
    if (find_char(c, 'E') >= 0)
        return;
    if (mantissa_digits(c) == 0)
        append_char(c, '1');
    append_char(c, 'E');
}

static void entry_chs(calc_t *c)
{
    if (entry_is_object(c)) {
        /* the sign of the field being typed, or of a compact duration */
        int pos = 0;
        for (int i = 0; i < c->entry_len; i++)
            if (c->entry[i] == '{' || c->entry[i] == ',')
                pos = i + 1;
        if (entry_has(c, '}'))
            return;
        if (pos < c->entry_len && c->entry[pos] == '-')
            remove_char(c, pos);
        else
            insert_char(c, pos, '-');
        return;
    }
    int e = find_char(c, 'E');
    int pos = (e >= 0) ? e + 1 : 0;
    if (pos < c->entry_len && c->entry[pos] == '-')
        remove_char(c, pos);
    else
        insert_char(c, pos, '-');
}

static void entry_backspace(calc_t *c)
{
    if (c->entry_len > 0)
        remove_char(c, c->entry_len - 1);
    if (c->entry_len == 0)
        cancel_entry_to_zero(c);
}

/* 2ND {: starts a record, after an optional type tag. */
static void entry_record_open(calc_t *c)
{
    if (!c->editing)
        begin_entry(c);
    if (entry_has(c, '{'))
        return;
    for (int i = 0; i < c->entry_len; i++)
        if (c->entry[i] < 'A' || c->entry[i] > 'Z')
            return;                         /* only after a tag, or nothing */
    append_char(c, '{');
}

/* The D{} / T{} / ... menu keys: start a record of that type. */
static void entry_record_tag(calc_t *c, uint8_t type)
{
    static const char *const tag[OBJ_COUNT] = { "", "D{", "T{", "DT{", "TZ{", "DZ{", "DW{", "DR{" };
    if (type == OBJ_NONE || type >= OBJ_COUNT)
        return;
    if (!c->editing)
        begin_entry(c);
    if (c->entry_len)
        return;
    for (const char *p = tag[type]; *p; p++)
        append_char(c, *p);
}

/* ALPHA + key: HEX digits A-F in HEX mode; otherwise a type tag letter
 * (before the {) or a compact-duration unit D, H, M, S after digits. */
static void entry_alpha(calc_t *c, uint8_t letter)
{
    if (c->base.radix == 16 && letter < 6 && !entry_is_object(c)) {
        entry_digit(c, (uint8_t)(10 + letter));
        return;
    }
    if (letter >= 26)
        return;
    char ch = (char)('A' + letter);
    if (!c->editing)
        begin_entry(c);
    if (entry_has(c, '{'))
        return;

    bool all_letters = true;
    for (int i = 0; i < c->entry_len; i++)
        if (c->entry[i] < 'A' || c->entry[i] > 'Z')
            all_letters = false;
    char last = c->entry_len ? c->entry[c->entry_len - 1] : '\0';
    bool unit = (ch == 'D' || ch == 'H' || ch == 'M' || ch == 'S');

    if (all_letters && c->entry_len < 2)
        append_char(c, ch);                         /* a tag: D, DT, TZ, ... */
    else if (unit && last >= '0' && last <= '9')
        append_char(c, ch);                         /* 1D2H, 90M, ... */
}

/* ---------- protected math ---------- */

typedef enum {
    ERR_NONE,
    ERR_DIV0,
    ERR_DOMAIN,
    ERR_OVERFLOW,
    ERR_TVM_NOT_FOUND,
    ERR_STAT,
    ERR_TYPE,           /* a DATE object where a number is needed */
    ERR_OTHER,
} calc_err_t;

static const char *error_text(calc_err_t e)
{
    switch (e) {
        case ERR_DIV0:     return "Err: Division By 0";
        case ERR_DOMAIN:   return "Err: Domain";
        case ERR_OVERFLOW: return "Err: Overflow";
        case ERR_TVM_NOT_FOUND: return "TVM Not Found";
        case ERR_STAT:     return "Err: Stat";
        case ERR_TYPE:     return "Err: Data Type";
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
        case OS_E_SIGNCHANGE & OS_E_MASK:
        case OS_E_ITERATIONS & OS_E_MASK: return ERR_TVM_NOT_FOUND;
        case OS_E_STAT & OS_E_MASK:       return ERR_STAT;
        default:                        return ERR_OTHER;
    }
}

/* Result of the last protected call. Static so it survives the error jump. */
static real_t g_result;
static real_t g_result2;        /* second answer (Y) for two-value stats */

static bool is_zero(const real_t *v) { return os_RealCompare(v, &r_zero) == 0; }
static bool is_neg(const real_t *v)  { return os_RealCompare(v, &r_zero) < 0; }

static bool is_positive(const real_t *v) { return os_RealCompare(v, &r_zero) > 0; }

static bool is_integer(const real_t *v)
{
    real_t f = os_RealFrac(v);
    return is_zero(&f);
}

/* A whole number not divisible by 2 (any size: x / 2 is checked for a
 * fractional part, so no conversion to a native integer is needed). */
static bool is_odd_integer(const real_t *v)
{
    if (!is_integer(v))
        return false;
    real_t half = os_RealDiv(v, &r_two);
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
            return is_zero(x) ? ERR_DIV0 : ERR_NONE;
        /* Negative bases follow the HP rule (as on the HP-32SII / DM32): a
         * root of a negative number needs an odd whole-number index, and a
         * power of one a whole-number exponent. Anything else is an error,
         * even where a real value exists (such as (-8)^0.4), and the OS's
         * power routine never sees a negative base with any other exponent
         * (it would return the real part of a complex result). */
        case CMD_XROOT_Y:
            if (is_zero(x))
                return ERR_DIV0;
            if (is_zero(y) && is_neg(x))
                return ERR_DOMAIN;          /* 0 to a negative power (MATH-103/104) */
            return (is_neg(y) && !is_odd_integer(x)) ? ERR_DOMAIN : ERR_NONE;
        case CMD_POW:
            return (is_neg(y) && !is_integer(x)) ? ERR_DOMAIN : ERR_NONE;
        case CMD_PERCENT_CHANGE:
            return is_zero(y) ? ERR_DIV0 : ERR_NONE;
        case CMD_SQRT:
            return is_neg(x) ? ERR_DOMAIN : ERR_NONE;
        case CMD_LOG:
        case CMD_LN:
        case CMD_LOG2:
            return is_positive(x) ? ERR_NONE : ERR_DOMAIN;
        case CMD_LOG_BASE:
            if (!is_positive(x) || !is_positive(y))
                return ERR_DOMAIN;
            return os_RealCompare(x, &r_one) == 0 ? ERR_DIV0 : ERR_NONE;
        case CMD_ASIN:
        case CMD_ACOS:
            neg_one = os_RealNeg(&r_one);
            if (os_RealCompare(x, &r_one) > 0 || os_RealCompare(x, &neg_one) < 0)
                return ERR_DOMAIN;
            return ERR_NONE;
        case CMD_LN_ONE_PLUS:
            neg_one = os_RealNeg(&r_one);
            return os_RealCompare(x, &neg_one) > 0 ? ERR_NONE : ERR_DOMAIN;
        case CMD_FACTORIAL:
            max_fact = os_Int24ToReal(69);
            if (is_neg(x) || !is_integer(x) || os_RealCompare(x, &max_fact) > 0)
                return ERR_DOMAIN;
            return ERR_NONE;
        case CMD_GCD:
        case CMD_LCM:
            if (!is_integer(x) || !is_integer(y) || is_neg(x) || is_neg(y))
                return ERR_DOMAIN;
            return ERR_NONE;
        case CMD_COMB:
        case CMD_PERM:
            /* Y items taken X at a time: whole numbers with X <= Y */
            if (!is_integer(x) || !is_integer(y) || is_neg(x) || is_neg(y) ||
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
static uint8_t g_unit_from, g_unit_to;
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
            if (q == 0 || q == 2) return r_zero;          /* tan */
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
    real_t r = r_one;
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
    real_t result = r_one;
    real_t k = *n;
    real_t count = *r;
    while (os_RealCompare(&count, &r_zero) > 0) {
        result = os_RealMul(&result, &k);
        k = os_RealSub(&k, &r_one);
        count = os_RealSub(&count, &r_one);
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
    real_t result = r_one;
    real_t i = r_one;
    while (os_RealCompare(&i, &k) <= 0) {
        real_t factor = os_RealAdd(&base, &i);
        result = os_RealMul(&result, &factor);
        result = os_RealDiv(&result, &i);
        i = os_RealAdd(&i, &r_one);
    }
    return result;
}

static real_t abs_real(const real_t *x)
{
    return is_neg(x) ? os_RealNeg(x) : *x;
}

/* ATN2: the angle of the point (X, Y), from -180 to 180 degrees (or -pi to
 * pi), correct in every quadrant. Computed from the smaller of |x|/|y| and
 * |y|/|x| so it never overflows, with exact answers on the axes and at 45
 * degrees. ATN2 of (0, 0) is 0. */
static real_t atan2_real(const real_t *y, const real_t *x)
{
    real_t ax = abs_real(x), ay = abs_real(y);
    int cmp = os_RealCompare(&ay, &ax);
    real_t a;               /* angle of (|x|, |y|): 0 to 90 degrees */

    if (is_zero(&ay)) {
        a = r_zero;
    } else if (is_zero(&ax)) {
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

    if (is_neg(x)) {                        /* left half: 180 - a */
        real_t half_turn = g_degrees ? os_Int24ToReal(180) : r_pi;
        a = os_RealSub(&half_turn, &a);
    }
    if (is_neg(y))                          /* below the x axis */
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
        return n == 2 ? r_one : r_two;
    if (n % 3 == 0)
        return n == 3 ? r_one : r_three;
    for (uint32_t k = 5; k <= n / k; k += 6) {
        if (n % k == 0)
            return base_real(k);
        if (n % (k + 2) == 0)
            return base_real(k + 2);
    }
    return r_one;
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
    real_t third = os_RealDiv(&r_one, &r_three);
    if (is_neg(x)) {
        real_t a = os_RealNeg(x);
        real_t r = os_RealPow(&a, &third);
        return os_RealNeg(&r);
    }
    return os_RealPow(x, &third);
}

/* Runs one math operation under the OS error handler. y is ignored by
 * one-argument operations. On success the answer is in g_result. */
static bool is_binary(calc_cmd_t cmd);

/* Which operands an operation reads, so a DATE object never reaches the
 * real-number routines. */
static bool reads_x(calc_cmd_t cmd)
{
    switch (cmd) {
        case CMD_TVM_VAR: case CMD_STAT_SUM: case CMD_STAT_MEAN: case CMD_STAT_WMEAN:
        case CMD_STAT_SDEV: case CMD_STAT_PDEV: case CMD_STAT_SCOV: case CMD_STAT_PCOV:
        case CMD_FIT_SLOPE: case CMD_FIT_INTERCEPT: case CMD_FIT_CORR: case CMD_FIT_BEST:
            return false;
        default:
            return true;
    }
}

static bool reads_y(calc_cmd_t cmd)
{
    return is_binary(cmd) || cmd == CMD_TO_POL || cmd == CMD_TO_REC ||
           cmd == CMD_STAT_PLUS || cmd == CMD_STAT_MINUS;
}

/* y^e, with a negative y taken from its magnitude: |y|^e, negated when the
 * result should be negative. precheck() has already made sure a negative y
 * only arrives with a whole-number power (Y^X) or the reciprocal of an odd
 * whole-number index (ˣ√Y), so the sign is:
 *   Y^X:   negative when the exponent x is odd
 *   ˣ√Y:   always negative (an odd root of a negative number)
 * The caller says which by passing the exponent: for ˣ√Y it is 1/x, which
 * isn't a whole number unless x is 1 or -1 (both odd, so negative too). */
static real_t signed_power(const real_t *y, const real_t *e)
{
    if (!is_neg(y))
        return os_RealPow(y, e);
    real_t mag = os_RealNeg(y);
    real_t r = os_RealPow(&mag, e);
    bool negative = is_integer(e) ? is_odd_integer(e) : true;
    return negative ? os_RealNeg(&r) : r;
}

static calc_err_t compute(calc_cmd_t cmd, const real_t *y, const real_t *x)
{
    if ((reads_x(cmd) && obj_is(x)) || (reads_y(cmd) && obj_is(y)))
        return ERR_TYPE;

    calc_err_t pre = precheck(cmd, y, x);
    if (pre != ERR_NONE)
        return pre;

    os_ResetFlag(ON, INTERRUPT);    /* an ON press must not become ERR:BREAK */
    int err = os_PushErrorHandler();
    if (err)
        return map_os_error(err);

    switch (cmd) {
        case CMD_ADD:    g_result = os_RealAdd(y, x); break;
        case CMD_SUB:    g_result = os_RealSub(y, x); break;
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
        case CMD_ALOG:   g_result = os_RealPow(&r_ten, x); break;
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
        case CMD_LCM:    g_result = os_RealLcm(y, x); break;
        case CMD_MOD: {
            /* y - x * floor(y / x): result takes the sign of x */
            real_t q = os_RealDiv(y, x);
            real_t f = os_RealFloor(&q);
            real_t m = os_RealMul(x, &f);
            g_result = os_RealSub(y, &m);
            break;
        }
        case CMD_MIN:    g_result = os_RealMin(y, x); break;
        case CMD_MAX:    g_result = os_RealMax(y, x); break;
        case CMD_COMB:   g_result = combinations(y, x); break;
        case CMD_PERM:   g_result = permutations(y, x); break;
        case CMD_PERCENT: {
            real_t m = os_RealMul(y, x);
            g_result = os_RealDiv(&m, &r_hundred);
            break;
        }
        case CMD_PERCENT_CHANGE: {
            real_t d = os_RealSub(x, y);
            real_t m = os_RealMul(&d, &r_hundred);
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
        case CMD_TWO_POW:   g_result = os_RealPow(&r_two, x); break;
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
        case CMD_ABS:       g_result = is_neg(x) ? os_RealNeg(x) : *x; break;
        case CMD_SIGN:
            g_result = os_Int24ToReal(is_zero(x) ? 0 : (is_neg(x) ? -1 : 1));
            break;
        case CMD_FACTORIAL: g_result = factorial(x); break;
        case CMD_UNIT:      g_result = unit_convert(x, g_unit_from, g_unit_to); break;
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
        default:         g_result = *x; break;
    }

    os_PopErrorHandler();
    return ERR_NONE;
}

static bool is_binary(calc_cmd_t cmd)
{
    switch (cmd) {
        case CMD_ADD: case CMD_SUB: case CMD_MUL: case CMD_DIV: case CMD_POW:
        case CMD_XROOT_Y: case CMD_LOG_BASE: case CMD_GCD: case CMD_LCM:
        case CMD_MOD: case CMD_MIN: case CMD_MAX: case CMD_COMB: case CMD_PERM:
        case CMD_ATAN2: case CMD_HMS_ADD: case CMD_HMS_SUB:
            return true;
        default:
            return false;
    }
}

static void do_date_binary(calc_t *c, calc_cmd_t op);
static void do_date_fn(calc_t *c, date_fn_t fn);

static void do_math(calc_t *c, calc_cmd_t cmd)
{
    finish_entry(c);

    /* DATE objects: + - x act on them, and 1/x, sqrt, x^2 are DCUT, DSHK
     * and DEXD (as in RPN83P); other math doesn't apply. */
    if (obj_is(&c->stack[0]) || (is_binary(cmd) && obj_is(&c->stack[1]))) {
        switch (cmd) {
            case CMD_ADD: case CMD_SUB: case CMD_MUL: do_date_binary(c, cmd); return;
            case CMD_RECIP:  do_date_fn(c, DF_CUT); return;
            case CMD_SQRT:   do_date_fn(c, DF_SHRINK); return;
            case CMD_SQUARE: do_date_fn(c, DF_EXTEND); return;
            default:
                c->message = "Err: Data Type";
                return;
        }
    }

    g_degrees = c->degrees;
    calc_err_t err = compute(cmd, &c->stack[1], &c->stack[0]);
    if (err != ERR_NONE) {
        c->message = error_text(err);   /* stack is left unchanged */
        return;
    }

    c->lastx = c->stack[0];
    c->unit_lastx = c->unit[REG_X];
    if (is_binary(cmd))
        drop_levels(c, 1);              /* the top level is duplicated */
    c->stack[0] = g_result;
    c->unit[REG_X] = 0;                 /* results carry no unit */
    c->lift = true;
}

/* defined with the DATE functions below */
static dt_err_t date_op(calc_cmd_t op, const real_t *y, const real_t *x, real_t *out);
static void date_error(calc_t *c, dt_err_t e);

/* ---------- registers ---------- */

/* Stat registers 028-040, in the HP-32SII's order for the first six. */
static const uint8_t stat_reg_sum[REG_STAT_LAST - REG_STAT_FIRST + 1] = {
    SUM_N, SUM_X, SUM_Y, SUM_X2, SUM_Y2, SUM_XY,
    SUM_LX, SUM_LX2, SUM_LY, SUM_LY2, SUM_LXLY, SUM_XLY, SUM_YLX,
};

real_t *calc_register(calc_t *c, uint8_t num)
{
    if (num >= REG_A && num <= REG_I)
        return &c->regs[num - REG_A];
    if (num >= REG_STAT_FIRST && num <= REG_STAT_LAST)
        return &c->stats.sum[stat_reg_sum[num - REG_STAT_FIRST]];
    if (num >= REG_EXTRA_FIRST && num < REG_EXTRA_FIRST + c->extra_count)
        return &c->extra[num - REG_EXTRA_FIRST];
    return NULL;
}

/* The register a STO/RCL/x<> argument names: a register number, or (i),
 * the register whose number is the integer part of i. NULL (with an error
 * message) if there is no such register. */
static real_t *target_register(calc_t *c, uint8_t arg, uint8_t *num_out)
{
    uint8_t num = arg;
    if (arg == REG_INDIRECT) {
        real_t ip = os_RealInt(&c->regs[REG_I - REG_A]);
        real_t lo = os_Int24ToReal(1), hi = os_Int24ToReal(255);
        if (os_RealCompare(&ip, &lo) < 0 || os_RealCompare(&ip, &hi) > 0) {
            c->message = "Err: Index";
            return NULL;
        }
        num = (uint8_t)os_RealToInt24(&ip);
    }
    real_t *r = calc_register(c, num);
    if (!r)
        c->message = "Err: Index";
    if (num_out)
        *num_out = num;
    return r;
}

/* Changing n, Sx, Sy, Sx2, Sy2 or Sxy by hand leaves the log sums behind,
 * so only the linear fit stays usable until the data is cleared. */
static void register_written(calc_t *c, uint8_t num)
{
    if (num >= REG_STAT_FIRST && num < REG_STAT_FIRST + 6)
        c->stats.lnx_ok = c->stats.lny_ok = false;
}

static calc_cmd_t arith_of(calc_cmd_t cmd)
{
    switch (cmd) {
        case CMD_STO_ADD: case CMD_RCL_ADD: return CMD_ADD;
        case CMD_STO_SUB: case CMD_RCL_SUB: return CMD_SUB;
        case CMD_STO_MUL: case CMD_RCL_MUL: return CMD_MUL;
        case CMD_STO_DIV: case CMD_RCL_DIV: return CMD_DIV;
        default:                            return CMD_NONE;
    }
}

/* STO: reg = X. STO+ etc: reg = reg op X. X and the stack are unchanged. */
static void do_store(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    finish_entry(c);
    uint8_t num;
    real_t *r = target_register(c, arg, &num);
    if (!r)
        return;

    bool stat_reg = num >= REG_STAT_FIRST && num <= REG_STAT_LAST;
    calc_cmd_t op = arith_of(cmd);
    if (stat_reg && obj_is(&c->stack[0])) {
        c->message = "Err: Data Type";      /* stat registers hold numbers */
        return;
    }
    if (op != CMD_NONE && (obj_is(r) || obj_is(&c->stack[0]))) {
        real_t out;
        dt_err_t e = date_op(op, r, &c->stack[0], &out);
        if (e) {
            date_error(c, e);
            return;
        }
        *r = out;
    } else if (op == CMD_NONE) {
        *r = c->stack[0];
    } else {
        calc_err_t err = compute(op, r, &c->stack[0]);
        if (err != ERR_NONE) {
            c->message = error_text(err);
            return;
        }
        *r = g_result;
    }
    register_written(c, num);
    /* STO is neutral: it leaves stack lift as it was, as on HP calculators. */
}

/* RCL: enters reg like a typed number. RCL+ etc: X = X op reg, saving LASTX. */
static void do_recall(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    finish_entry(c);
    real_t *r = target_register(c, arg, NULL);
    if (!r)
        return;

    calc_cmd_t op = arith_of(cmd);
    if (op == CMD_NONE) {
        real_t value = *r;
        enter_value(c, &value);     /* lifts unless lift is disabled (ENTER) */
        return;
    }

    real_t result;
    if (obj_is(r) || obj_is(&c->stack[0])) {
        dt_err_t e = date_op(op, &c->stack[0], r, &result);
        if (e) {
            date_error(c, e);
            return;
        }
    } else {
        calc_err_t err = compute(op, &c->stack[0], r);
        if (err != ERR_NONE) {
            c->message = error_text(err);
            return;
        }
        result = g_result;
    }
    c->lastx = c->stack[0];
    c->unit_lastx = c->unit[REG_X];
    c->stack[0] = result;
    c->unit[REG_X] = 0;
    c->lift = true;
}

/* x<> reg: swaps X with a register. */
static void do_exchange(calc_t *c, uint8_t arg)
{
    finish_entry(c);
    uint8_t num;
    real_t *r = target_register(c, arg, &num);
    if (!r)
        return;
    if (num >= REG_STAT_FIRST && num <= REG_STAT_LAST && obj_is(&c->stack[0])) {
        c->message = "Err: Data Type";
        return;
    }
    real_t t = *r;
    *r = c->stack[0];
    c->stack[0] = t;
    c->unit[REG_X] = 0;             /* registers hold plain numbers */
    register_written(c, num);
    c->lift = true;
}

/* A whole number from X for a size setting: below lo becomes lo (the
 * default); above hi, or not a whole number, is an error (-1). */
static int size_from_x(calc_t *c, int lo, int hi)
{
    if (obj_is(&c->stack[0])) {
        c->message = "Err: Data Type";
        return -1;
    }
    real_t f = os_RealFrac(&c->stack[0]);
    real_t a = os_Int24ToReal(lo), b = os_Int24ToReal(hi);
    if (!mx_is_zero(&f) || os_RealCompare(&c->stack[0], &b) > 0) {
        c->message = "Err: Domain";
        return -1;
    }
    if (os_RealCompare(&c->stack[0], &a) < 0)
        return lo;
    return os_RealToInt24(&c->stack[0]);
}

/* A whole number from X in [lo, hi], or -1 (with an error message). */
static int whole_from_x(calc_t *c, int lo, int hi)
{
    if (obj_is(&c->stack[0])) {
        c->message = "Err: Data Type";
        return -1;
    }
    real_t f = os_RealFrac(&c->stack[0]);
    real_t a = os_Int24ToReal(lo), b = os_Int24ToReal(hi);
    if (!mx_is_zero(&f) || os_RealCompare(&c->stack[0], &a) < 0 ||
        os_RealCompare(&c->stack[0], &b) > 0) {
        c->message = "Err: Domain";
        return -1;
    }
    return os_RealToInt24(&c->stack[0]);
}

/* RSIZ: X is the total number of registers, from the standard 40 (A-Z, i
 * and the stat registers) up to 140; less than 40 means 40. Registers
 * removed lose their values. */
static void set_reg_size(calc_t *c)
{
    finish_entry(c);
    int total = size_from_x(c, REG_STAT_LAST, REG_STAT_LAST + REG_EXTRA_MAX);
    if (total < 0)
        return;
    int n = total - REG_STAT_LAST;
    for (int i = n; i < REG_EXTRA_MAX; i++)
        c->extra[i] = r_zero;
    c->extra_count = (uint8_t)n;
    c->message = "Register Size Set";
}

/* SSIZ: X is the stack depth, 4 to 12; less than 4 means 4. New levels
 * start at zero; levels beyond a smaller depth are lost. */
static void set_stack_size(calc_t *c)
{
    finish_entry(c);
    int n = size_from_x(c, STACK_MIN, STACK_MAX);
    if (n < 0)
        return;
    for (int k = n; k < STACK_MAX; k++) {
        c->stack[k] = r_zero;
        c->unit[k] = 0;
    }
    c->depth = (uint8_t)n;
    c->message = "Stack Size Set";
}

/* ---------- random numbers ---------- */

/* A random real with 14 random digits after the decimal point. */
static real_t random_real(void)
{
    char buf[18] = "0.";
    char *p = buf + 2;
    for (int half = 0; half < 2; half++) {
        uint32_t r = random() % 10000000UL;      /* 7 digits */
        for (int i = 6; i >= 0; i--) {
            p[i] = (char)('0' + r % 10);
            r /= 10;
        }
        p += 7;
    }
    *p = '\0';
    return os_StrToReal(buf, NULL);
}

/* SEED: the same X always gives the same sequence of RAND results. */
static void seed_from(const real_t *x)
{
    const uint8_t *b = (const uint8_t *)x;
    uint32_t h = 2166136261UL;                  /* FNV-1a hash of the value */
    for (size_t i = 0; i < sizeof *x; i++) {
        h ^= b[i];
        h *= 16777619UL;
    }
    srandom(h);
}

/* ---------- units ---------- */

/* Pressing a unit: if X has no unit, X gets this one. If X has a unit of
 * the same kind, X is converted to this one (LASTX keeps the old value).
 * Units of different kinds don't mix. */
static void do_unit(calc_t *c, uint8_t to)
{
    finish_entry(c);
    if (obj_is(&c->stack[0])) {
        c->message = "Err: Data Type";      /* units go on numbers */
        return;
    }
    uint8_t tag = c->unit[REG_X];

    if (tag == 0) {
        c->unit[REG_X] = (uint8_t)(to + 1);
        c->lift = true;
        return;
    }

    uint8_t from = (uint8_t)(tag - 1);
    if (unit_dim(from) != unit_dim(to)) {
        c->message = "Err: Unit Mismatch";
        return;
    }

    g_unit_from = from;
    g_unit_to = to;
    calc_err_t err = compute(CMD_UNIT, &c->stack[1], &c->stack[0]);
    if (err != ERR_NONE) {
        c->message = error_text(err);
        return;
    }
    c->lastx = c->stack[0];
    c->unit_lastx = tag;
    c->stack[0] = g_result;
    c->unit[REG_X] = (uint8_t)(to + 1);
    c->lift = true;
}

/* UBAS: converts X to its kind's base unit (m, kg, s, K, ...). */
static void do_unit_base(calc_t *c)
{
    finish_entry(c);
    uint8_t tag = c->unit[REG_X];
    if (tag == 0)
        return;
    do_unit(c, unit_base_of(unit_dim((uint8_t)(tag - 1))));
}

/* ---------- DATE ---------- */

/* Whole numbers between reals and 64-bit integers (split into millions so
 * the OS's 24-bit conversions can handle each part). */
static bool real_to_whole(const real_t *v, dt_int *out)
{
    if (obj_is(v))
        return false;
    real_t f = os_RealFrac(v);
    real_t lim = os_StrToReal("8E12", NULL);
    real_t a = mx_abs(v);
    if (!mx_is_zero(&f) || os_RealCompare(&a, &lim) >= 0)
        return false;
    real_t mil = os_Int24ToReal(1000000);
    real_t q = os_RealDiv(v, &mil);
    q = os_RealInt(&q);
    real_t qm = os_RealMul(&q, &mil);
    real_t r = os_RealSub(v, &qm);
    *out = (dt_int)os_RealToInt24(&q) * 1000000 + os_RealToInt24(&r);
    return true;
}

static real_t whole_to_real(dt_int n)
{
    real_t q = os_Int24ToReal((int)(n / 1000000));
    real_t r = os_Int24ToReal((int)(n % 1000000));
    real_t mil = os_Int24ToReal(1000000);
    real_t t = os_RealMul(&q, &mil);
    return os_RealAdd(&t, &r);
}

/* A stack value as a DATE operand: an object, or a number (whole, and in
 * quarter hours for TimeZone arithmetic). */
static dt_val_t date_val(const real_t *v)
{
    dt_val_t d;
    memset(&d, 0, sizeof d);
    if (obj_is(v)) {
        d.is_obj = true;
        d.obj = obj_unpack(v);
        return d;
    }
    d.is_whole = real_to_whole(v, &d.whole);
    real_t four = os_Int24ToReal(4);
    real_t q = os_RealMul(v, &four);
    d.is_quarter = real_to_whole(&q, &d.quarters);
    return d;
}

static void date_error(calc_t *c, dt_err_t e)
{
    c->message = (e == DT_ERR_TYPE) ? "Err: Data Type" :
                 (e == DT_ERR_DOMAIN) ? "Err: Domain" :
                 (e == DT_ERR_SYNTAX) ? "Err: Syntax" : "Err: Invalid";
}

static real_t result_real(const dt_result_t *r)
{
    if (r->is_obj)
        return obj_pack(&r->obj);
    real_t n = whole_to_real(r->num);
    if (r->den == 1)
        return n;
    real_t d = os_Int24ToReal(r->den);
    return os_RealDiv(&n, &d);
}

/* CVTZ (also the x key): a Date, DateTime or ZonedDateTime and a TimeZone
 * or hours, in either order, give a ZonedDateTime in that zone. */
static dt_err_t date_cvtz(const dt_val_t *a, const dt_val_t *b, dt_result_t *out)
{
    const dt_val_t *when = NULL, *zone = NULL;
    for (int k = 0; k < 2; k++) {
        const dt_val_t *v = k ? b : a, *w = k ? a : b;
        if (v->is_obj && (v->obj.type == OBJ_DATE || v->obj.type == OBJ_DATETIME ||
                          v->obj.type == OBJ_DZ)) {
            when = v;
            zone = w;
        }
    }
    if (!when)
        return DT_ERR_TYPE;
    int q;
    if (zone->is_obj && zone->obj.type == OBJ_TZ)
        q = dt_tz_quarters(&zone->obj);
    else if (!zone->is_obj && zone->is_quarter)
        q = (int)zone->quarters;
    else
        return zone->is_obj ? DT_ERR_TYPE : DT_ERR_DOMAIN;
    out->is_obj = true;
    return dt_convert_tz(&when->obj, q, &out->obj);
}

/* y op x where either may be an object: + and -, and x for CVTZ. */
static dt_err_t date_op(calc_cmd_t op, const real_t *y, const real_t *x, real_t *out)
{
    dt_val_t a = date_val(y), b = date_val(x);
    dt_result_t r;
    dt_err_t e;
    if (op == CMD_ADD)
        e = dt_add(&a, &b, &r);
    else if (op == CMD_SUB)
        e = dt_sub(&a, &b, &r);
    else if (op == CMD_MUL)
        e = date_cvtz(&a, &b, &r);
    else
        e = DT_ERR_TYPE;
    if (e == DT_OK)
        *out = result_real(&r);
    return e;
}

/* y op x on the stack, for objects. */
static void do_date_binary(calc_t *c, calc_cmd_t op)
{
    real_t r;
    dt_err_t e = date_op(op, &c->stack[1], &c->stack[0], &r);
    if (e) {
        date_error(c, e);
        return;
    }
    c->lastx = c->stack[0];
    c->unit_lastx = c->unit[REG_X];
    drop_levels(c, 1);
    c->stack[0] = r;
    c->unit[REG_X] = 0;
    c->lift = true;
}

static dt_int epoch_days(const calc_t *c)
{
    switch (c->epoch) {
        case EPOCH_NTP:    return dt_days_from_civil(1900, 1, 1);
        case EPOCH_GPS:    return dt_days_from_civil(1980, 1, 6);
        case EPOCH_TIOS:   return dt_days_from_civil(1997, 1, 1);
        case EPOCH_Y2K:    return dt_days_from_civil(2000, 1, 1);
        case EPOCH_CUSTOM: return dt_days_from_civil(c->custom_epoch.year,
                                   c->custom_epoch.month, c->custom_epoch.day);
        default:           return 0;
    }
}

/* Replace X with a result (LASTX keeps the old X). */
static void set_x(calc_t *c, const real_t *v)
{
    c->lastx = c->stack[0];
    c->unit_lastx = c->unit[REG_X];
    c->stack[0] = *v;
    c->unit[REG_X] = 0;
    c->lift = true;
}

static void set_x_obj(calc_t *c, const dt_obj_t *o)
{
    real_t v = obj_pack(o);
    set_x(c, &v);
}

static void set_x_whole(calc_t *c, dt_int n)
{
    real_t v = whole_to_real(n);
    set_x(c, &v);
}

static void push_obj(calc_t *c, const dt_obj_t *o)
{
    real_t v = obj_pack(o);
    enter_value(c, &v);
}

/* The hardware clock as a ZonedDateTime in the clock's zone. */
static bool clock_now(const calc_t *c, dt_obj_t *out)
{
    int y, mo, d, h, mi, s;
    if (!sys_clock_read(&y, &mo, &d, &h, &mi, &s))
        return false;
    memset(out, 0, sizeof *out);
    out->type = OBJ_DZ;
    out->year = (int16_t)y; out->month = (int8_t)mo; out->day = (int8_t)d;
    out->hour = (int8_t)h; out->minute = (int8_t)mi; out->second = (int8_t)s;
    out->tz_hour = (int8_t)(c->clock_tz / 4);
    out->tz_minute = (int8_t)((c->clock_tz % 4) * 15);
    return dt_validate(out) == DT_OK;
}

/* A time zone from X: a TimeZone object or hours (multiples of 0.25). */
static bool tz_from_x(calc_t *c, int *q)
{
    dt_val_t v = date_val(&c->stack[0]);
    if (v.is_obj && v.obj.type == OBJ_TZ) {
        *q = dt_tz_quarters(&v.obj);
        return true;
    }
    if (!v.is_obj && v.is_quarter && v.quarters >= -95 && v.quarters <= 95) {
        *q = (int)v.quarters;
        return true;
    }
    date_error(c, v.is_obj ? DT_ERR_TYPE : DT_ERR_DOMAIN);
    return false;
}

static void do_date_fn(calc_t *c, date_fn_t fn)
{
    finish_entry(c);
    dt_val_t x = date_val(&c->stack[0]);
    uint8_t t = x.is_obj ? x.obj.type : OBJ_NONE;
    bool dated = (t == OBJ_DATE || t == OBJ_DATETIME || t == OBJ_DZ);
    dt_obj_t o;
    dt_int n;
    dt_err_t e = DT_OK;
    int q;

    memset(&o, 0, sizeof o);
    switch (fn) {
        /* ---- epoch days and seconds ---- */
        case DF_TO_ED:
        case DF_TO_ES:
            if (!dated) { e = DT_ERR_TYPE; break; }
            dt_to_epoch_seconds(&x.obj, epoch_days(c), &n);
            if (fn == DF_TO_ED) {               /* the time is dropped */
                n = (n >= 0) ? n / 86400 : -((-n + 86399) / 86400);
            }
            set_x_whole(c, n);
            return;
        case DF_ED_TO_D: case DF_ED_TO_DT: case DF_ED_TO_DZ:
        case DF_ES_TO_D: case DF_ES_TO_DT: case DF_ES_TO_DZ: case DF_ES_TO_DZ_APP: {
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
            if (!x.is_whole) { e = DT_ERR_DOMAIN; break; }
            bool days = (fn == DF_ED_TO_D || fn == DF_ED_TO_DT || fn == DF_ED_TO_DZ);
            obj_type_t ty = (fn == DF_ED_TO_D || fn == DF_ES_TO_D) ? OBJ_DATE :
                            (fn == DF_ED_TO_DT || fn == DF_ES_TO_DT) ? OBJ_DATETIME : OBJ_DZ;
            int tzq = (fn == DF_ES_TO_DZ_APP) ? c->app_tz : 0;
            e = dt_from_epoch_seconds(days ? x.whole * 86400 : x.whole, epoch_days(c),
                                      ty, tzq, &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        }
        case DF_LEAP:
            if (dated)
                n = dt_is_leap(x.obj.year);
            else if (!x.is_obj && x.is_whole && x.whole >= 1 && x.whole <= 9999)
                n = dt_is_leap((int)x.whole);
            else { e = x.is_obj ? DT_ERR_TYPE : DT_ERR_DOMAIN; break; }
            set_x_whole(c, n);
            return;
        case DF_DOW:
            if (!dated) { e = DT_ERR_TYPE; break; }
            o.type = OBJ_DW;
            o.day = (int8_t)dt_iso_dow(dt_days_from_civil(x.obj.year, x.obj.month, x.obj.day));
            set_x_obj(c, &o);
            return;
        case DF_CVTZ:
            do_date_binary(c, CMD_MUL);
            return;

        /* ---- components ---- */
        case DF_YEAR: case DF_MONTH:
            if (!dated) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, fn == DF_YEAR ? x.obj.year : x.obj.month);
            return;
        case DF_DAY:
            if (dated) { set_x_whole(c, x.obj.day); return; }
            if (t == OBJ_DR) { set_x_whole(c, x.obj.year); return; }
            e = DT_ERR_TYPE;
            break;
        case DF_HOUR: case DF_MINUTE: case DF_SECOND:
            if (t == OBJ_TIME || t == OBJ_DATETIME || t == OBJ_DZ || t == OBJ_DR) {
                set_x_whole(c, fn == DF_HOUR ? x.obj.hour :
                               fn == DF_MINUTE ? x.obj.minute : x.obj.second);
                return;
            }
            e = DT_ERR_TYPE;
            break;
        case DF_TZ_HOUR: case DF_TZ_MINUTE:
            if (t != OBJ_TZ && t != OBJ_DZ) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, fn == DF_TZ_HOUR ? x.obj.tz_hour : x.obj.tz_minute);
            return;
        case DF_GET_DATE: case DF_GET_TIME: case DF_GET_DT: case DF_GET_TZ:
            if (!(t == OBJ_DATETIME || t == OBJ_DZ) ||
                ((fn == DF_GET_DT || fn == DF_GET_TZ) && t != OBJ_DZ)) {
                e = DT_ERR_TYPE;
                break;
            }
            o = x.obj;
            o.type = (fn == DF_GET_DATE) ? OBJ_DATE : (fn == DF_GET_TIME) ? OBJ_TIME :
                     (fn == DF_GET_DT) ? OBJ_DATETIME : OBJ_TZ;
            if (o.type == OBJ_DATE)
                o.hour = o.minute = o.second = 0;
            if (o.type != OBJ_TZ)
                o.tz_hour = o.tz_minute = 0;
            set_x_obj(c, &o);
            return;

        /* ---- casting ---- */
        case DF_SHRINK:                         /* DT -> D, DZ -> DT */
            if (t != OBJ_DATETIME && t != OBJ_DZ) { e = DT_ERR_TYPE; break; }
            o = x.obj;
            o.type = (t == OBJ_DZ) ? OBJ_DATETIME : OBJ_DATE;
            if (o.type == OBJ_DATE)
                o.hour = o.minute = o.second = 0;
            o.tz_hour = o.tz_minute = 0;
            set_x_obj(c, &o);
            return;
        case DF_EXTEND:                         /* D -> DT (midnight), DT -> DZ (UTC) */
            if (t != OBJ_DATE && t != OBJ_DATETIME) { e = DT_ERR_TYPE; break; }
            o = x.obj;
            o.type = (t == OBJ_DATE) ? OBJ_DATETIME : OBJ_DZ;
            set_x_obj(c, &o);
            return;
        case DF_CUT: {                          /* DT -> D and T; DZ -> DT and TZ */
            if (t != OBJ_DATETIME && t != OBJ_DZ) { e = DT_ERR_TYPE; break; }
            dt_obj_t first = x.obj, second = x.obj;
            if (t == OBJ_DATETIME) {
                first.type = OBJ_DATE;
                first.hour = first.minute = first.second = 0;
                second.type = OBJ_TIME;
                second.year = 0; second.month = second.day = 0;
            } else {
                first.type = OBJ_DATETIME;
                first.tz_hour = first.tz_minute = 0;
                memset(&second, 0, sizeof second);
                second.type = OBJ_TZ;
                second.tz_hour = x.obj.tz_hour;
                second.tz_minute = x.obj.tz_minute;
            }
            set_x_obj(c, &first);               /* first in Y, second in X */
            real_t v = obj_pack(&second);
            push(c, &v);
            return;
        }
        case DF_LINK: {                         /* D + T -> DT; DT + TZ -> DZ */
            dt_val_t y = date_val(&c->stack[1]);
            if (!x.is_obj || !y.is_obj) { e = DT_ERR_TYPE; break; }
            const dt_obj_t *a = &y.obj, *b = &x.obj;
            if (a->type > b->type) { const dt_obj_t *s = a; a = b; b = s; }
            if (a->type == OBJ_DATE && b->type == OBJ_TIME) {
                o = *a;
                o.type = OBJ_DATETIME;
                o.hour = b->hour; o.minute = b->minute; o.second = b->second;
            } else if (a->type == OBJ_DATETIME && b->type == OBJ_TZ) {
                o = *a;
                o.type = OBJ_DZ;
                o.tz_hour = b->tz_hour; o.tz_minute = b->tz_minute;
            } else {
                e = DT_ERR_TYPE;
                break;
            }
            c->lastx = c->stack[0];
            c->unit_lastx = c->unit[REG_X];
            drop_levels(c, 1);
            c->stack[0] = obj_pack(&o);
            c->unit[REG_X] = 0;
            c->lift = true;
            return;
        }

        /* ---- Time, TimeZone, Duration, DayOfWeek ---- */
        case DF_TIME_TO_S:
            if (t != OBJ_TIME) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, dt_time_seconds(&x.obj));
            return;
        case DF_S_TO_TIME:
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
            if (!x.is_whole) { e = DT_ERR_DOMAIN; break; }
            e = dt_time_from_seconds(x.whole, &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        case DF_TZ_TO_HR: {
            if (t != OBJ_TZ) { e = DT_ERR_TYPE; break; }
            real_t qq = os_Int24ToReal(dt_tz_quarters(&x.obj)), four = os_Int24ToReal(4);
            real_t h = os_RealDiv(&qq, &four);
            set_x(c, &h);
            return;
        }
        case DF_HR_TO_TZ:
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
            if (!x.is_quarter) { e = DT_ERR_DOMAIN; break; }
            e = dt_tz_from_quarters(x.quarters, &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        case DF_DAYS_TO_DR: case DF_HOURS_TO_DR: case DF_MINUTES_TO_DR: case DF_SECONDS_TO_DR: {
            static const dt_int unit[4] = { 86400, 3600, 60, 1 };
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
            if (!x.is_whole) { e = DT_ERR_DOMAIN; break; }
            e = dt_duration_from_seconds(x.whole * unit[fn - DF_DAYS_TO_DR], &o);
            if (!e) { set_x_obj(c, &o); return; }
            break;
        }
        case DF_DR_TO_S:
            if (t != OBJ_DR) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, dt_duration_seconds(&x.obj));
            return;
        case DF_DW_TO_ISO: case DF_DW_TO_UNIX:
            if (t != OBJ_DW) { e = DT_ERR_TYPE; break; }
            set_x_whole(c, fn == DF_DW_TO_ISO ? x.obj.day : x.obj.day % 7);
            return;
        case DF_ISO_TO_DW: case DF_UNIX_TO_DW: {
            if (x.is_obj) { e = DT_ERR_TYPE; break; }
            dt_int lo = (fn == DF_ISO_TO_DW) ? 1 : 0;
            if (!x.is_whole || x.whole < lo || x.whole > lo + 6) { e = DT_ERR_DOMAIN; break; }
            o.type = OBJ_DW;
            o.day = (int8_t)(x.whole == 0 ? 7 : x.whole);
            set_x_obj(c, &o);
            return;
        }

        /* ---- epochs ---- */
        case DF_EPOCH_UNIX: case DF_EPOCH_NTP: case DF_EPOCH_GPS:
        case DF_EPOCH_TIOS: case DF_EPOCH_Y2K: case DF_EPOCH_CUSTOM:
            c->epoch = (uint8_t)(fn - DF_EPOCH_UNIX);
            return;
        case DF_EPOCH_SET_CUSTOM:
            if (t != OBJ_DATE) { e = DT_ERR_TYPE; break; }
            c->custom_epoch = x.obj;
            c->epoch = EPOCH_CUSTOM;
            c->message = "Epoch Set";
            return;
        case DF_EPOCH_GET_CUSTOM:
            push_obj(c, &c->custom_epoch);
            return;

        /* ---- clock ---- */
        case DF_NOW: case DF_NOW_DATE: case DF_NOW_TIME: case DF_NOW_DZ: case DF_NOW_DZ_UTC: {
            dt_obj_t now;
            if (!clock_now(c, &now)) {
                c->message = "Err: Clock";
                return;
            }
            if (fn == DF_NOW) {
                dt_to_epoch_seconds(&now, epoch_days(c), &n);
                real_t v = whole_to_real(n);
                enter_value(c, &v);
                return;
            }
            dt_convert_tz(&now, fn == DF_NOW_DZ_UTC ? 0 : c->app_tz, &o);
            if (fn == DF_NOW_DATE) {
                o.type = OBJ_DATE;
                o.hour = o.minute = o.second = 0;
                o.tz_hour = o.tz_minute = 0;
            } else if (fn == DF_NOW_TIME) {
                o.type = OBJ_TIME;
                o.year = 0; o.month = o.day = 0;
                o.tz_hour = o.tz_minute = 0;
            }
            push_obj(c, &o);
            return;
        }
        case DF_SET_APP_TZ:
        case DF_SET_CLOCK_TZ:
            if (!tz_from_x(c, &q))
                return;
            if (fn == DF_SET_APP_TZ)
                c->app_tz = (int8_t)q;
            else
                c->clock_tz = (int8_t)q;
            c->message = "Time Zone Set";
            return;
        case DF_GET_APP_TZ:
        case DF_GET_CLOCK_TZ:
            dt_tz_from_quarters(fn == DF_GET_APP_TZ ? c->app_tz : c->clock_tz, &o);
            push_obj(c, &o);
            return;
        case DF_SET_CLOCK:
            /* a ZonedDateTime, shown in the clock's own zone */
            if (t != OBJ_DZ && t != OBJ_DATETIME) { e = DT_ERR_TYPE; break; }
            if (t == OBJ_DATETIME) {            /* taken as already in the clock's zone */
                o = x.obj;
            } else {
                e = dt_convert_tz(&x.obj, c->clock_tz, &o);
                if (e)
                    break;
            }
            sys_clock_write(o.year, o.month, o.day, o.hour, o.minute, o.second);
            c->message = "Clock Set";
            return;

        /* ---- display ---- */
        case DF_FORMAT_RAW:
        case DF_FORMAT_STRING:
            c->date_as_string = (fn == DF_FORMAT_STRING);
            return;
    }
    if (e)
        date_error(c, e);
}

/* ---------- conversions with two answers ---------- */

/* ->POL and ->REC replace X and Y with the converted pair (HP-42S style):
 * ->POL takes x in X and y in Y and gives r in X and the angle in Y. */
static void do_polar_rect(calc_t *c, calc_cmd_t cmd)
{
    finish_entry(c);
    g_degrees = c->degrees;
    calc_err_t err = compute(cmd, &c->stack[1], &c->stack[0]);
    if (err != ERR_NONE) {
        c->message = error_text(err);
        return;
    }
    c->lastx = c->stack[0];
    c->unit_lastx = c->unit[REG_X];
    c->stack[0] = g_result;
    c->stack[1] = g_result2;
    c->unit[REG_X] = c->unit[REG_Y] = 0;
    c->lift = true;
}

/* ---------- BASE ---------- */

/* Runs a logic, shift, rotate, bit or word-arithmetic operation. Its
 * arguments must be whole numbers that fit the word size. */
static void do_base_op(calc_t *c, base_op_t op)
{
    finish_entry(c);
    bool binary = base_is_binary(op);
    base_word_t x, y = 0, r, r2 = 0;
    if (obj_is(&c->stack[0]) || (binary && obj_is(&c->stack[1]))) {
        c->message = "Err: Data Type";
        return;
    }
    /* both operands must be words of the current size */
    base_fit_t fx = base_fit(&c->base, &c->stack[0], &x);
    base_fit_t fy = binary ? base_fit(&c->base, &c->stack[1], &y) : BW_OK;
    if (fx != BW_OK || fy != BW_OK) {
        c->message = (fx == BW_TOO_BIG || fy == BW_TOO_BIG) ? "Err: Word Size"
                                                            : "Err: Domain";
        return;
    }
    base_state_t trial = c->base;           /* carry changes only on success */
    if (!base_apply(&trial, op, y, x, &r, &r2)) {
        c->message = (op == BOP_DIV || op == BOP_DIV2) && x == 0 ?
                     "Err: Division By 0" : "Err: Domain";
        return;
    }
    c->base = trial;

    c->lastx = c->stack[0];
    c->unit_lastx = c->unit[REG_X];
    if (op == BOP_DIV2) {                   /* BDIV: quotient in Y, remainder in X */
        c->stack[1] = base_real(r);
        c->stack[0] = base_real(r2);
        c->unit[REG_Y] = 0;
    } else {
        if (binary)
            drop_levels(c, 1);
        c->stack[0] = base_real(r);
    }
    c->unit[REG_X] = 0;
    c->lift = true;
}

/* In HEX, OCT and BIN the arithmetic keys work on words. */
static calc_cmd_t base_key(const calc_t *c, calc_cmd_t cmd, uint8_t *arg)
{
    if (!base_mode(c) || obj_is(&c->stack[0]) || obj_is(&c->stack[1]) ||
        entry_is_object(c))
        return cmd;
    switch (cmd) {
        case CMD_ADD: *arg = BOP_ADD; return CMD_BASE_OP;
        case CMD_SUB: *arg = BOP_SUB; return CMD_BASE_OP;
        case CMD_MUL: *arg = BOP_MUL; return CMD_BASE_OP;
        case CMD_DIV: *arg = BOP_DIV; return CMD_BASE_OP;
        case CMD_CHS:
            if (c->editing)
                return CMD_NONE;            /* no signs while typing */
            *arg = BOP_NEG;
            return CMD_BASE_OP;
        default:
            return cmd;
    }
}

/* ---------- statistics ---------- */

static const char *const model_names[FIT_COUNT] = { "LINF", "LOGF", "EXPF", "PWRF" };

/* Runs a statistics command under the error handler and puts its answer on
 * the stack, the way the HP-42S does:
 *   Sigma+ / Sigma-   x from X, y from Y; X becomes n, LASTX the old X, and
 *                     stack lift is disabled (the next number replaces n)
 *   SUM MEAN WMN SDEV PDEV   x answer in X, y answer in Y
 *   forecasts         replace X (LASTX keeps it)
 *   everything else   one answer pushed onto the stack */
static void do_stat(calc_t *c, calc_cmd_t cmd)
{
    finish_entry(c);
    g_stats = &c->stats;
    calc_err_t err = compute(cmd, &c->stack[1], &c->stack[0]);
    if (err != ERR_NONE) {
        c->message = error_text(err);
        return;
    }

    switch (cmd) {
        case CMD_STAT_PLUS:
        case CMD_STAT_MINUS:
            c->lastx = c->stack[0];
            c->unit_lastx = c->unit[REG_X];
            c->stack[0] = g_result;
            c->unit[REG_X] = 0;
            c->lift = false;
            break;
        case CMD_STAT_SUM: case CMD_STAT_MEAN: case CMD_STAT_WMEAN:
        case CMD_STAT_SDEV: case CMD_STAT_PDEV: {
            real_t y = g_result2, x = g_result;
            enter_value(c, &y);
            push(c, &x);
            break;
        }
        case CMD_FIT_X:
        case CMD_FIT_Y:
            c->lastx = c->stack[0];
            c->unit_lastx = c->unit[REG_X];
            c->stack[0] = g_result;
            c->unit[REG_X] = 0;
            c->lift = true;
            break;
        case CMD_FIT_BEST:
            c->message = model_names[c->stats.model];
            break;
        default:
            enter_value(c, &g_result);
            break;
    }
}

/* ---------- TVM ---------- */

static bool is_positive_real(const real_t *v)
{
    return !mx_is_neg(v) && !mx_is_zero(v);
}

/* N, I%YR, PV, PMT, FV: like an HP-12C, the key stores X, unless the last
 * key was a TVM key too, in which case it solves for that value. P/YR and
 * C/YR always store. With TVM_RECALL (2ND + key) the value is recalled. */
static void do_tvm_var(calc_t *c, uint8_t arg, calc_cmd_t prev)
{
    uint8_t which = arg & ~TVM_RECALL;
    real_t *var = tvm_var(which);
    bool was_editing = c->editing;
    finish_entry(c);

    if (arg & TVM_RECALL) {
        real_t v = *var;
        enter_value(c, &v);
        return;
    }

    bool solve = which <= TVM_FV && prev == CMD_TVM_VAR && !was_editing;
    if (!solve && obj_is(&c->stack[0])) {
        c->message = "Err: Data Type";
        return;
    }
    if (!solve) {
        if (which >= TVM_PYR && !is_positive_real(&c->stack[0])) {
            c->message = "Err: Domain";
            return;
        }
        *var = c->stack[0];
        c->message = "TVM Stored";
        return;
    }

    g_tvm_which = which;
    g_tvm_settings = &c->tvm;
    calc_err_t err = compute(CMD_TVM_VAR, &c->stack[1], &c->stack[0]);
    if (err != ERR_NONE) {
        c->message = error_text(err);
        return;
    }
    *var = g_result;
    enter_value(c, &g_result);
    c->message = "TVM Calculated";
}

/* IYR1, IYR2 (starting guesses for I%YR) and TMAX (most iterations). */
static void do_tvm_setting(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    finish_entry(c);
    tvm_settings_t *t = &c->tvm;

    if (arg & TVM_RECALL) {
        real_t v = (cmd == CMD_TVM_IYR1) ? t->iyr1 :
                   (cmd == CMD_TVM_IYR2) ? t->iyr2 : os_Int24ToReal(t->iter_max);
        enter_value(c, &v);
        return;
    }

    if (obj_is(&c->stack[0])) {
        c->message = "Err: Data Type";
        return;
    }
    if (cmd == CMD_TVM_ITER_MAX) {
        real_t max = os_Int24ToReal(255);
        real_t f = os_RealFrac(&c->stack[0]);
        if (!is_positive_real(&c->stack[0]) || !mx_is_zero(&f) ||
            os_RealCompare(&c->stack[0], &max) > 0) {
            c->message = "Err: Domain";
            return;
        }
        t->iter_max = (uint8_t)os_RealToInt24(&c->stack[0]);
        t->iter_max_set = true;
    } else if (cmd == CMD_TVM_IYR1) {
        t->iyr1 = c->stack[0];
        t->iyr1_set = true;
    } else {
        t->iyr2 = c->stack[0];
        t->iyr2_set = true;
    }
    c->message = "TVM Stored";
}

/* ---------- public ---------- */

void calc_seed_random(uint32_t seed)
{
    srandom(seed);
}

void calc_finish_entry(calc_t *c)
{
    finish_entry(c);
}

void calc_init(calc_t *c)
{
    init_constants();
    memset(c, 0, sizeof *c);
    c->depth = STACK_MIN;
    clear_stack(c);
    c->lastx = r_zero;
    for (int i = 0; i < REG_BASE_COUNT; i++)
        c->regs[i] = r_zero;
    for (int i = 0; i < REG_EXTRA_MAX; i++)
        c->extra[i] = r_zero;
    c->extra_count = 0;
    c->lift = true;
    c->disp_style = DISP_FIX;
    c->disp_digits = DISP_FLOAT;
    tvm_settings_reset(&c->tvm);
    c->tvm.begin = false;
    c->stats.linear_only = false;
    c->stats.model = FIT_LIN;
    stats_clear(&c->stats);
    base_reset(&c->base);
    c->date_as_string = false;
    c->epoch = EPOCH_UNIX;
    memset(&c->custom_epoch, 0, sizeof c->custom_epoch);
    c->custom_epoch.type = OBJ_DATE;            /* 2050-01-01, as on RPN83P */
    c->custom_epoch.year = 2050;
    c->custom_epoch.month = 1;
    c->custom_epoch.day = 1;
    c->app_tz = 0;
    c->clock_tz = 0;
}

void calc_execute(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    real_t tmp;
    uint8_t u;
    calc_cmd_t prev = c->last_cmd;

    cmd = base_key(c, cmd, &arg);
    c->message = NULL;

    /* A record being typed is checked before anything else uses it. A bad
     * one keeps the entry open, as on RPN83P, so DEL can fix it. */
    if (entry_is_object(c)) {
        bool editing_key = cmd == CMD_DIGIT || cmd == CMD_DOT || cmd == CMD_EE ||
                           cmd == CMD_CHS || cmd == CMD_BACKSPACE || cmd == CMD_CLEAR ||
                           cmd == CMD_RECORD_OPEN || cmd == CMD_ALPHA_LETTER ||
                           cmd == CMD_RECORD_TAG || (cmd == CMD_RECORD_CLOSE && in_record(c));
        if (!editing_key) {
            dt_obj_t o;
            dt_err_t e = dt_parse(c->entry, &o);
            if (e) {
                date_error(c, e);
                return;
            }
            c->stack[0] = obj_pack(&o);
            c->unit[REG_X] = 0;
            c->editing = false;
        }
    }
    c->last_cmd = cmd;

    switch (cmd) {
        case CMD_DIGIT:     entry_digit(c, arg); break;
        case CMD_DOT:       entry_dot(c); break;
        case CMD_EE:        entry_ee(c); break;

        case CMD_CHS:
            if (c->editing)
                entry_chs(c);
            else if (obj_is(&c->stack[0]))
                c->message = "Err: Data Type";
            else
                c->stack[0] = os_RealNeg(&c->stack[0]);
            break;

        case CMD_BACKSPACE:
            if (c->editing)
                entry_backspace(c);
            else
                cancel_entry_to_zero(c);    /* acts as CLX */
            break;

        case CMD_CLEAR:
            if (prev == CMD_CLEAR && !c->editing) {
                clear_stack(c);                         /* CLEAR CLEAR */
                c->lift = false;
                c->message = "Stack cleared";
                c->last_cmd = CMD_NONE;                 /* a third press is CLX */
            } else {
                cancel_entry_to_zero(c);
            }
            break;

        case CMD_STO: case CMD_STO_ADD: case CMD_STO_SUB:
        case CMD_STO_MUL: case CMD_STO_DIV:
            do_store(c, cmd, arg);
            break;

        case CMD_RCL: case CMD_RCL_ADD: case CMD_RCL_SUB:
        case CMD_RCL_MUL: case CMD_RCL_DIV:
            do_recall(c, cmd, arg);
            break;

        case CMD_XCH:
            do_exchange(c, arg);
            break;

        case CMD_CLEAR_REGS:                    /* CLRG */
            for (int i = 0; i < REG_BASE_COUNT; i++)
                c->regs[i] = r_zero;
            for (int i = 0; i < REG_EXTRA_MAX; i++)
                c->extra[i] = r_zero;
            c->message = "Registers Cleared";
            break;

        case CMD_SET_REG_SIZE:
            set_reg_size(c);
            break;

        case CMD_GET_REG_SIZE:                  /* RSZ?: the total */
            finish_entry(c);
            tmp = os_Int24ToReal(REG_STAT_LAST + c->extra_count);
            enter_value(c, &tmp);
            break;

        case CMD_SET_STACK_SIZE:
            set_stack_size(c);
            break;

        case CMD_GET_STACK_SIZE:
            finish_entry(c);
            tmp = os_Int24ToReal(c->depth);
            enter_value(c, &tmp);
            break;

        case CMD_ENTER:
            finish_entry(c);
            push_unit(c, &c->stack[0], c->unit[REG_X]);
            c->lift = false;
            break;

        case CMD_SWAP:
            finish_entry(c);
            tmp = c->stack[0];
            c->stack[0] = c->stack[1];
            c->stack[1] = tmp;
            u = c->unit[REG_X];
            c->unit[REG_X] = c->unit[REG_Y];
            c->unit[REG_Y] = u;
            c->lift = true;
            break;

        case CMD_ROLL_DOWN:
            finish_entry(c);
            tmp = c->stack[0];               /* R-down: X goes to the top */
            u = c->unit[REG_X];
            drop_levels(c, 0);
            c->stack[c->depth - 1] = tmp;
            c->unit[c->depth - 1] = u;
            c->lift = true;
            break;

        case CMD_LASTX:
            finish_entry(c);
            tmp = c->lastx;
            enter_value_unit(c, &tmp, c->unit_lastx);
            break;

        case CMD_PI:
            finish_entry(c);
            enter_value(c, &r_pi);
            break;

        case CMD_E:
            finish_entry(c);
            enter_value(c, &r_e);
            break;

        case CMD_SHOW:
            finish_entry(c);
            if (obj_is(&c->stack[0])) {
                dt_obj_t o = obj_unpack(&c->stack[0]);
                dt_format(c->show_buf, &o, false);  /* SHOW: the raw record */
            } else {
                format_real(c->show_buf, &c->stack[0], DISP_FIX, DISP_FLOAT,
                            SHOW_DIGITS, SHOW_DIGITS, true);
            }
            c->message = c->show_buf;
            break;

        case CMD_ROLL_UP:
            finish_entry(c);
            tmp = c->stack[c->depth - 1];    /* R-up: the top comes down to X */
            u = c->unit[c->depth - 1];
            lift_levels(c);
            c->stack[0] = tmp;
            c->unit[REG_X] = u;
            c->lift = true;
            break;

        case CMD_DROP:
            finish_entry(c);
            drop_levels(c, 0);
            c->lift = true;
            break;

        case CMD_CLEAR_STACK:
            finish_entry(c);
            clear_stack(c);
            c->lift = false;
            break;

        case CMD_UNIT:
            do_unit(c, arg);
            break;

        case CMD_UNIT_VALUE:                    /* UVAL: drop the unit */
            finish_entry(c);
            c->unit[REG_X] = 0;
            break;

        case CMD_UNIT_BASE:
            do_unit_base(c);
            break;

        case CMD_TVM_VAR:
            do_tvm_var(c, arg, prev);
            break;

        case CMD_TVM_IYR1:
        case CMD_TVM_IYR2:
        case CMD_TVM_ITER_MAX:
            do_tvm_setting(c, cmd, arg);
            break;

        case CMD_STAT_PLUS: case CMD_STAT_MINUS: case CMD_STAT_SUM:
        case CMD_STAT_MEAN: case CMD_STAT_WMEAN: case CMD_STAT_SDEV:
        case CMD_STAT_PDEV: case CMD_STAT_SCOV: case CMD_STAT_PCOV:
        case CMD_FIT_X: case CMD_FIT_Y: case CMD_FIT_SLOPE:
        case CMD_FIT_INTERCEPT: case CMD_FIT_CORR: case CMD_FIT_BEST:
            do_stat(c, cmd);
            break;

        case CMD_STAT_N:
            finish_entry(c);
            tmp = c->stats.sum[SUM_N];
            enter_value(c, &tmp);
            break;

        case CMD_STAT_REG:                      /* the Sigma submenu */
            finish_entry(c);
            if (arg < SUM_COUNT) {
                tmp = c->stats.sum[arg];
                enter_value(c, &tmp);
            }
            break;

        case CMD_STAT_CLEAR:
            stats_clear(&c->stats);
            c->message = "Stats Cleared";
            break;

        case CMD_STAT_ALL_MODE:
        case CMD_STAT_LIN_MODE: {
            bool lin = (cmd == CMD_STAT_LIN_MODE);
            if (lin != c->stats.linear_only) {
                c->stats.linear_only = lin;
                /* log sums are only good if kept from the first point */
                bool empty = mx_is_zero(&c->stats.sum[SUM_N]);
                c->stats.lnx_ok = c->stats.lny_ok = !lin && empty;
                if (lin)
                    c->stats.model = FIT_LIN;
            }
            break;
        }

        case CMD_FIT_MODEL:
            if (arg < FIT_COUNT && stats_model_ok(&c->stats, arg))
                c->stats.model = arg;
            else
                c->message = "Err: Stat";
            break;

        case CMD_ROUND_FIX:
            g_disp_style = c->disp_style;
            g_disp_digits = c->disp_digits;
            do_math(c, cmd);
            break;

        case CMD_ROUND_N:
        case CMD_ROUND_G:
            g_round_digits = arg;
            if (cmd == CMD_ROUND_G && arg == 0)
                g_round_digits = 1;
            do_math(c, cmd);
            break;

        case CMD_TO_POL:
        case CMD_TO_REC:
            do_polar_rect(c, cmd);
            break;

        case CMD_BASE_OP:
            do_base_op(c, (base_op_t)arg);
            break;

        case CMD_BASE_RADIX:
            finish_entry(c);                /* digits typed in the old radix */
            c->base.radix = arg;
            base_validate(&c->base);
            break;

        case CMD_SET_WORD_SIZE: {
            finish_entry(c);
            int n = whole_from_x(c, 1, BASE_MAX_BITS);     /* WSIZ: 1 to 44 bits */
            if (n < 0)
                break;
            c->base.bits = (uint8_t)n;
            c->message = "Word Size Set";
            break;
        }

        case CMD_GET_WORD_SIZE:
            finish_entry(c);
            tmp = os_Int24ToReal(c->base.bits);
            enter_value(c, &tmp);
            break;

        case CMD_CLEAR_CARRY:
            c->base.carry = false;
            break;

        case CMD_SET_CARRY:
            c->base.carry = true;
            break;

        case CMD_GET_CARRY:
            finish_entry(c);
            tmp = c->base.carry ? r_one : r_zero;
            enter_value(c, &tmp);
            break;

        case CMD_RECORD_OPEN:
            entry_record_open(c);
            break;

        case CMD_RECORD_CLOSE:                  /* 2ND }: close a record, else DROP */
            if (in_record(c)) {
                append_char(c, '}');
            } else {
                finish_entry(c);
                drop_levels(c, 0);
                c->lift = true;
            }
            break;

        case CMD_RECORD_TAG:
            entry_record_tag(c, arg);
            break;

        case CMD_ALPHA_LETTER:
            entry_alpha(c, arg);
            break;

        case CMD_DATE_FN:
            do_date_fn(c, (date_fn_t)arg);
            break;

        case CMD_TVM_BEGIN:
            c->tvm.begin = true;
            break;

        case CMD_TVM_END:
            c->tvm.begin = false;
            break;

        case CMD_TVM_CLEAR:                     /* CLTV */
            finish_entry(c);
            tvm_clear(&c->tvm);
            c->message = "TVM Cleared";
            break;

        case CMD_TVM_RESET:                     /* RSTV */
            tvm_settings_reset(&c->tvm);
            c->message = "TVM Reset";
            break;

        case CMD_RAND:
            finish_entry(c);
            tmp = random_real();
            enter_value(c, &tmp);
            break;

        case CMD_SEED:
            finish_entry(c);
            seed_from(&c->stack[0]);           /* stack unchanged */
            break;

        /* Display and angle modes are neutral: number entry ends, but the
         * stack and stack lift are untouched. */
        case CMD_DISP_FIX:
        case CMD_DISP_SCI:
        case CMD_DISP_ENG:
            finish_entry(c);
            c->disp_style = (cmd == CMD_DISP_FIX) ? DISP_FIX :
                            (cmd == CMD_DISP_SCI) ? DISP_SCI : DISP_ENG;
            /* 0-11 digits, as on the HP-32SII; 12 or more means as many
             * as needed (FIX-) */
            c->disp_digits = (arg > DISP_MAX_DIGITS) ? DISP_FLOAT : (int8_t)arg;
            break;

        case CMD_DISP_ALL:
            finish_entry(c);
            c->disp_style = DISP_FIX;
            c->disp_digits = DISP_FLOAT;
            break;

        case CMD_MODE_RAD:
            c->degrees = false;
            break;

        case CMD_MODE_DEG:
            c->degrees = true;
            break;

        case CMD_MODE_ANGLE_TOGGLE:
            c->degrees = !c->degrees;
            break;

        case CMD_NONE:
            break;

        default:
            do_math(c, cmd);
            break;
    }
}
