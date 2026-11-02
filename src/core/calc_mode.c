#include "calc_mode.h"

/* Settings mirror their commands' order, so a command's offset from the
 * first choice is the setting's value. */
_Static_assert(CMD_COMPLEX_RAD - CMD_COMPLEX_RECT == COMPLEX_RAD &&
               CMD_COMPLEX_DEG - CMD_COMPLEX_RECT == COMPLEX_DEG &&
               DF_EPOCH_CUSTOM - DF_EPOCH_UNIX == EPOCH_CUSTOM,
               "mode commands must follow their settings' order");

bool calc_mode_set(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    switch (cmd) {
        case CMD_MODE_RAD: case CMD_MODE_DEG:
            c->degrees = cmd == CMD_MODE_DEG;
            return true;
        case CMD_CLEAR_CARRY: case CMD_SET_CARRY:
            c->base.carry = cmd == CMD_SET_CARRY;
            return true;
        case CMD_TVM_BEGIN: case CMD_TVM_END:
            c->tvm.begin = cmd == CMD_TVM_BEGIN;
            return true;
        case CMD_DECIMAL_POINT: case CMD_DECIMAL_COMMA:
            c->decimal_comma = cmd == CMD_DECIMAL_COMMA;
            return true;
        case CMD_RESULT_REAL: case CMD_RESULT_COMPLEX:
            c->complex_results = cmd == CMD_RESULT_COMPLEX;
            return true;
        case CMD_COMPLEX_RECT: case CMD_COMPLEX_RAD: case CMD_COMPLEX_DEG:
            c->complex_mode = (uint8_t)(cmd - CMD_COMPLEX_RECT);
            return true;
        case CMD_DATE_FN:
            if (arg >= DF_EPOCH_UNIX && arg <= DF_EPOCH_CUSTOM) {
                c->epoch = (uint8_t)(arg - DF_EPOCH_UNIX);
                return true;
            }
            if (arg == DF_FORMAT_RAW || arg == DF_FORMAT_STRING) {
                c->date_as_string = arg == DF_FORMAT_STRING;
                return true;
            }
            return false;
        default:
            return false;
    }
}

bool calc_mode_active(const calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    switch (cmd) {
        case CMD_MODE_RAD:       return !c->degrees;
        case CMD_MODE_DEG:       return c->degrees;
        case CMD_TVM_BEGIN:      return c->tvm.begin;
        case CMD_TVM_END:        return !c->tvm.begin;
        case CMD_DECIMAL_POINT:  return !c->decimal_comma;
        case CMD_DECIMAL_COMMA:  return c->decimal_comma;
        case CMD_RESULT_REAL:    return !c->complex_results;
        case CMD_RESULT_COMPLEX: return c->complex_results;
        case CMD_COMPLEX_RECT: case CMD_COMPLEX_RAD: case CMD_COMPLEX_DEG:
            return c->complex_mode == cmd - CMD_COMPLEX_RECT;
        case CMD_BASE_RADIX:     return c->base.radix == arg;
        case CMD_STAT_ALL_MODE:  return !c->stats.linear_only;
        case CMD_STAT_LIN_MODE:  return c->stats.linear_only;
        case CMD_FIT_MODEL:      return c->stats.model == arg;
        /* TVM solver settings show whether they differ from the defaults. */
        case CMD_TVM_IYR1:       return c->tvm.iyr1_set;
        case CMD_TVM_IYR2:       return c->tvm.iyr2_set;
        case CMD_TVM_ITER_MAX:   return c->tvm.iter_max_set;
        case CMD_DATE_FN:
            if (arg >= DF_EPOCH_UNIX && arg <= DF_EPOCH_CUSTOM)
                return c->epoch == arg - DF_EPOCH_UNIX;
            if (arg == DF_FORMAT_RAW || arg == DF_FORMAT_STRING)
                return c->date_as_string == (arg == DF_FORMAT_STRING);
            return false;
        default:
            return false;
    }
}
