#include "actions.h"

calc_cmd_t action_for(menu_id_t id)
{
    switch (id) {
        /* MATH */
        case MID_CUBE:            return CMD_CUBE;
        case MID_CUBE_ROOT:       return CMD_CUBE_ROOT;
        case MID_XROOT_Y:         return CMD_XROOT_Y;
        case MID_TWO_POW:         return CMD_TWO_POW;
        case MID_LOG2:            return CMD_LOG2;
        case MID_LOG_BASE:        return CMD_LOG_BASE;
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
        /* CONV */
        case MID_RAD_TO_DEG:       return CMD_TO_DEG;
        case MID_DEG_TO_RAD:       return CMD_TO_RAD;
        /* CLR */
        case MID_CLEAR_X:         return CMD_CLEAR;
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
        default:      return false;
    }
}
