#include "actions.h"
#include "res.h"
#include "../core/format.h"

static calc_cmd_t simple_action(menu_id_t id);

/* The DATE function for a DATE menu item (date_fn_t), or -1. */
static int date_fn_for(menu_id_t id)
{
    switch (id) {
        case MID_DATE_TO_EPOCH_DAYS: return DF_TO_ED;
        case MID_EPOCH_DAYS_TO_DATE: return DF_ED_TO_D;
        case MID_DATE_TO_EPOCH_SECONDS: return DF_TO_ES;
        case MID_EPOCH_SECONDS_TO_DATE: return DF_ES_TO_D;
        case MID_IS_DATE_LEAP: return DF_LEAP;
        case MID_DATE_TO_DAY_OF_WEEK: return DF_DOW;
        case MID_DATE_CONVERT_TO_TIME_ZONE: return DF_CVTZ;
        case MID_DATE_EXTRACT_YEAR: return DF_YEAR;
        case MID_DATE_EXTRACT_MONTH: return DF_MONTH;
        case MID_DATE_EXTRACT_DAY: return DF_DAY;
        case MID_DATE_SHRINK_TO_NOTHING: return DF_SHRINK;
        case MID_DATE_EXTEND_TO_DATE_TIME: return DF_EXTEND;
        case MID_DATE_CUT_TO_NOTHING: return DF_CUT;
        case MID_DATE_LINK_TO_DATE_TIME: return DF_LINK;
        case MID_TIME_TO_SECONDS: return DF_TIME_TO_S;
        case MID_SECONDS_TO_TIME: return DF_S_TO_TIME;
        case MID_TIME_EXTRACT_HOUR: return DF_HOUR;
        case MID_TIME_EXTRACT_MINUTE: return DF_MINUTE;
        case MID_TIME_EXTRACT_SECOND: return DF_SECOND;
        case MID_DATE_TIME_TO_EPOCH_DAYS: return DF_TO_ED;
        case MID_EPOCH_DAYS_TO_DATE_TIME: return DF_ED_TO_DT;
        case MID_DATE_TIME_TO_EPOCH_SECONDS: return DF_TO_ES;
        case MID_EPOCH_SECONDS_TO_DATE_TIME: return DF_ES_TO_DT;
        case MID_IS_DATE_TIME_LEAP: return DF_LEAP;
        case MID_DATE_TIME_TO_DAY_OF_WEEK: return DF_DOW;
        case MID_DATE_TIME_CONVERT_TO_TIME_ZONE: return DF_CVTZ;
        case MID_DATE_TIME_EXTRACT_DATE: return DF_GET_DATE;
        case MID_DATE_TIME_EXTRACT_TIME: return DF_GET_TIME;
        case MID_DATE_TIME_SHRINK_TO_DATE: return DF_SHRINK;
        case MID_DATE_TIME_EXTEND_TO_OFFSET_DATE_TIME: return DF_EXTEND;
        case MID_DATE_TIME_CUT_TO_DATE: return DF_CUT;
        case MID_DATE_TIME_LINK_TO_OFFSET_DATE_TIME: return DF_LINK;
        case MID_OFFSET_TO_HOURS: return DF_TZ_TO_HR;
        case MID_HOURS_TO_OFFSET: return DF_HR_TO_TZ;
        case MID_OFFSET_EXTRACT_HOUR: return DF_TZ_HOUR;
        case MID_OFFSET_EXTRACT_MINUTE: return DF_TZ_MINUTE;
        case MID_OFFSET_DATE_TIME_TO_EPOCH_DAYS: return DF_TO_ED;
        case MID_EPOCH_DAYS_TO_OFFSET_DATE_TIME: return DF_ED_TO_DZ;
        case MID_OFFSET_DATE_TIME_TO_EPOCH_SECONDS: return DF_TO_ES;
        case MID_EPOCH_SECONDS_TO_OFFSET_DATE_TIME_UTC: return DF_ES_TO_DZ;
        case MID_EPOCH_SECONDS_TO_OFFSET_DATE_TIME_APP: return DF_ES_TO_DZ_APP;
        case MID_IS_OFFSET_DATE_TIME_LEAP: return DF_LEAP;
        case MID_OFFSET_DATE_TIME_TO_DAY_OF_WEEK: return DF_DOW;
        case MID_OFFSET_DATE_TIME_CONVERT_TO_TIME_ZONE: return DF_CVTZ;
        case MID_OFFSET_DATE_TIME_EXTRACT_DATE: return DF_GET_DATE;
        case MID_OFFSET_DATE_TIME_EXTRACT_TIME: return DF_GET_TIME;
        case MID_OFFSET_DATE_TIME_EXTRACT_DATE_TIME: return DF_GET_DT;
        case MID_OFFSET_DATE_TIME_EXTRACT_OFFSET: return DF_GET_TZ;
        case MID_OFFSET_DATE_TIME_SHRINK_TO_DATE_TIME: return DF_SHRINK;
        case MID_OFFSET_DATE_TIME_EXTEND_TO_NOTHING: return DF_EXTEND;
        case MID_OFFSET_DATE_TIME_CUT_TO_DATE_TIME: return DF_CUT;
        case MID_OFFSET_DATE_TIME_LINK_TO_NOTHING: return DF_LINK;
        case MID_DAYS_TO_DURATION: return DF_DAYS_TO_DR;
        case MID_HOURS_TO_DURATION: return DF_HOURS_TO_DR;
        case MID_MINUTES_TO_DURATION: return DF_MINUTES_TO_DR;
        case MID_SECONDS_TO_DURATION: return DF_SECONDS_TO_DR;
        case MID_DURATION_TO_SECONDS: return DF_DR_TO_S;
        case MID_DURATION_EXTRACT_DAY: return DF_DAY;
        case MID_DURATION_EXTRACT_HOUR: return DF_HOUR;
        case MID_DURATION_EXTRACT_MINUTE: return DF_MINUTE;
        case MID_DURATION_EXTRACT_SECOND: return DF_SECOND;
        case MID_DAY_OF_WEEK_TO_ISO_DOW_NUMBER: return DF_DW_TO_ISO;
        case MID_ISO_DOW_NUMBER_TO_DAY_OF_WEEK: return DF_ISO_TO_DW;
        case MID_DAY_OF_WEEK_TO_UNIX_DOW_NUMBER: return DF_DW_TO_UNIX;
        case MID_UNIX_DOW_NUMBER_TO_DAY_OF_WEEK: return DF_UNIX_TO_DW;
        case MID_EPOCH_UNIX: return DF_EPOCH_UNIX;
        case MID_EPOCH_NTP: return DF_EPOCH_NTP;
        case MID_EPOCH_GPS: return DF_EPOCH_GPS;
        case MID_EPOCH_TIOS: return DF_EPOCH_TIOS;
        case MID_EPOCH_Y2K: return DF_EPOCH_Y2K;
        case MID_EPOCH_CUSTOM: return DF_EPOCH_CUSTOM;
        case MID_EPOCH_SET_CUSTOM: return DF_EPOCH_SET_CUSTOM;
        case MID_EPOCH_GET_CUSTOM: return DF_EPOCH_GET_CUSTOM;
        case MID_GET_NOW: return DF_NOW;
        case MID_GET_NOW_DATE: return DF_NOW_DATE;
        case MID_GET_NOW_TIME: return DF_NOW_TIME;
        case MID_GET_NOW_OFFSET_DATE_TIME: return DF_NOW_DZ;
        case MID_GET_NOW_OFFSET_DATE_TIME_UTC: return DF_NOW_DZ_UTC;
        case MID_SET_APP_TIME_ZONE: return DF_SET_APP_TZ;
        case MID_GET_APP_TIME_ZONE: return DF_GET_APP_TZ;
        case MID_SET_CLOCK_TIME_ZONE: return DF_SET_CLOCK_TZ;
        case MID_GET_CLOCK_TIME_ZONE: return DF_GET_CLOCK_TZ;
        case MID_SET_CLOCK: return DF_SET_CLOCK;
        case MID_FORMAT_RECORD_RAW: return DF_FORMAT_RAW;
        case MID_FORMAT_RECORD_STRING: return DF_FORMAT_STRING;
        default: return -1;
    }
}

