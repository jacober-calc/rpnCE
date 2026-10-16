#include <string.h>
#include <ti/error.h>

#include "calc.h"
#include "format.h"

/* ---------- constants ---------- */

static real_t r_zero, r_one, r_ten, r_ln10, r_pi, r_e;

static void init_constants(void)
{
    r_zero = os_Int24ToReal(0);
    r_one = os_Int24ToReal(1);
    r_ten = os_Int24ToReal(10);
    r_ln10 = os_RealLog(&r_ten);
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

/* Checks inputs the OS would choke on (some crash instead of erroring). */
static calc_err_t precheck(calc_cmd_t cmd, const real_t *x)
{
    real_t neg_one;
    switch (cmd) {
        case CMD_DIV:
        case CMD_RECIP:
            return is_zero(x) ? ERR_DIV0 : ERR_NONE;
        case CMD_SQRT:
            return is_neg(x) ? ERR_DOMAIN : ERR_NONE;
        case CMD_LOG:
        case CMD_LN:
            return (is_neg(x) || is_zero(x)) ? ERR_DOMAIN : ERR_NONE;
        case CMD_ASIN:
        case CMD_ACOS:
            neg_one = os_RealNeg(&r_one);
            if (os_RealCompare(x, &r_one) > 0 || os_RealCompare(x, &neg_one) < 0)
                return ERR_DOMAIN;
            return ERR_NONE;
        default:
            return ERR_NONE;
    }
}

/* Runs one math operation under the OS error handler. y is ignored by
 * one-argument operations. On success the answer is in g_result. */
static calc_err_t compute(calc_cmd_t cmd, const real_t *y, const real_t *x)
{
    calc_err_t pre = precheck(cmd, x);
    if (pre != ERR_NONE)
        return pre;

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
        /* Radians for now; DEG/RAD mode comes with the MODE menu. */
        case CMD_SIN:    g_result = os_RealSinRad(x); break;
        case CMD_COS:    g_result = os_RealCosRad(x); break;
        case CMD_TAN:    g_result = os_RealTanRad(x); break;
        case CMD_ASIN:   g_result = os_RealAsinRad(x); break;
        case CMD_ACOS:   g_result = os_RealAcosRad(x); break;
        case CMD_ATAN:   g_result = os_RealAtanRad(x); break;
        default:         g_result = *x; break;
    }

    os_PopErrorHandler();
    return ERR_NONE;
}

static bool is_binary(calc_cmd_t cmd)
{
    return cmd == CMD_ADD || cmd == CMD_SUB || cmd == CMD_MUL ||
           cmd == CMD_DIV || cmd == CMD_POW;
}

static void do_math(calc_t *c, calc_cmd_t cmd)
{
    finish_entry(c);

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

        case CMD_NONE:
            break;

        default:
            do_math(c, cmd);
            break;
    }
}
