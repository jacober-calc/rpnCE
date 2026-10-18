#ifndef RPNCE_CORE_CALC_H
#define RPNCE_CORE_CALC_H

/*
 * Calculator state and commands. This module knows nothing about the
 * screen or keypad: the UI turns keys into commands and reads state back.
 */

#include <stdbool.h>
#include <stdint.h>
#include <ti/real.h>

#include "tvm.h"
#include "stats.h"
#include "base.h"

#define ENTRY_MAX 34      /* room for 32 binary digits */
/* Stack: 4 levels like the HP-32SII, up to 12 with SSIZ. */
#define STACK_MIN 4
#define STACK_MAX 12

/* Registers, numbered as on the HP-32SII:
 *   001-026 A-Z, 027 i, 028-033 n Sx Sy Sx2 Sy2 Sxy (the 32SII's six stat
 *   registers), 034-040 the log sums used for curve fits, 041-140 extra
 *   registers: RSIZ sets the total, 40 to 140 (reachable through (i)). */
#define REG_A          1
#define REG_I          27
#define REG_STAT_FIRST 28
#define REG_STAT_LAST  40
#define REG_EXTRA_FIRST 41
#define REG_EXTRA_MAX  100
#define REG_BASE_COUNT 27            /* A-Z and i */
#define REG_INDIRECT   0             /* register argument meaning (i) */
#define TVM_RECALL 0x80  /* with CMD_TVM_*: recall the value (2ND + key) */

/* Stack levels, and unit tags on them: 0 for none, else unit index + 1. */
enum { REG_X, REG_Y, REG_Z, REG_T };

typedef enum {
    CMD_NONE,
    /* number entry */
    CMD_DIGIT,          /* arg = 0..9 */
    CMD_DOT,
    CMD_EE,
    CMD_CHS,
    CMD_BACKSPACE,
    CMD_CLEAR,
    /* stack */
    CMD_ENTER,
    CMD_SWAP,           /* X<>Y */
    CMD_ROLL_DOWN,      /* R-down */
    CMD_LASTX,
    CMD_SHOW,           /* show X with all 14 digits */
    /* two-argument math: Y op X */
    CMD_ADD, CMD_SUB, CMD_MUL, CMD_DIV, CMD_POW,
    /* one-argument math */
    CMD_SQUARE, CMD_SQRT, CMD_RECIP,
    CMD_LOG, CMD_ALOG, CMD_LN, CMD_EXP,
    CMD_SIN, CMD_COS, CMD_TAN, CMD_ASIN, CMD_ACOS, CMD_ATAN,
    /* constants */
    CMD_PI, CMD_E,
    /* stack menu */
    CMD_ROLL_UP, CMD_DROP, CMD_CLEAR_STACK,
    /* modes */
    CMD_MODE_RAD, CMD_MODE_DEG, CMD_MODE_ANGLE_TOGGLE,
    /* more two-argument math: Y op X */
    CMD_XROOT_Y, CMD_LOG_BASE, CMD_GCD, CMD_LCM, CMD_MOD,
    CMD_MIN, CMD_MAX, CMD_COMB, CMD_PERM, CMD_ATAN2,
    /* two-argument, Y is kept */
    CMD_PERCENT, CMD_PERCENT_CHANGE,
    /* more one-argument math */
    CMD_CUBE, CMD_CUBE_ROOT, CMD_TWO_POW, CMD_LOG2,
    CMD_EXP_MINUS_ONE, CMD_LN_ONE_PLUS,
    CMD_INT_PART, CMD_FRAC_PART, CMD_FLOOR, CMD_CEIL, CMD_NEAR,
    CMD_ABS, CMD_SIGN, CMD_FACTORIAL,
    CMD_TO_DEG, CMD_TO_RAD,
    /* NUM: prime factor and rounding (arg = digits for RNDN / RNDG) */
    CMD_PRIME, CMD_ROUND_FIX, CMD_ROUND_N, CMD_ROUND_G,
    /* HYP */
    CMD_SINH, CMD_COSH, CMD_TANH, CMD_ASINH, CMD_ACOSH, CMD_ATANH,
    /* CONV: polar/rectangular (X and Y), hours and H.MMSS */
    CMD_TO_REC, CMD_TO_POL, CMD_TO_HOURS, CMD_TO_HMS, CMD_HMS_ADD, CMD_HMS_SUB,
    /* BASE: arg = radix, or base_op_t for CMD_BASE_OP */
    CMD_BASE_RADIX, CMD_BASE_OP, CMD_SET_WORD_SIZE, CMD_GET_WORD_SIZE,
    CMD_CLEAR_CARRY, CMD_SET_CARRY, CMD_GET_CARRY,
    /* random numbers */
    CMD_RAND,           /* push a random number, 0 <= r < 1 */
    CMD_SEED,           /* restart the generator from X */
    /* display format: arg = digits 0..9, or 10+ for as many as needed */
    CMD_DISP_FIX, CMD_DISP_SCI, CMD_DISP_ENG,
    CMD_DISP_ALL,
    /* units: arg = unit index (see units.h) */
    CMD_UNIT, CMD_UNIT_VALUE, CMD_UNIT_BASE,
    /* TVM: arg = TVM_N..TVM_CYR, plus TVM_RECALL to recall instead */
    CMD_TVM_VAR, CMD_TVM_IYR1, CMD_TVM_IYR2, CMD_TVM_ITER_MAX,
    CMD_TVM_BEGIN, CMD_TVM_END, CMD_TVM_CLEAR, CMD_TVM_RESET,
    /* statistics */
    CMD_STAT_PLUS, CMD_STAT_MINUS, CMD_STAT_ALL_MODE, CMD_STAT_LIN_MODE,
    CMD_STAT_CLEAR, CMD_STAT_SUM, CMD_STAT_MEAN, CMD_STAT_WMEAN, CMD_STAT_N,
    CMD_STAT_REG,       /* arg = SUM_* */
    CMD_STAT_SDEV, CMD_STAT_SCOV, CMD_STAT_PDEV, CMD_STAT_PCOV,
    CMD_FIT_X, CMD_FIT_Y, CMD_FIT_SLOPE, CMD_FIT_INTERCEPT, CMD_FIT_CORR,
    CMD_FIT_MODEL,      /* arg = FIT_* */
    CMD_FIT_BEST,
    /* registers: arg = register number 1..140, or REG_INDIRECT for (i) */
    CMD_STO, CMD_STO_ADD, CMD_STO_SUB, CMD_STO_MUL, CMD_STO_DIV,
    CMD_RCL, CMD_RCL_ADD, CMD_RCL_SUB, CMD_RCL_MUL, CMD_RCL_DIV,
    CMD_XCH,            /* x<> register */
    CMD_CLEAR_REGS,     /* CLRG */
    CMD_SET_REG_SIZE, CMD_GET_REG_SIZE,      /* RSIZ (total, 40-140, from X), RSZ? */
    CMD_SET_STACK_SIZE, CMD_GET_STACK_SIZE,  /* SSIZ (from X), SSZ? */
} calc_cmd_t;