/* The record type a DATE menu's first key starts (D{}, T{}, ...), or -1. */
static int record_tag_for(menu_id_t id)
{
    switch (id) {
        case MID_DATE_CREATE: return OBJ_DATE;
        case MID_TIME_CREATE: return OBJ_TIME;
        case MID_DATE_TIME_CREATE: return OBJ_DATETIME;
        case MID_OFFSET_CREATE: return OBJ_TZ;
        case MID_OFFSET_DATE_TIME_CREATE: return OBJ_DZ;
        case MID_DURATION_CREATE: return OBJ_DR;
        case MID_DAY_OF_WEEK_CREATE: return OBJ_DW;
        default: return -1;
    }
}


/* The base_op_t for a BASE menu item, or -1. */
static int base_op_for(menu_id_t id)
{
    switch (id) {
        case MID_BITWISE_AND:            return BOP_AND;
        case MID_BITWISE_OR:             return BOP_OR;
        case MID_BITWISE_XOR:            return BOP_XOR;
        case MID_BITWISE_NOT:            return BOP_NOT;
        case MID_BITWISE_NEG:            return BOP_NEG;
        case MID_SHIFT_LEFT_LOGICAL:     return BOP_SL;
        case MID_SHIFT_RIGHT_LOGICAL:    return BOP_SR;
        case MID_SHIFT_RIGHT_ARITHMETIC: return BOP_ASR;
        case MID_SHIFT_LEFT_LOGICAL_N:   return BOP_SLN;
        case MID_SHIFT_RIGHT_LOGICAL_N:  return BOP_SRN;
        case MID_ROTATE_LEFT_CIRCULAR:   return BOP_RL;
        case MID_ROTATE_RIGHT_CIRCULAR:  return BOP_RR;
        case MID_ROTATE_LEFT_CARRY:      return BOP_RLC;
        case MID_ROTATE_RIGHT_CARRY:     return BOP_RRC;
        case MID_ROTATE_LEFT_CIRCULAR_N: return BOP_RLN;
        case MID_ROTATE_RIGHT_CIRCULAR_N: return BOP_RRN;
        case MID_ROTATE_LEFT_CARRY_N:    return BOP_RLCN;
        case MID_ROTATE_RIGHT_CARRY_N:   return BOP_RRCN;
        case MID_CLEAR_BIT:              return BOP_CB;
        case MID_SET_BIT:                return BOP_SB;
        case MID_GET_BIT:                return BOP_GETB;
        case MID_REVERSE_BIT:            return BOP_REVB;
        case MID_COUNT_BIT:              return BOP_CNTB;
        case MID_BASE_ADD:               return BOP_ADD;
        case MID_BASE_SUBT:              return BOP_SUB;
        case MID_BASE_MULT:              return BOP_MUL;
        case MID_BASE_DIV:               return BOP_DIV;
        case MID_BASE_DIV2:              return BOP_DIV2;
        default:                         return -1;
    }
}

