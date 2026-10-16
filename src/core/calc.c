#include <string.h>
#include <ti/error.h>
#include <ti/flags.h>

#include "calc.h"
#include "format.h"

/* ---------- constants ---------- */

static real_t r_zero, r_one, r_two, r_three, r_ten, r_hundred, r_360;
static real_t r_ln10, r_ln2, r_pi, r_e;

static void init_constants(void)
{
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
}

/* ---------- stack primitives ---------- */

static void push(calc_t *c, const real_t *v)
{
    c->t = c->z;
    c->z = c->y;
    c->y = c->x;
    c->x = *v;
}

/* Puts a new value in X, lifting the stack first if lift is enabled. */
static void enter_value(calc_t *c, const real_t *v)
{
    if (c->lift)
        push(c, v);
    else
        c->x = *v;
    c->lift = true;
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
        push(c, &c->x);
    c->lift = true;
    c->editing = true;
    c->entry_len = 0;
    c->entry[0] = '\0';
}

/* Turns the typed text into the X value and leaves edit mode. */
static void finish_entry(calc_t *c)
{
    if (!c->editing)
        return;
    c->x = os_StrToReal(c->entry, NULL);
    c->editing = false;
}

static void cancel_entry_to_zero(calc_t *c)
{
    c->editing = false;
    c->entry_len = 0;
    c->entry[0] = '\0';
    c->x = r_zero;
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

static void entry_digit(calc_t *c, uint8_t d)
{
    begin_entry(c);
    int e = find_char(c, 'E');
    if (e >= 0) {
        int exp_digits = c->entry_len - e - 1;
        if (exp_digits > 0 && c->entry[e + 1] == '-')
            exp_digits--;
        if (exp_digits >= 2)
            return;
    } else if (mantissa_digits(c) >= 14) {
        return;
    }
    append_char(c, (char)('0' + d));
}

static void entry_dot(calc_t *c)
{
    begin_entry(c);
    if (find_char(c, '.') >= 0 || find_char(c, 'E') >= 0)
        return;
    if (mantissa_digits(c) == 0)
        append_char(c, '0');
    append_char(c, '.');
}

static void entry_ee(calc_t *c)
{
    begin_entry(c);
    if (find_char(c, 'E') >= 0)
        return;
    if (mantissa_digits(c) == 0)
        append_char(c, '1');
    append_char(c, 'E');
}

static void entry_chs(calc_t *c)
{
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

/* ---------- protected math ---------- */

typedef enum {
    ERR_NONE,
    ERR_DIV0,
    ERR_DOMAIN,
    ERR_OVERFLOW,
    ERR_OTHER,
} calc_err_t;

static const char *error_text(calc_err_t e)
{
    switch (e) {
        case ERR_DIV0:     return "Err: Division By 0";
        case ERR_DOMAIN:   return "Err: Domain";
        case ERR_OVERFLOW: return "Err: Overflow";
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
        default:                        return ERR_OTHER;
    }
}

/* Result of the last protected call. Static so it survives the error jump. */
static real_t g_result;

static bool is_zero(const real_t *v) { return os_RealCompare(v, &r_zero) == 0; }
static bool is_neg(const real_t *v)  { return os_RealCompare(v, &r_zero) < 0; }

static bool is_positive(const real_t *v) { return os_RealCompare(v, &r_zero) > 0; }

static bool is_integer(const real_t *v)
{
    real_t f = os_RealFrac(v);
    return is_zero(&f);
}

/* Checks inputs the OS would choke on (some crash instead of erroring). */
static calc_err_t precheck(calc_cmd_t cmd, const real_t *y, const real_t *x)
{
    real_t neg_one, max_fact;
    switch (cmd) {
        case CMD_DIV:
        case CMD_RECIP:
        case CMD_XROOT_Y:
        case CMD_MOD:
            return is_zero(x) ? ERR_DIV0 : ERR_NONE;
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
        case CMD_FACTORIAL:
            max_fact = os_Int24ToReal(69);
            if (is_neg(x) || !is_integer(x) || os_RealCompare(x, &max_fact) > 0)
                return ERR_DOMAIN;
            return ERR_NONE;
        case CMD_GCD:
        case CMD_LCM:
        case CMD_COMB:
        case CMD_PERM:
            if (!is_integer(x) || !is_integer(y) || is_neg(x) || is_neg(y))
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
static calc_err_t compute(calc_cmd_t cmd, const real_t *y, const real_t *x)
{
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
        case CMD_POW:    g_result = os_RealPow(y, x); break;
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
            g_result = os_RealPow(y, &inv);
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
        case CMD_COMB:   g_result = os_RealNcr(y, x); break;
        case CMD_PERM:   g_result = os_RealNpr(y, x); break;
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
            return true;
        default:
            return false;
    }
}

static void do_math(calc_t *c, calc_cmd_t cmd)
{
    finish_entry(c);

    g_degrees = c->degrees;
    calc_err_t err = compute(cmd, &c->y, &c->x);
    if (err != ERR_NONE) {
        c->message = error_text(err);   /* stack is left unchanged */
        return;
    }

    c->lastx = c->x;
    if (is_binary(cmd)) {
        c->y = c->z;
        c->z = c->t;                    /* T is duplicated on a drop */
    }
    c->x = g_result;
    c->lift = true;
}

/* ---------- public ---------- */

void calc_init(calc_t *c)
{
    init_constants();
    memset(c, 0, sizeof *c);
    c->x = c->y = c->z = c->t = c->lastx = r_zero;
    c->lift = true;
}

void calc_execute(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    real_t tmp;

    c->message = NULL;

    switch (cmd) {
        case CMD_DIGIT:     entry_digit(c, arg); break;
        case CMD_DOT:       entry_dot(c); break;
        case CMD_EE:        entry_ee(c); break;

        case CMD_CHS:
            if (c->editing)
                entry_chs(c);
            else
                c->x = os_RealNeg(&c->x);
            break;

        case CMD_BACKSPACE:
            if (c->editing)
                entry_backspace(c);
            else
                cancel_entry_to_zero(c);    /* acts as CLX */
            break;

        case CMD_CLEAR:
            cancel_entry_to_zero(c);
            break;

        case CMD_ENTER:
            finish_entry(c);
            push(c, &c->x);
            c->lift = false;
            break;

        case CMD_SWAP:
            finish_entry(c);
            tmp = c->x;
            c->x = c->y;
            c->y = tmp;
            c->lift = true;
            break;

        case CMD_ROLL_DOWN:
            finish_entry(c);
            tmp = c->x;
            c->x = c->y;
            c->y = c->z;
            c->z = c->t;
            c->t = tmp;
            c->lift = true;
            break;

        case CMD_LASTX:
            finish_entry(c);
            tmp = c->lastx;
            enter_value(c, &tmp);
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
            format_real(c->show_buf, sizeof c->show_buf, &c->x, true);
            c->message = c->show_buf;
            break;

        case CMD_ROLL_UP:
            finish_entry(c);
            tmp = c->t;
            c->t = c->z;
            c->z = c->y;
            c->y = c->x;
            c->x = tmp;
            c->lift = true;
            break;

        case CMD_DROP:
            finish_entry(c);
            c->x = c->y;
            c->y = c->z;
            c->z = c->t;
            c->lift = true;
            break;

        case CMD_CLEAR_STACK:
            finish_entry(c);
            c->x = c->y = c->z = c->t = r_zero;
            c->lift = false;
            break;

        case CMD_MODE_RAD:
            c->degrees = false;
            break;

        case CMD_MODE_DEG:
            c->degrees = true;
            break;

        case CMD_NONE:
            break;

        default:
            do_math(c, cmd);
            break;
    }
}