typedef struct {
    real_t stack[STACK_MAX];    /* stack[0] is X, [1] Y, [2] Z, [3] T, ... */
    uint8_t unit[STACK_MAX];    /* unit tag of each level */
    uint8_t depth;              /* stack levels in use: 4 to STACK_MAX */
    real_t lastx;
    uint8_t unit_lastx;

    real_t regs[REG_BASE_COUNT];        /* A-Z and i (registers 001-027) */
    real_t extra[REG_EXTRA_MAX];        /* registers 041 and up */
    uint8_t extra_count;

    tvm_settings_t tvm;
    stats_t stats;
    base_state_t base;
    bool lift;                  /* stack lift enabled */
    bool degrees;               /* angle mode: true = DEG, false = RAD */
    uint8_t disp_style;         /* DISP_FIX, DISP_SCI, DISP_ENG */
    int8_t disp_digits;         /* 0-9, or DISP_FLOAT */

    bool editing;               /* X is being typed in */
    char entry[ENTRY_MAX + 1];  /* text being typed, plain ASCII */
    uint8_t entry_len;

    calc_cmd_t last_cmd;        /* previous command, for CLEAR CLEAR */

    const char *message;        /* error or info text, or NULL */
    char show_buf[28];          /* backing store for CMD_SHOW (FORMAT_MAX) */
} calc_t;

void calc_init(calc_t *c);
void calc_execute(calc_t *c, calc_cmd_t cmd, uint8_t arg);

/* Seeds the random generator (call once at startup, e.g. from the clock). */
void calc_seed_random(uint32_t seed);

/* The register with this number (1..140), or NULL if it doesn't exist. */
real_t *calc_register(calc_t *c, uint8_t num);

/* Ends number entry, if any, so the state can be saved. */
void calc_finish_entry(calc_t *c);

#endif
