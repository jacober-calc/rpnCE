#include "actions.h"
#include "res.h"
#include "../core/format.h"

/* A menu action is data: one command byte and one argument byte. Missing
 * entries are CMD_NONE. Unit actions still come from RPNCEres, so their
 * indices stay tied to the resource table. */
_Static_assert(CMD_NONE == 0 && CMD_GET_STACK_SIZE <= UINT8_MAX,
               "menu actions must fit in a byte");
static const struct { uint8_t cmd, arg; } actions[] = {
    /* TVM, statistics, curve-fit models and radix */
    [MID_TVM_N] = { CMD_TVM_VAR, TVM_N },
    [MID_TVM_IYR] = { CMD_TVM_VAR, TVM_IYR },
    [MID_TVM_PV] = { CMD_TVM_VAR, TVM_PV },
    [MID_TVM_PMT] = { CMD_TVM_VAR, TVM_PMT },
    [MID_TVM_FV] = { CMD_TVM_VAR, TVM_FV },
    [MID_TVM_PYR] = { CMD_TVM_VAR, TVM_PYR },
    [MID_TVM_CYR] = { CMD_TVM_VAR, TVM_CYR },
    [MID_TVM_IYR0] = { CMD_TVM_IYR1, 0 },
    [MID_TVM_IYR1] = { CMD_TVM_IYR2, 0 },
    [MID_TVM_ITER_MAX] = { CMD_TVM_ITER_MAX, 0 },
    [MID_STAT_REG_N] = { CMD_STAT_REG, SUM_N },
    [MID_STAT_REG_X] = { CMD_STAT_REG, SUM_X },
    [MID_STAT_REG_X2] = { CMD_STAT_REG, SUM_X2 },
    [MID_STAT_REG_Y] = { CMD_STAT_REG, SUM_Y },
    [MID_STAT_REG_Y2] = { CMD_STAT_REG, SUM_Y2 },
    [MID_STAT_REG_XY] = { CMD_STAT_REG, SUM_XY },
    [MID_STAT_REG_LN_X] = { CMD_STAT_REG, SUM_LX },
    [MID_STAT_REG_LN_X2] = { CMD_STAT_REG, SUM_LX2 },
    [MID_STAT_REG_LN_Y] = { CMD_STAT_REG, SUM_LY },
    [MID_STAT_REG_LN_Y2] = { CMD_STAT_REG, SUM_LY2 },
    [MID_STAT_REG_LN_XLN_Y] = { CMD_STAT_REG, SUM_LXLY },
    [MID_STAT_REG_XLN_Y] = { CMD_STAT_REG, SUM_XLY },
    [MID_STAT_REG_YLN_X] = { CMD_STAT_REG, SUM_YLX },
    [MID_CFIT_LINEAR] = { CMD_FIT_MODEL, FIT_LIN },
    [MID_CFIT_LOG] = { CMD_FIT_MODEL, FIT_LOG },
    [MID_CFIT_EXP] = { CMD_FIT_MODEL, FIT_EXP },
    [MID_CFIT_POWER] = { CMD_FIT_MODEL, FIT_PWR },
    [MID_DEC] = { CMD_BASE_RADIX, 10 },
    [MID_HEX] = { CMD_BASE_RADIX, 16 },
    [MID_OCT] = { CMD_BASE_RADIX, 8 },
    [MID_BIN] = { CMD_BASE_RADIX, 2 },
    /* DATE functions */
    [MID_DATE_TO_EPOCH_DAYS] = { CMD_DATE_FN, DF_TO_ED },
    [MID_EPOCH_DAYS_TO_DATE] = { CMD_DATE_FN, DF_ED_TO_D },
    [MID_DATE_TO_EPOCH_SECONDS] = { CMD_DATE_FN, DF_TO_ES },
    [MID_EPOCH_SECONDS_TO_DATE] = { CMD_DATE_FN, DF_ES_TO_D },
    [MID_IS_DATE_LEAP] = { CMD_DATE_FN, DF_LEAP },
    [MID_DATE_TO_DAY_OF_WEEK] = { CMD_DATE_FN, DF_DOW },
    [MID_DATE_CONVERT_TO_TIME_ZONE] = { CMD_DATE_FN, DF_CVTZ },
    [MID_DATE_EXTRACT_YEAR] = { CMD_DATE_FN, DF_YEAR },
    [MID_DATE_EXTRACT_MONTH] = { CMD_DATE_FN, DF_MONTH },
    [MID_DATE_EXTRACT_DAY] = { CMD_DATE_FN, DF_DAY },
    [MID_DATE_SHRINK_TO_NOTHING] = { CMD_DATE_FN, DF_SHRINK },
    [MID_DATE_EXTEND_TO_DATE_TIME] = { CMD_DATE_FN, DF_EXTEND },
    [MID_DATE_CUT_TO_NOTHING] = { CMD_DATE_FN, DF_CUT },
    [MID_DATE_LINK_TO_DATE_TIME] = { CMD_DATE_FN, DF_LINK },
    [MID_TIME_TO_SECONDS] = { CMD_DATE_FN, DF_TIME_TO_S },
    [MID_SECONDS_TO_TIME] = { CMD_DATE_FN, DF_S_TO_TIME },
    [MID_TIME_EXTRACT_HOUR] = { CMD_DATE_FN, DF_HOUR },
    [MID_TIME_EXTRACT_MINUTE] = { CMD_DATE_FN, DF_MINUTE },
    [MID_TIME_EXTRACT_SECOND] = { CMD_DATE_FN, DF_SECOND },
    [MID_DATE_TIME_TO_EPOCH_DAYS] = { CMD_DATE_FN, DF_TO_ED },
    [MID_EPOCH_DAYS_TO_DATE_TIME] = { CMD_DATE_FN, DF_ED_TO_DT },
    [MID_DATE_TIME_TO_EPOCH_SECONDS] = { CMD_DATE_FN, DF_TO_ES },
    [MID_EPOCH_SECONDS_TO_DATE_TIME] = { CMD_DATE_FN, DF_ES_TO_DT },
    [MID_IS_DATE_TIME_LEAP] = { CMD_DATE_FN, DF_LEAP },
    [MID_DATE_TIME_TO_DAY_OF_WEEK] = { CMD_DATE_FN, DF_DOW },
    [MID_DATE_TIME_CONVERT_TO_TIME_ZONE] = { CMD_DATE_FN, DF_CVTZ },
    [MID_DATE_TIME_EXTRACT_DATE] = { CMD_DATE_FN, DF_GET_DATE },
    [MID_DATE_TIME_EXTRACT_TIME] = { CMD_DATE_FN, DF_GET_TIME },
    [MID_DATE_TIME_SHRINK_TO_DATE] = { CMD_DATE_FN, DF_SHRINK },
    [MID_DATE_TIME_EXTEND_TO_OFFSET_DATE_TIME] = { CMD_DATE_FN, DF_EXTEND },
    [MID_DATE_TIME_CUT_TO_DATE] = { CMD_DATE_FN, DF_CUT },
    [MID_DATE_TIME_LINK_TO_OFFSET_DATE_TIME] = { CMD_DATE_FN, DF_LINK },
    [MID_OFFSET_TO_HOURS] = { CMD_DATE_FN, DF_TZ_TO_HR },
    [MID_HOURS_TO_OFFSET] = { CMD_DATE_FN, DF_HR_TO_TZ },
    [MID_OFFSET_EXTRACT_HOUR] = { CMD_DATE_FN, DF_TZ_HOUR },
    [MID_OFFSET_EXTRACT_MINUTE] = { CMD_DATE_FN, DF_TZ_MINUTE },
    [MID_OFFSET_DATE_TIME_TO_EPOCH_DAYS] = { CMD_DATE_FN, DF_TO_ED },
    [MID_EPOCH_DAYS_TO_OFFSET_DATE_TIME] = { CMD_DATE_FN, DF_ED_TO_DZ },
    [MID_OFFSET_DATE_TIME_TO_EPOCH_SECONDS] = { CMD_DATE_FN, DF_TO_ES },
    [MID_EPOCH_SECONDS_TO_OFFSET_DATE_TIME_UTC] = { CMD_DATE_FN, DF_ES_TO_DZ },
    [MID_EPOCH_SECONDS_TO_OFFSET_DATE_TIME_APP] = { CMD_DATE_FN, DF_ES_TO_DZ_APP },
    [MID_IS_OFFSET_DATE_TIME_LEAP] = { CMD_DATE_FN, DF_LEAP },
    [MID_OFFSET_DATE_TIME_TO_DAY_OF_WEEK] = { CMD_DATE_FN, DF_DOW },
    [MID_OFFSET_DATE_TIME_CONVERT_TO_TIME_ZONE] = { CMD_DATE_FN, DF_CVTZ },
    [MID_OFFSET_DATE_TIME_EXTRACT_DATE] = { CMD_DATE_FN, DF_GET_DATE },
    [MID_OFFSET_DATE_TIME_EXTRACT_TIME] = { CMD_DATE_FN, DF_GET_TIME },
    [MID_OFFSET_DATE_TIME_EXTRACT_DATE_TIME] = { CMD_DATE_FN, DF_GET_DT },
    [MID_OFFSET_DATE_TIME_EXTRACT_OFFSET] = { CMD_DATE_FN, DF_GET_TZ },
    [MID_OFFSET_DATE_TIME_SHRINK_TO_DATE_TIME] = { CMD_DATE_FN, DF_SHRINK },
    [MID_OFFSET_DATE_TIME_EXTEND_TO_NOTHING] = { CMD_DATE_FN, DF_EXTEND },
    [MID_OFFSET_DATE_TIME_CUT_TO_DATE_TIME] = { CMD_DATE_FN, DF_CUT },
    [MID_OFFSET_DATE_TIME_LINK_TO_NOTHING] = { CMD_DATE_FN, DF_LINK },
    [MID_DAYS_TO_DURATION] = { CMD_DATE_FN, DF_DAYS_TO_DR },
    [MID_HOURS_TO_DURATION] = { CMD_DATE_FN, DF_HOURS_TO_DR },
    [MID_MINUTES_TO_DURATION] = { CMD_DATE_FN, DF_MINUTES_TO_DR },
    [MID_SECONDS_TO_DURATION] = { CMD_DATE_FN, DF_SECONDS_TO_DR },
    [MID_DURATION_TO_SECONDS] = { CMD_DATE_FN, DF_DR_TO_S },
    [MID_DURATION_EXTRACT_DAY] = { CMD_DATE_FN, DF_DAY },
    [MID_DURATION_EXTRACT_HOUR] = { CMD_DATE_FN, DF_HOUR },
    [MID_DURATION_EXTRACT_MINUTE] = { CMD_DATE_FN, DF_MINUTE },
    [MID_DURATION_EXTRACT_SECOND] = { CMD_DATE_FN, DF_SECOND },
    [MID_DAY_OF_WEEK_TO_ISO_DOW_NUMBER] = { CMD_DATE_FN, DF_DW_TO_ISO },
    [MID_ISO_DOW_NUMBER_TO_DAY_OF_WEEK] = { CMD_DATE_FN, DF_ISO_TO_DW },
    [MID_DAY_OF_WEEK_TO_UNIX_DOW_NUMBER] = { CMD_DATE_FN, DF_DW_TO_UNIX },
    [MID_UNIX_DOW_NUMBER_TO_DAY_OF_WEEK] = { CMD_DATE_FN, DF_UNIX_TO_DW },
    [MID_EPOCH_UNIX] = { CMD_DATE_FN, DF_EPOCH_UNIX },
    [MID_EPOCH_NTP] = { CMD_DATE_FN, DF_EPOCH_NTP },
    [MID_EPOCH_GPS] = { CMD_DATE_FN, DF_EPOCH_GPS },
    [MID_EPOCH_TIOS] = { CMD_DATE_FN, DF_EPOCH_TIOS },
    [MID_EPOCH_Y2K] = { CMD_DATE_FN, DF_EPOCH_Y2K },
    [MID_EPOCH_CUSTOM] = { CMD_DATE_FN, DF_EPOCH_CUSTOM },
    [MID_EPOCH_SET_CUSTOM] = { CMD_DATE_FN, DF_EPOCH_SET_CUSTOM },
    [MID_EPOCH_GET_CUSTOM] = { CMD_DATE_FN, DF_EPOCH_GET_CUSTOM },
    [MID_GET_NOW] = { CMD_DATE_FN, DF_NOW },
    [MID_GET_NOW_DATE] = { CMD_DATE_FN, DF_NOW_DATE },
    [MID_GET_NOW_TIME] = { CMD_DATE_FN, DF_NOW_TIME },
    [MID_GET_NOW_OFFSET_DATE_TIME] = { CMD_DATE_FN, DF_NOW_DZ },
    [MID_GET_NOW_OFFSET_DATE_TIME_UTC] = { CMD_DATE_FN, DF_NOW_DZ_UTC },
    [MID_SET_APP_TIME_ZONE] = { CMD_DATE_FN, DF_SET_APP_TZ },
    [MID_GET_APP_TIME_ZONE] = { CMD_DATE_FN, DF_GET_APP_TZ },
    [MID_SET_CLOCK_TIME_ZONE] = { CMD_DATE_FN, DF_SET_CLOCK_TZ },
    [MID_GET_CLOCK_TIME_ZONE] = { CMD_DATE_FN, DF_GET_CLOCK_TZ },
    [MID_SET_CLOCK] = { CMD_DATE_FN, DF_SET_CLOCK },
    [MID_FORMAT_RECORD_RAW] = { CMD_DATE_FN, DF_FORMAT_RAW },
    [MID_FORMAT_RECORD_STRING] = { CMD_DATE_FN, DF_FORMAT_STRING },
    /* DATE record entry */
    [MID_DATE_CREATE] = { CMD_RECORD_TAG, OBJ_DATE },
    [MID_TIME_CREATE] = { CMD_RECORD_TAG, OBJ_TIME },
    [MID_DATE_TIME_CREATE] = { CMD_RECORD_TAG, OBJ_DATETIME },
    [MID_OFFSET_CREATE] = { CMD_RECORD_TAG, OBJ_TZ },
    [MID_OFFSET_DATE_TIME_CREATE] = { CMD_RECORD_TAG, OBJ_DZ },
    [MID_DURATION_CREATE] = { CMD_RECORD_TAG, OBJ_DR },
    [MID_DAY_OF_WEEK_CREATE] = { CMD_RECORD_TAG, OBJ_DW },
    /* BASE operations */
    [MID_BITWISE_AND] = { CMD_BASE_OP, BOP_AND },
    [MID_BITWISE_OR] = { CMD_BASE_OP, BOP_OR },
    [MID_BITWISE_XOR] = { CMD_BASE_OP, BOP_XOR },
    [MID_BITWISE_NOT] = { CMD_BASE_OP, BOP_NOT },
    [MID_BITWISE_NEG] = { CMD_BASE_OP, BOP_NEG },
    [MID_SHIFT_LEFT_LOGICAL] = { CMD_BASE_OP, BOP_SL },
    [MID_SHIFT_RIGHT_LOGICAL] = { CMD_BASE_OP, BOP_SR },
    [MID_SHIFT_RIGHT_ARITHMETIC] = { CMD_BASE_OP, BOP_ASR },
    [MID_SHIFT_LEFT_LOGICAL_N] = { CMD_BASE_OP, BOP_SLN },
    [MID_SHIFT_RIGHT_LOGICAL_N] = { CMD_BASE_OP, BOP_SRN },
    [MID_ROTATE_LEFT_CIRCULAR] = { CMD_BASE_OP, BOP_RL },
    [MID_ROTATE_RIGHT_CIRCULAR] = { CMD_BASE_OP, BOP_RR },
    [MID_ROTATE_LEFT_CARRY] = { CMD_BASE_OP, BOP_RLC },
    [MID_ROTATE_RIGHT_CARRY] = { CMD_BASE_OP, BOP_RRC },
    [MID_ROTATE_LEFT_CIRCULAR_N] = { CMD_BASE_OP, BOP_RLN },
    [MID_ROTATE_RIGHT_CIRCULAR_N] = { CMD_BASE_OP, BOP_RRN },
    [MID_ROTATE_LEFT_CARRY_N] = { CMD_BASE_OP, BOP_RLCN },
    [MID_ROTATE_RIGHT_CARRY_N] = { CMD_BASE_OP, BOP_RRCN },
    [MID_CLEAR_BIT] = { CMD_BASE_OP, BOP_CB },
    [MID_SET_BIT] = { CMD_BASE_OP, BOP_SB },
    [MID_GET_BIT] = { CMD_BASE_OP, BOP_GETB },
    [MID_REVERSE_BIT] = { CMD_BASE_OP, BOP_REVB },
    [MID_COUNT_BIT] = { CMD_BASE_OP, BOP_CNTB },
    [MID_BASE_ADD] = { CMD_BASE_OP, BOP_ADD },
    [MID_BASE_SUBT] = { CMD_BASE_OP, BOP_SUB },
    [MID_BASE_MULT] = { CMD_BASE_OP, BOP_MUL },
    [MID_BASE_DIV] = { CMD_BASE_OP, BOP_DIV },
    [MID_BASE_DIV2] = { CMD_BASE_OP, BOP_DIV2 },
    /* Commands without arguments */
    [MID_PRIME] = { CMD_PRIME, 0 },
    [MID_ROUND_TO_FIX] = { CMD_ROUND_FIX, 0 },
    [MID_SINH] = { CMD_SINH, 0 },
    [MID_COSH] = { CMD_COSH, 0 },
    [MID_TANH] = { CMD_TANH, 0 },
    [MID_ASINH] = { CMD_ASINH, 0 },
    [MID_ACOSH] = { CMD_ACOSH, 0 },
    [MID_ATANH] = { CMD_ATANH, 0 },
    [MID_POLAR_TO_RECT] = { CMD_TO_REC, 0 },
    [MID_RECT_TO_POLAR] = { CMD_TO_POL, 0 },
    [MID_HMS_TO_HR] = { CMD_TO_HOURS, 0 },
    [MID_HR_TO_HMS] = { CMD_TO_HMS, 0 },
    [MID_HMS_PLUS] = { CMD_HMS_ADD, 0 },
    [MID_HMS_MINUS] = { CMD_HMS_SUB, 0 },
    [MID_CLEAR_CARRY_FLAG] = { CMD_CLEAR_CARRY, 0 },
    [MID_SET_CARRY_FLAG] = { CMD_SET_CARRY, 0 },
    [MID_GET_CARRY_FLAG] = { CMD_GET_CARRY, 0 },
    [MID_SET_WORD_SIZE] = { CMD_SET_WORD_SIZE, 0 },
    [MID_GET_WORD_SIZE] = { CMD_GET_WORD_SIZE, 0 },
    [MID_STAT_PLUS] = { CMD_STAT_PLUS, 0 },
    [MID_STAT_MINUS] = { CMD_STAT_MINUS, 0 },
    [MID_STAT_ALL_MODE] = { CMD_STAT_ALL_MODE, 0 },
    [MID_STAT_LINEAR_MODE] = { CMD_STAT_LIN_MODE, 0 },
    [MID_STAT_CLEAR] = { CMD_STAT_CLEAR, 0 },
    [MID_STAT_SUM] = { CMD_STAT_SUM, 0 },
    [MID_STAT_MEAN] = { CMD_STAT_MEAN, 0 },
    [MID_STAT_WEIGHTED_MEAN] = { CMD_STAT_WMEAN, 0 },
    [MID_STAT_N] = { CMD_STAT_N, 0 },
    [MID_STAT_SAMPLE_SDEV] = { CMD_STAT_SDEV, 0 },
    [MID_STAT_SAMPLE_COV] = { CMD_STAT_SCOV, 0 },
    [MID_STAT_POP_SDEV] = { CMD_STAT_PDEV, 0 },
    [MID_STAT_POP_COV] = { CMD_STAT_PCOV, 0 },
    [MID_CFIT_FORECAST_X] = { CMD_FIT_X, 0 },
    [MID_CFIT_FORECAST_Y] = { CMD_FIT_Y, 0 },
    [MID_CFIT_SLOPE] = { CMD_FIT_SLOPE, 0 },
    [MID_CFIT_INTERCEPT] = { CMD_FIT_INTERCEPT, 0 },
    [MID_CFIT_CORRELATION] = { CMD_FIT_CORR, 0 },
    [MID_CFIT_BEST] = { CMD_FIT_BEST, 0 },
    [MID_UNIT_VALUE] = { CMD_UNIT_VALUE, 0 },
    [MID_UNIT_BASE] = { CMD_UNIT_BASE, 0 },
    [MID_TVM_BEGIN] = { CMD_TVM_BEGIN, 0 },
    [MID_TVM_END] = { CMD_TVM_END, 0 },
    [MID_TVM_CLEAR] = { CMD_TVM_CLEAR, 0 },
    [MID_TVM_SOLVER_RESET] = { CMD_TVM_RESET, 0 },
    [MID_CUBE] = { CMD_CUBE, 0 },
    [MID_CUBE_ROOT] = { CMD_CUBE_ROOT, 0 },
    [MID_XROOT_Y] = { CMD_XROOT_Y, 0 },
    [MID_TWO_POW] = { CMD_TWO_POW, 0 },
    [MID_LOG2] = { CMD_LOG2, 0 },
    [MID_LOG_BASE] = { CMD_LOG_BASE, 0 },
    [MID_ATAN2] = { CMD_ATAN2, 0 },
    [MID_EXP_MINUS_ONE] = { CMD_EXP_MINUS_ONE, 0 },
    [MID_LN_ONE_PLUS] = { CMD_LN_ONE_PLUS, 0 },
    [MID_PERCENT] = { CMD_PERCENT, 0 },
    [MID_PERCENT_CHANGE] = { CMD_PERCENT_CHANGE, 0 },
    [MID_GCD] = { CMD_GCD, 0 },
    [MID_LCM] = { CMD_LCM, 0 },
    [MID_INT_PART] = { CMD_INT_PART, 0 },
    [MID_FRAC_PART] = { CMD_FRAC_PART, 0 },
    [MID_FLOOR] = { CMD_FLOOR, 0 },
    [MID_CEIL] = { CMD_CEIL, 0 },
    [MID_NEAR] = { CMD_NEAR, 0 },
    [MID_ABS] = { CMD_ABS, 0 },
    [MID_SIGN] = { CMD_SIGN, 0 },
    [MID_MOD] = { CMD_MOD, 0 },
    [MID_MIN] = { CMD_MIN, 0 },
    [MID_MAX] = { CMD_MAX, 0 },
    [MID_COMB] = { CMD_COMB, 0 },
    [MID_PERM] = { CMD_PERM, 0 },
    [MID_FACTORIAL] = { CMD_FACTORIAL, 0 },
    [MID_RANDOM] = { CMD_RAND, 0 },
    [MID_RANDOM_SEED] = { CMD_SEED, 0 },
    [MID_RAD_TO_DEG] = { CMD_TO_DEG, 0 },
    [MID_DEG_TO_RAD] = { CMD_TO_RAD, 0 },
    [MID_CLEAR_X] = { CMD_CLEAR, 0 },
    [MID_CLEAR_REGS] = { CMD_CLEAR_REGS, 0 },
    [MID_CLEAR_STAT] = { CMD_STAT_CLEAR, 0 },
    [MID_CLEAR_TVM] = { CMD_TVM_CLEAR, 0 },
    [MID_SET_REG_SIZE] = { CMD_SET_REG_SIZE, 0 },
    [MID_GET_REG_SIZE] = { CMD_GET_REG_SIZE, 0 },
    [MID_SET_STACK_SIZE] = { CMD_SET_STACK_SIZE, 0 },
    [MID_GET_STACK_SIZE] = { CMD_GET_STACK_SIZE, 0 },
    [MID_CLEAR_STACK] = { CMD_CLEAR_STACK, 0 },
    [MID_RAD] = { CMD_MODE_RAD, 0 },
    [MID_DEG] = { CMD_MODE_DEG, 0 },
    [MID_STACK_DUP] = { CMD_ENTER, 0 },
    [MID_STACK_ROLL_UP] = { CMD_ROLL_UP, 0 },
    [MID_STACK_ROLL_DOWN] = { CMD_ROLL_DOWN, 0 },
    [MID_STACK_DROP] = { CMD_DROP, 0 },
    [MID_STACK_EXCHANGE_XY] = { CMD_SWAP, 0 },
};

