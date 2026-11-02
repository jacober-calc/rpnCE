#include <string.h>
#include <sys/util.h>

#include "calc.h"
#include "calc_internal.h"
#include "calc_entry.h"
#include "calc_date.h"
#include "value_math.h"
#include "format.h"
#include "value_format.h"
#include "mathx.h"
#include "units.h"

/* ---------- stack primitives ---------- */

/* All stack movement copies a complete value, including its unit. */
static void lift_levels(calc_t *c)
{
    for (int k = c->depth - 1; k > 0; k--) c->stack[k] = c->stack[k - 1];
}
static void drop_levels(calc_t *c, int from)
{
    for (int k = from; k < c->depth - 1; k++) c->stack[k] = c->stack[k + 1];
}
static void save_lastx(calc_t *c) { c->lastx = c->stack[0]; }
void calc_stack_set_result(calc_t *c, const value_t *v, bool drop)
{
    value_t result = *v;
    save_lastx(c);
    if (drop) drop_levels(c, 1);
    c->stack[0] = result;
    c->lift = true;
}
void calc_stack_set_x(calc_t *c, const value_t *v) { calc_stack_set_result(c, v, false); }
void calc_stack_push(calc_t *c, const value_t *v)
{
    value_t copy = *v;
    lift_levels(c);
    c->stack[0] = copy;
}
void calc_stack_enter(calc_t *c, const value_t *v)
{
    value_t copy = *v;
    if (c->lift) calc_stack_push(c, &copy);
    else c->stack[0] = copy;
    c->lift = true;
}
static void clear_stack(calc_t *c)
{
    for (int i = 0; i < STACK_MAX; i++) c->stack[i] = value_real(mx_zero);
}
static value_context_t context(calc_t *c, uint8_t arg)
{
    value_context_t ctx = {0};
    ctx.degrees = c->degrees;
    ctx.complex_results = c->complex_results;
    ctx.complex_mode = c->complex_mode;
    ctx.disp_style = c->disp_style;
    ctx.disp_digits = c->disp_digits;
    ctx.arg = arg;
    ctx.tvm = &c->tvm;
    ctx.stats = &c->stats;
    return ctx;
}
static bool evaluate(calc_t *c, calc_cmd_t cmd, uint8_t arg,
                     const value_t *y, const value_t *x, value_result_t *out)
{
    value_context_t ctx = context(c, arg);
    c->message = value_eval(&ctx, cmd, y, x, out);
    return c->message == NULL;
}

static void do_math(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    value_result_t result;
    if (!evaluate(c, cmd, arg, &c->stack[1], &c->stack[0], &result)) return;
    if (cmd == CMD_PRIME) {
        save_lastx(c);
        calc_stack_push(c, &result.x);
        c->lift = true;
    } else if (result.split) {
        calc_stack_set_x(c, &result.y);
        calc_stack_push(c, &result.x);
    } else {
        calc_stack_set_result(c, &result.x, result.drop);
        if (result.pair) c->stack[1] = result.y;
    }
}

/* ---------- registers ---------- */

/* Stat registers 028-040, in the HP-32SII's order for the first six. */
static const uint8_t stat_reg_sum[REG_STAT_LAST - REG_STAT_FIRST + 1] = {
    SUM_N, SUM_X, SUM_Y, SUM_X2, SUM_Y2, SUM_XY,
    SUM_LX, SUM_LX2, SUM_LY, SUM_LY2, SUM_LXLY, SUM_XLY, SUM_YLX,
};

