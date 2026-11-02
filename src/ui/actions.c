#include "actions.h"
#include "../core/calc_mode.h"
#include "../core/format.h"

/* All menu actions, including unit indexes, share one generated lookup. */
_Static_assert(CMD_NONE == 0 && CMD_COUNT <= UINT8_MAX + 1,
               "menu actions must fit in a byte");
#include "menu_actions.h"

static calc_cmd_t lookup_action(menu_id_t id, uint8_t *arg)
{
    *arg = 0;
    if ((unsigned)id >= MID_COUNT) return CMD_NONE;
    uint16_t first = 0;
    const uint8_t *p = actions;
    while (p < actions + sizeof actions) {
        uint8_t delta = *p++, cmd = *p++, base = *p++;
        first += delta & ACTION_DELTA;
        uint8_t count = (delta & ACTION_RUN) ? *p++ : 1;
        if ((unsigned)id < first) return CMD_NONE;
        unsigned offset = (unsigned)id - first;
        if (offset < count) {
            *arg = (uint8_t)(base + offset);
            return (calc_cmd_t)cmd;
        }
    }
    return CMD_NONE;
}

calc_cmd_t action_for(menu_id_t id, bool shifted, uint8_t *arg)
{
    calc_cmd_t cmd = lookup_action(id, arg);
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
        case MID_TVM_IYR0:  return c->tvm.iyr1_set;
        case MID_TVM_IYR1:  return c->tvm.iyr2_set;
        case MID_TVM_ITER_MAX: return c->tvm.iter_max_set;
        case MID_FIX: return c->disp_style == DISP_FIX && c->disp_digits != DISP_FLOAT;
        case MID_SCI: return c->disp_style == DISP_SCI;
        case MID_ENG: return c->disp_style == DISP_ENG;
        default: {
            uint8_t arg;
            calc_cmd_t cmd = lookup_action(id, &arg);
            return calc_mode_active(c, cmd, arg);
        }
    }
}