calc_cmd_t action_for(menu_id_t id, bool shifted, uint8_t *arg)
{
    *arg = 0;
    int u = unit_for_menu(id);
    if (u >= 0) {
        *arg = (uint8_t)u;
        return CMD_UNIT;
    }
    if ((unsigned)id >= sizeof actions / sizeof actions[0])
        return CMD_NONE;

    calc_cmd_t cmd = (calc_cmd_t)actions[id].cmd;
    *arg = actions[id].arg;
    switch (cmd) {
        case CMD_TVM_VAR: case CMD_TVM_IYR1: case CMD_TVM_IYR2: case CMD_TVM_ITER_MAX:
            if (shifted)
                *arg |= TVM_RECALL;
            break;
        default:
            break;
    }
    return cmd;
}

bool action_is_active(const calc_t *c, menu_id_t id)
{
    switch (id) {
        case MID_RAD: return !c->degrees;
        case MID_DEG: return c->degrees;
        case MID_FORMAT_RECORD_RAW:    return !c->date_as_string;
        case MID_FORMAT_RECORD_STRING: return c->date_as_string;
        case MID_EPOCH_UNIX:   return c->epoch == EPOCH_UNIX;
        case MID_EPOCH_NTP:    return c->epoch == EPOCH_NTP;
        case MID_EPOCH_GPS:    return c->epoch == EPOCH_GPS;
        case MID_EPOCH_TIOS:   return c->epoch == EPOCH_TIOS;
        case MID_EPOCH_Y2K:    return c->epoch == EPOCH_Y2K;
        case MID_EPOCH_CUSTOM: return c->epoch == EPOCH_CUSTOM;
        case MID_DEC: return c->base.radix == 10;
        case MID_HEX: return c->base.radix == 16;
        case MID_OCT: return c->base.radix == 8;
        case MID_BIN: return c->base.radix == 2;
        case MID_STAT_ALL_MODE:    return !c->stats.linear_only;
        case MID_STAT_LINEAR_MODE: return c->stats.linear_only;
        case MID_CFIT_LINEAR: return c->stats.model == FIT_LIN;
        case MID_CFIT_LOG:    return c->stats.model == FIT_LOG;
        case MID_CFIT_EXP:    return c->stats.model == FIT_EXP;
        case MID_CFIT_POWER:  return c->stats.model == FIT_PWR;
        case MID_TVM_BEGIN: return c->tvm.begin;
        case MID_TVM_END:   return !c->tvm.begin;
        case MID_TVM_IYR0:  return c->tvm.iyr1_set;
        case MID_TVM_IYR1:  return c->tvm.iyr2_set;
        case MID_TVM_ITER_MAX: return c->tvm.iter_max_set;
        case MID_FIX: return c->disp_style == DISP_FIX && c->disp_digits != DISP_FLOAT;
        case MID_SCI: return c->disp_style == DISP_SCI;
        case MID_ENG: return c->disp_style == DISP_ENG;
        default:      return false;
    }
}