static value_t *ordinary_register(calc_t *c, uint8_t num)
{
    if (num >= REG_A && num <= REG_I) return &c->regs[num - REG_A];
    if (num >= REG_EXTRA_FIRST && num < REG_EXTRA_FIRST + c->extra_count)
        return &c->extra[num - REG_EXTRA_FIRST];
    return NULL;
}
bool calc_register_read(const calc_t *c, uint8_t num, value_t *out)
{
    if (num >= REG_STAT_FIRST && num <= REG_STAT_LAST) {
        *out = value_real(c->stats.sum[stat_reg_sum[num - REG_STAT_FIRST]]);
        return true;
    }
    if (num >= REG_A && num <= REG_I) *out = c->regs[num - REG_A];
    else if (num >= REG_EXTRA_FIRST && num < REG_EXTRA_FIRST + c->extra_count)
        *out = c->extra[num - REG_EXTRA_FIRST];
    else return false;
    return true;
}
bool calc_register_write(calc_t *c, uint8_t num, const value_t *v)
{
    if (num >= REG_STAT_FIRST && num <= REG_STAT_LAST) {
        const real_t *n = value_get_real(v);
        if (!n) { c->message = "Err: Data Type"; return false; }
        c->stats.sum[stat_reg_sum[num - REG_STAT_FIRST]] = *n;
        c->stats.constant = 0;
        if (num < REG_STAT_FIRST + 6) c->stats.lnx_ok = c->stats.lny_ok = false;
        return true;
    }
    value_t *r = ordinary_register(c, num);
    if (!r || !value_valid(v)) return false;
    *r = *v;
    return true;
}
static bool target_register(calc_t *c, uint8_t arg, uint8_t *num, value_t *out)
{
    *num = arg;
    if (arg == REG_INDIRECT) {
        const real_t *index = value_get_real(&c->regs[REG_I - REG_A]);
        if (!index) { c->message = "Err: Data Type"; return false; }
        real_t ip = os_RealInt(index);
        real_t lo = os_Int24ToReal(1), hi = os_Int24ToReal(255);
        if (os_RealCompare(&ip, &lo) < 0 || os_RealCompare(&ip, &hi) > 0) {
            c->message = "Err: Index"; return false;
        }
        *num = (uint8_t)os_RealToInt24(&ip);
    }
    if (calc_register_read(c, *num, out)) return true;
    c->message = "Err: Index";
    return false;
}
static calc_cmd_t arith_of(calc_cmd_t cmd)
{
    switch (cmd) {
        case CMD_STO_ADD: case CMD_RCL_ADD: return CMD_ADD;
        case CMD_STO_SUB: case CMD_RCL_SUB: return CMD_SUB;
        case CMD_STO_MUL: case CMD_RCL_MUL: return CMD_MUL;
        case CMD_STO_DIV: case CMD_RCL_DIV: return CMD_DIV;
        default: return CMD_NONE;
    }
}
static void do_store(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    uint8_t num;
    value_t reg;
    if (!target_register(c, arg, &num, &reg)) return;
    calc_cmd_t op = arith_of(cmd);
    value_result_t result;
    if (op == CMD_NONE) result.x = c->stack[0];
    else if (!evaluate(c, op, 0, &reg, &c->stack[0], &result)) return;
    calc_register_write(c, num, &result.x);
}
static void do_recall(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    uint8_t num;
    value_t reg;
    if (!target_register(c, arg, &num, &reg)) return;
    calc_cmd_t op = arith_of(cmd);
    if (op == CMD_NONE) calc_stack_enter(c, &reg);
    else {
        value_result_t result;
        if (evaluate(c, op, 0, &c->stack[0], &reg, &result))
            calc_stack_set_x(c, &result.x);
    }
}
static void do_exchange(calc_t *c, uint8_t arg)
{
    uint8_t num;
    value_t reg;
    if (!target_register(c, arg, &num, &reg)) return;
    if (!calc_register_write(c, num, &c->stack[0])) return;
    c->stack[0] = reg;
    c->lift = true;
}

/* A whole number from X for a size setting: below lo becomes lo (the
 * default); above hi, or not a whole number, is an error (-1). */
static int size_from_x(calc_t *c, int lo, int hi)
{
    if (!value_is_plain(&c->stack[0])) {
        c->message = "Err: Data Type";
        return -1;
    }
    real_t f = os_RealFrac(&c->stack[0].real);
    real_t a = os_Int24ToReal(lo), b = os_Int24ToReal(hi);
    if (!mx_is_zero(&f) || os_RealCompare(&c->stack[0].real, &b) > 0) {
        c->message = "Err: Domain";
        return -1;
    }
    if (os_RealCompare(&c->stack[0].real, &a) < 0)
        return lo;
    return os_RealToInt24(&c->stack[0].real);
}