calc_cmd_t action_for(menu_id_t id, bool shifted, uint8_t *arg)
{
    uint8_t recall = shifted ? TVM_RECALL : 0;
    *arg = 0;

    int u = unit_for_menu(id);
    if (u >= 0) {
        *arg = (uint8_t)u;
        return CMD_UNIT;
    }

    switch (id) {
        case MID_TVM_N:    *arg = TVM_N | recall;   return CMD_TVM_VAR;
        case MID_TVM_IYR:  *arg = TVM_IYR | recall; return CMD_TVM_VAR;
        case MID_TVM_PV:   *arg = TVM_PV | recall;  return CMD_TVM_VAR;
        case MID_TVM_PMT:  *arg = TVM_PMT | recall; return CMD_TVM_VAR;
        case MID_TVM_FV:   *arg = TVM_FV | recall;  return CMD_TVM_VAR;
        case MID_TVM_PYR:  *arg = TVM_PYR | recall; return CMD_TVM_VAR;
        case MID_TVM_CYR:  *arg = TVM_CYR | recall; return CMD_TVM_VAR;
        case MID_TVM_IYR0: *arg = recall; return CMD_TVM_IYR1;
        case MID_TVM_IYR1: *arg = recall; return CMD_TVM_IYR2;
        case MID_TVM_ITER_MAX: *arg = recall; return CMD_TVM_ITER_MAX;
        /* STAT > Sigma: recall a sum */
        case MID_STAT_REG_N:    *arg = SUM_N;    return CMD_STAT_REG;
        case MID_STAT_REG_X:    *arg = SUM_X;    return CMD_STAT_REG;
        case MID_STAT_REG_X2:   *arg = SUM_X2;   return CMD_STAT_REG;
        case MID_STAT_REG_Y:    *arg = SUM_Y;    return CMD_STAT_REG;
        case MID_STAT_REG_Y2:   *arg = SUM_Y2;   return CMD_STAT_REG;
        case MID_STAT_REG_XY:   *arg = SUM_XY;   return CMD_STAT_REG;
        case MID_STAT_REG_LN_X: *arg = SUM_LX;   return CMD_STAT_REG;
        case MID_STAT_REG_LN_X2: *arg = SUM_LX2; return CMD_STAT_REG;
        case MID_STAT_REG_LN_Y: *arg = SUM_LY;   return CMD_STAT_REG;
        case MID_STAT_REG_LN_Y2: *arg = SUM_LY2; return CMD_STAT_REG;
        case MID_STAT_REG_LN_XLN_Y: *arg = SUM_LXLY; return CMD_STAT_REG;
        case MID_STAT_REG_XLN_Y: *arg = SUM_XLY; return CMD_STAT_REG;
        case MID_STAT_REG_YLN_X: *arg = SUM_YLX; return CMD_STAT_REG;
        /* CFIT models */
        case MID_CFIT_LINEAR:   *arg = FIT_LIN;  return CMD_FIT_MODEL;
        case MID_CFIT_LOG:      *arg = FIT_LOG;  return CMD_FIT_MODEL;
        case MID_CFIT_EXP:      *arg = FIT_EXP;  return CMD_FIT_MODEL;
        case MID_CFIT_POWER:    *arg = FIT_PWR;  return CMD_FIT_MODEL;
        /* BASE: radix and operations */
        case MID_DEC: *arg = 10; return CMD_BASE_RADIX;
        case MID_HEX: *arg = 16; return CMD_BASE_RADIX;
        case MID_OCT: *arg = 8;  return CMD_BASE_RADIX;
        case MID_BIN: *arg = 2;  return CMD_BASE_RADIX;
        default:           break;
    }

    int df = date_fn_for(id);
    if (df >= 0) {
        *arg = (uint8_t)df;
        return CMD_DATE_FN;
    }
    int tag = record_tag_for(id);
    if (tag >= 0) {
        *arg = (uint8_t)tag;
        return CMD_RECORD_TAG;
    }

    int op = base_op_for(id);
    if (op >= 0) {
        *arg = (uint8_t)op;
        return CMD_BASE_OP;
    }
    return simple_action(id);
}

