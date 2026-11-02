#ifndef RPNCE_CORE_COMMANDS_H
#define RPNCE_CORE_COMMANDS_H

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
    /* DATE: typing records, and the DATE menu functions */
    CMD_RECORD_OPEN,    /* 2ND { */
    CMD_RECORD_CLOSE,   /* 2ND }: closes a record being typed, else DROP */
    CMD_RECORD_TAG,     /* arg = obj_type_t: D{} etc. from the menus */
    CMD_ALPHA_LETTER,   /* arg = 0..25: ALPHA + key (type tags, HEX digits) */
    CMD_DATE_FN,        /* arg = date_fn_t */
    /* random numbers */
    CMD_RAND,           /* push a random number, 0 <= r < 1 */
    CMD_SEED,           /* restart the generator from X */
    /* display format: arg = digits 0..11, or 12+ for as many as needed */
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
    CMD_DECIMAL_POINT, CMD_DECIMAL_COMMA,
    CMD_COMPLEX_RECT_ENTRY, CMD_COMPLEX_POLAR_ENTRY, CMD_COMPLEX_LINK,
    CMD_COMPLEX_REAL, CMD_COMPLEX_IMAG, CMD_COMPLEX_CONJ, CMD_COMPLEX_ABS, CMD_COMPLEX_ANGLE,
    CMD_RESULT_REAL, CMD_RESULT_COMPLEX,
    CMD_COMPLEX_RECT, CMD_COMPLEX_RAD, CMD_COMPLEX_DEG,
    CMD_DATE_VALUE,     /* arg = structural date_fn_t; DATE operands required */
} calc_cmd_t;

#endif