/* A whole number from X in [lo, hi], or -1 (with an error message). */
static int whole_from_x(calc_t *c, int lo, int hi)
{
    if (!value_is_plain(&c->stack[0])) {
        c->message = "Err: Data Type";
        return -1;
    }
    real_t f = os_RealFrac(&c->stack[0].real);
    real_t a = os_Int24ToReal(lo), b = os_Int24ToReal(hi);
    if (!mx_is_zero(&f) || os_RealCompare(&c->stack[0].real, &a) < 0 ||
        os_RealCompare(&c->stack[0].real, &b) > 0) {
        c->message = "Err: Domain";
        return -1;
    }
    return os_RealToInt24(&c->stack[0].real);
}

/* RSIZ: X is the total number of registers, from the standard 40 (A-Z, i
 * and the stat registers) up to 140; less than 40 means 40. Registers
 * removed lose their values. */
static void set_reg_size(calc_t *c)
{
    int total = size_from_x(c, REG_STAT_LAST, REG_STAT_LAST + REG_EXTRA_MAX);
    if (total < 0)
        return;
    int n = total - REG_STAT_LAST;
    for (int i = n; i < REG_EXTRA_MAX; i++)
        c->extra[i] = value_real(mx_zero);
    c->extra_count = (uint8_t)n;
    c->message = "Register Size Set";
}

/* SSIZ: X is the stack depth, 4 to 12; less than 4 means 4. New levels
 * start at zero; levels beyond a smaller depth are lost. */