static calc_cmd_t simple_action(menu_id_t id)
{
    switch (id) {
        /* NUM */
        case MID_PRIME:           return CMD_PRIME;
        case MID_ROUND_TO_FIX:    return CMD_ROUND_FIX;
        /* HYP */
        case MID_SINH:            return CMD_SINH;
        case MID_COSH:            return CMD_COSH;
        case MID_TANH:            return CMD_TANH;
        case MID_ASINH:           return CMD_ASINH;
        case MID_ACOSH:           return CMD_ACOSH;
        case MID_ATANH:           return CMD_ATANH;
        /* CONV */
        case MID_POLAR_TO_RECT:   return CMD_TO_REC;
        case MID_RECT_TO_POLAR:   return CMD_TO_POL;
        case MID_HMS_TO_HR:       return CMD_TO_HOURS;
        case MID_HR_TO_HMS:       return CMD_TO_HMS;
        case MID_HMS_PLUS:        return CMD_HMS_ADD;
        case MID_HMS_MINUS:       return CMD_HMS_SUB;
        /* BASE settings */
        case MID_CLEAR_CARRY_FLAG: return CMD_CLEAR_CARRY;
        case MID_SET_CARRY_FLAG:  return CMD_SET_CARRY;
        case MID_GET_CARRY_FLAG:  return CMD_GET_CARRY;
        case MID_SET_WORD_SIZE:   return CMD_SET_WORD_SIZE;
        case MID_GET_WORD_SIZE:   return CMD_GET_WORD_SIZE;
        /* STAT */
        case MID_STAT_PLUS:       return CMD_STAT_PLUS;
        case MID_STAT_MINUS:      return CMD_STAT_MINUS;
        case MID_STAT_ALL_MODE:   return CMD_STAT_ALL_MODE;
        case MID_STAT_LINEAR_MODE: return CMD_STAT_LIN_MODE;
        case MID_STAT_CLEAR:      return CMD_STAT_CLEAR;
        case MID_STAT_SUM:        return CMD_STAT_SUM;
        case MID_STAT_MEAN:       return CMD_STAT_MEAN;
        case MID_STAT_WEIGHTED_MEAN: return CMD_STAT_WMEAN;
        case MID_STAT_N:          return CMD_STAT_N;
        case MID_STAT_SAMPLE_SDEV: return CMD_STAT_SDEV;
        case MID_STAT_SAMPLE_COV: return CMD_STAT_SCOV;
        case MID_STAT_POP_SDEV:   return CMD_STAT_PDEV;
        case MID_STAT_POP_COV:    return CMD_STAT_PCOV;
        case MID_CFIT_FORECAST_X: return CMD_FIT_X;
        case MID_CFIT_FORECAST_Y: return CMD_FIT_Y;
        case MID_CFIT_SLOPE:      return CMD_FIT_SLOPE;
        case MID_CFIT_INTERCEPT:  return CMD_FIT_INTERCEPT;
        case MID_CFIT_CORRELATION: return CMD_FIT_CORR;
        case MID_CFIT_BEST:       return CMD_FIT_BEST;
        /* UNIT */
        case MID_UNIT_VALUE:      return CMD_UNIT_VALUE;
        case MID_UNIT_BASE:       return CMD_UNIT_BASE;
        /* TVM */
        case MID_TVM_BEGIN:       return CMD_TVM_BEGIN;
        case MID_TVM_END:         return CMD_TVM_END;
        case MID_TVM_CLEAR:       return CMD_TVM_CLEAR;
        case MID_TVM_SOLVER_RESET: return CMD_TVM_RESET;
        /* MATH */
        case MID_CUBE:            return CMD_CUBE;
        case MID_CUBE_ROOT:       return CMD_CUBE_ROOT;
        case MID_XROOT_Y:         return CMD_XROOT_Y;
        case MID_TWO_POW:         return CMD_TWO_POW;
        case MID_LOG2:            return CMD_LOG2;
        case MID_LOG_BASE:        return CMD_LOG_BASE;
        case MID_ATAN2:           return CMD_ATAN2;
        case MID_EXP_MINUS_ONE:   return CMD_EXP_MINUS_ONE;
        case MID_LN_ONE_PLUS:     return CMD_LN_ONE_PLUS;
        /* NUM */
        case MID_PERCENT:         return CMD_PERCENT;
        case MID_PERCENT_CHANGE:  return CMD_PERCENT_CHANGE;
        case MID_GCD:             return CMD_GCD;
        case MID_LCM:             return CMD_LCM;
        case MID_INT_PART:        return CMD_INT_PART;
        case MID_FRAC_PART:       return CMD_FRAC_PART;
        case MID_FLOOR:           return CMD_FLOOR;
        case MID_CEIL:            return CMD_CEIL;
        case MID_NEAR:            return CMD_NEAR;
        case MID_ABS:             return CMD_ABS;
        case MID_SIGN:            return CMD_SIGN;
        case MID_MOD:             return CMD_MOD;
        case MID_MIN:             return CMD_MIN;
        case MID_MAX:             return CMD_MAX;
        /* PROB */
        case MID_COMB:            return CMD_COMB;
        case MID_PERM:            return CMD_PERM;
        case MID_FACTORIAL:       return CMD_FACTORIAL;
        case MID_RANDOM:          return CMD_RAND;
        case MID_RANDOM_SEED:     return CMD_SEED;
        /* CONV */
        case MID_RAD_TO_DEG:       return CMD_TO_DEG;
        case MID_DEG_TO_RAD:       return CMD_TO_RAD;
        /* CLR */
        case MID_CLEAR_X:         return CMD_CLEAR;
        case MID_CLEAR_REGS:      return CMD_CLEAR_REGS;
        case MID_CLEAR_STAT:      return CMD_STAT_CLEAR;
        case MID_CLEAR_TVM:       return CMD_TVM_CLEAR;
        case MID_SET_REG_SIZE:    return CMD_SET_REG_SIZE;
        case MID_GET_REG_SIZE:    return CMD_GET_REG_SIZE;
        case MID_SET_STACK_SIZE:  return CMD_SET_STACK_SIZE;
        case MID_GET_STACK_SIZE:  return CMD_GET_STACK_SIZE;
        case MID_CLEAR_STACK:     return CMD_CLEAR_STACK;
        /* MODE */
        case MID_RAD:             return CMD_MODE_RAD;
        case MID_DEG:             return CMD_MODE_DEG;
        /* STK */
        case MID_STACK_DUP:       return CMD_ENTER;
        case MID_STACK_ROLL_UP:   return CMD_ROLL_UP;
        case MID_STACK_ROLL_DOWN: return CMD_ROLL_DOWN;
        case MID_STACK_DROP:      return CMD_DROP;
        case MID_STACK_EXCHANGE_XY: return CMD_SWAP;
        default:                  return CMD_NONE;
    }
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