static void set_stack_size(calc_t *c)
{
    int n = size_from_x(c, STACK_MIN, STACK_MAX);
    if (n < 0)
        return;
    for (int k = n; k < STACK_MAX; k++) {
        c->stack[k] = value_real(mx_zero);
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

/* UBAS: converts X to its kind's base unit (m, kg, s, K, ...). */
static void do_unit_base(calc_t *c)
{
    uint8_t tag = c->stack[REG_X].unit;
    if (tag == 0)
        return;
    do_math(c, CMD_UNIT, unit_base_of(unit_dim((uint8_t)(tag - 1))));
}

/* ---------- BASE ---------- */

/* Runs a logic, shift, rotate, bit or word-arithmetic operation. Its
 * arguments must be whole numbers that fit the word size. */
static void do_base_op(calc_t *c, base_op_t op)
{
    base_state_t trial = c->base;
    value_context_t ctx = context(c, (uint8_t)op);
    ctx.base = &trial;
    value_result_t result;
    c->message = value_eval(&ctx, CMD_BASE_OP, &c->stack[1], &c->stack[0], &result);
    if (c->message) return;
    calc_stack_set_result(c, &result.x, result.drop);
    if (result.pair) c->stack[1] = result.y;
    c->base = trial;
}

/* In HEX, OCT and BIN the arithmetic keys work on words. */
static calc_cmd_t base_key(const calc_t *c, calc_cmd_t cmd, uint8_t *arg)
{
    if (!calc_base_mode(c) || calc_entry_is_object(c)) return cmd;
    if (cmd == CMD_CHS) {
        if (c->editing) return CMD_NONE;
        if (!value_is_plain(&c->stack[0])) return cmd;
        *arg = BOP_NEG;
        return CMD_BASE_OP;
    }
    if (!value_is_plain(&c->stack[0]) || !value_is_plain(&c->stack[1])) return cmd;
    switch (cmd) {
        case CMD_ADD: *arg = BOP_ADD; return CMD_BASE_OP;
        case CMD_SUB: *arg = BOP_SUB; return CMD_BASE_OP;
        case CMD_MUL: *arg = BOP_MUL; return CMD_BASE_OP;
        case CMD_DIV: *arg = BOP_DIV; return CMD_BASE_OP;
        default:
            return cmd;
    }
}

/* ---------- statistics ---------- */

static const char *const model_names[FIT_COUNT] = { "LINF", "LOGF", "EXPF", "PWRF" };

/* Sigma+/- replaces X with n and disables lift. Paired results enter Y then
 * X; forecasts replace X; other answers enter with the current lift rule. */
static void do_stat(calc_t *c, calc_cmd_t cmd)
{
    static stats_t trial;
    trial = c->stats;
    value_context_t ctx = context(c, 0);
    ctx.stats = &trial;
    value_result_t result;
    c->message = value_eval(&ctx, cmd, &c->stack[1], &c->stack[0], &result);
    if (c->message) return;

    c->stats = trial;

    if (result.pair) {
        calc_stack_enter(c, &result.y);
        calc_stack_push(c, &result.x);
        return;
    }
    switch (cmd) {
        case CMD_STAT_PLUS:
        case CMD_STAT_MINUS:
            calc_stack_set_x(c, &result.x);
            c->lift = false;
            break;
        case CMD_FIT_X:
        case CMD_FIT_Y:
            calc_stack_set_x(c, &result.x);
            break;
        case CMD_FIT_BEST:
            c->message = model_names[c->stats.model];
            break;
        default:
            calc_stack_enter(c, &result.x);
            break;
    }
}

/* ---------- TVM ---------- */

/* Consecutive TVM keys solve; otherwise store X. P/YR and C/YR always store.
 * TVM_RECALL bypasses both paths. */
static void do_tvm_var(calc_t *c, uint8_t arg, calc_cmd_t prev)
{
    uint8_t which = arg & ~TVM_RECALL;
    real_t *var = tvm_var(which);
    if (arg & TVM_RECALL) {
        value_t v = value_real(*var);
        calc_stack_enter(c, &v);
        return;
    }

    bool solve = which <= TVM_FV && prev == CMD_TVM_VAR;
    if (!solve && (!value_is_plain(&c->stack[0]))) {
        c->message = "Err: Data Type";
        return;
    }
    if (!solve) {
        if (which >= TVM_PYR && os_RealCompare(&c->stack[0].real, &mx_zero) <= 0) {
            c->message = "Err: Domain";
            return;
        }
        *var = c->stack[0].real;
        c->message = "TVM Stored";
        return;
    }

    value_result_t result;
    if (!evaluate(c, CMD_TVM_VAR, which, &c->stack[1], &c->stack[0], &result)) return;
    *var = result.x.real;
    calc_stack_enter(c, &result.x);
    c->message = "TVM Calculated";
}

/* IYR1, IYR2 (starting guesses for I%YR) and TMAX (most iterations). */
static void do_tvm_setting(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    tvm_settings_t *t = &c->tvm;

    if (arg & TVM_RECALL) {
        value_t v = value_real((cmd == CMD_TVM_IYR1) ? t->iyr1 :
                   (cmd == CMD_TVM_IYR2) ? t->iyr2 : os_Int24ToReal(t->iter_max));
        calc_stack_enter(c, &v);
        return;
    }

    if (!value_is_plain(&c->stack[0])) {
        c->message = "Err: Data Type";
        return;
    }
    if (cmd == CMD_TVM_ITER_MAX) {
        real_t max = os_Int24ToReal(255);
        real_t f = os_RealFrac(&c->stack[0].real);
        if (os_RealCompare(&c->stack[0].real, &mx_zero) <= 0 || !mx_is_zero(&f) ||
            os_RealCompare(&c->stack[0].real, &max) > 0) {
            c->message = "Err: Domain";
            return;
        }
        t->iter_max = (uint8_t)os_RealToInt24(&c->stack[0].real);
        t->iter_max_set = true;
    } else if (cmd == CMD_TVM_IYR1) {
        t->iyr1 = c->stack[0].real;
        t->iyr1_set = true;
    } else {
        t->iyr2 = c->stack[0].real;
        t->iyr2_set = true;
    }
    c->message = "TVM Stored";
}

/* ---------- public ---------- */

void calc_seed_random(uint32_t seed)
{
    srandom(seed);
}

void calc_init(calc_t *c)
{
    value_math_init();
    memset(c, 0, sizeof *c);
    c->depth = STACK_MIN;
    clear_stack(c);
    c->lastx = value_real(mx_zero);
    for (int i = 0; i < REG_BASE_COUNT; i++)
        c->regs[i] = value_real(mx_zero);
    for (int i = 0; i < REG_EXTRA_MAX; i++)
        c->extra[i] = value_real(mx_zero);
    c->extra_count = 0;
    c->lift = true;
    c->degrees = true;
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
    value_t tmp;
    bool was_editing = c->editing;
    calc_cmd_t prev = c->last_cmd;

    c->message = NULL;

    /* Finish and validate every entry through one parser before any consumer
     * sees it. A failed entry remains open for repair with DEL or CLEAR. */
    bool editing_key = (command_flags(cmd) & COMMAND_ENTRY) ||
        (cmd == CMD_RECORD_CLOSE && calc_entry_in_record(c));
    if (c->editing && !editing_key) {
        calc_finish_entry(c);
        if (c->editing) return;
    }
    cmd = base_key(c, cmd, &arg);
    c->last_cmd = cmd;
    if (command_flags(cmd) & COMMAND_STAT) {
        do_stat(c, cmd);
        return;
    }

    switch (cmd) {
        case CMD_COMPLEX_RECT_ENTRY: calc_entry_complex(c, 'i'); break;
        case CMD_COMPLEX_POLAR_ENTRY: calc_entry_complex(c, 'd'); break;
        case CMD_RESULT_REAL: case CMD_RESULT_COMPLEX:
            c->complex_results = cmd == CMD_RESULT_COMPLEX; break;
        case CMD_COMPLEX_RECT: case CMD_COMPLEX_RAD: case CMD_COMPLEX_DEG:
            c->complex_mode = (uint8_t)(cmd - CMD_COMPLEX_RECT); break;
        case CMD_DIGIT:     calc_entry_digit(c, arg); break;
        case CMD_DOT:       calc_entry_dot(c); break;
        case CMD_EE:        calc_entry_ee(c); break;

        case CMD_CHS:
            if (c->editing) calc_entry_chs(c);
            else do_math(c, cmd, 0);
            break;

        case CMD_BACKSPACE:
            if (c->editing)
                calc_entry_backspace(c);
            else
                calc_entry_clear(c);    /* acts as CLX */
            break;

        case CMD_CLEAR:
            if (prev == CMD_CLEAR && !c->editing) {
                clear_stack(c);                         /* CLEAR CLEAR */
                c->lift = false;
                c->message = "Stack cleared";
                c->last_cmd = CMD_NONE;                 /* a third press is CLX */
            } else {
                calc_entry_clear(c);
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
                c->regs[i] = value_real(mx_zero);
            for (int i = 0; i < REG_EXTRA_MAX; i++)
                c->extra[i] = value_real(mx_zero);
            c->message = "Registers Cleared";
            break;

        case CMD_SET_REG_SIZE:
            set_reg_size(c);
            break;

        case CMD_GET_REG_SIZE:                  /* RSZ?: the total */
            tmp = value_real(os_Int24ToReal(REG_STAT_LAST + c->extra_count));
            calc_stack_enter(c, &tmp);
            break;

        case CMD_SET_STACK_SIZE:
            set_stack_size(c);
            break;

        case CMD_GET_STACK_SIZE:
            tmp = value_real(os_Int24ToReal(c->depth));
            calc_stack_enter(c, &tmp);
            break;

        case CMD_ENTER:
            calc_stack_push(c, &c->stack[0]);
            c->lift = false;
            break;

        case CMD_SWAP:
            tmp = c->stack[0];
            c->stack[0] = c->stack[1];
            c->stack[1] = tmp;

            c->lift = true;
            break;

        case CMD_ROLL_DOWN:
            tmp = c->stack[0];               /* R-down: X goes to the top */

            drop_levels(c, 0);
            c->stack[c->depth - 1] = tmp;

            c->lift = true;
            break;

        case CMD_LASTX:
            tmp = c->lastx;
            calc_stack_enter(c, &tmp);
            break;

        case CMD_PI:
        case CMD_E: {
            value_result_t result;
            if (evaluate(c, cmd, 0, &c->stack[1], &c->stack[0], &result))
                calc_stack_enter(c, &result.x);
            break;
        }

        case CMD_SHOW:
            value_format(c->show_buf, &c->stack[0], c->disp_style, c->disp_digits,
                         c->complex_mode, c->decimal_comma, false, true);
            c->message = c->show_buf;
            break;

        case CMD_ROLL_UP:
            tmp = c->stack[c->depth - 1];    /* R-up: the top comes down to X */

            lift_levels(c);
            c->stack[0] = tmp;

            c->lift = true;
            break;

        case CMD_DROP:
            drop_levels(c, 0);
            c->lift = true;
            break;

        case CMD_CLEAR_STACK:
            clear_stack(c);
            c->lift = false;
            break;

        case CMD_UNIT:
            do_math(c, CMD_UNIT, arg);
            break;

        case CMD_UNIT_VALUE:                    /* UVAL: drop the unit */
            c->stack[REG_X].unit = 0;
            break;

        case CMD_UNIT_BASE:
            do_unit_base(c);
            break;

        case CMD_TVM_VAR:
            do_tvm_var(c, arg, was_editing ? CMD_NONE : prev);
            break;

        case CMD_TVM_IYR1:
        case CMD_TVM_IYR2:
        case CMD_TVM_ITER_MAX:
            do_tvm_setting(c, cmd, arg);
            break;

        case CMD_STAT_N:
            tmp = value_real(c->stats.sum[SUM_N]);
            calc_stack_enter(c, &tmp);
            break;

        case CMD_STAT_REG:                      /* the Sigma submenu */
            if (arg < SUM_COUNT) {
                tmp = value_real(c->stats.sum[arg]);
                calc_stack_enter(c, &tmp);
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

        case CMD_BASE_OP:
            do_base_op(c, (base_op_t)arg);
            break;

        case CMD_BASE_RADIX:
            /* Entry was completed in the previous radix. */
            c->base.radix = arg;
            base_validate(&c->base);
            break;

        case CMD_SET_WORD_SIZE: {
            int n = whole_from_x(c, 1, BASE_MAX_BITS);     /* WSIZ: 1 to 44 bits */
            if (n < 0)
                break;
            c->base.bits = (uint8_t)n;
            c->message = "Word Size Set";
            break;
        }

        case CMD_GET_WORD_SIZE:
            tmp = value_real(os_Int24ToReal(c->base.bits));
            calc_stack_enter(c, &tmp);
            break;

        case CMD_CLEAR_CARRY:
            c->base.carry = false;
            break;

        case CMD_SET_CARRY:
            c->base.carry = true;
            break;

        case CMD_GET_CARRY:
            tmp = value_real(c->base.carry ? mx_one : mx_zero);
            calc_stack_enter(c, &tmp);
            break;

        case CMD_RECORD_OPEN:
            calc_entry_record_open(c);
            break;

        case CMD_RECORD_CLOSE:                  /* 2ND }: close a record, else DROP */
            if (calc_entry_in_record(c)) {
                calc_entry_append(c, '}');
            } else {
                drop_levels(c, 0);
                c->lift = true;
            }
            break;

        case CMD_RECORD_TAG:
            calc_entry_record_tag(c, arg);
            break;

        case CMD_ALPHA_LETTER:
            calc_entry_alpha(c, arg);
            break;

        case CMD_DATE_FN:
            calc_date_execute(c, (date_fn_t)arg);
            break;

        case CMD_TVM_BEGIN:
            c->tvm.begin = true;
            break;

        case CMD_TVM_END:
            c->tvm.begin = false;
            break;

        case CMD_TVM_CLEAR:                     /* CLTV */
            tvm_clear(&c->tvm);
            c->message = "TVM Cleared";
            break;

        case CMD_TVM_RESET:                     /* RSTV */
            tvm_settings_reset(&c->tvm);
            c->message = "TVM Reset";
            break;

        case CMD_RAND:
            tmp = value_real(random_real());
            calc_stack_enter(c, &tmp);
            break;

        case CMD_SEED:
            if (!value_is_plain(&c->stack[0])) c->message = "Err: Data Type";
            else seed_from(&c->stack[0].real);           /* stack unchanged */
            break;

        /* Display and angle modes are neutral: number entry ends, but the
         * stack and stack lift are untouched. */
        case CMD_DISP_FIX:
        case CMD_DISP_SCI:
        case CMD_DISP_ENG:
            c->disp_style = (cmd == CMD_DISP_FIX) ? DISP_FIX :
                            (cmd == CMD_DISP_SCI) ? DISP_SCI : DISP_ENG;
            /* 0-11 digits, as on the HP-32SII; 12 or more means as many
             * as needed (ALL for FIX) */
            c->disp_digits = (arg > DISP_MAX_DIGITS) ? DISP_FLOAT : (int8_t)arg;
            break;

        case CMD_DISP_ALL:
            c->disp_style = DISP_FIX;
            c->disp_digits = DISP_FLOAT;
            break;

        case CMD_MODE_RAD:
            c->degrees = false;
            break;

        case CMD_DECIMAL_POINT:
        case CMD_DECIMAL_COMMA:
            c->decimal_comma = cmd == CMD_DECIMAL_COMMA;
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
            do_math(c, cmd, arg);
            break;
    }
}
