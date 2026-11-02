#include "actions.h"
#include "../core/calc_mode.h"
#include "../core/format.h"

/* All menu actions, including unit indexes, share one generated table
 * indexed by menu ID; unbound IDs are zero, which is CMD_NONE. */
_Static_assert(CMD_NONE == 0 && CMD_COUNT <= UINT8_MAX + 1,
               "menu actions must fit in a byte");
#include "menu_actions.h"

static calc_cmd_t lookup_action(menu_id_t id, uint8_t *arg)
{
    if ((unsigned)id >= MID_COUNT) {
        *arg = 0;
        return CMD_NONE;
    }
    *arg = actions[id].arg;
    return (calc_cmd_t)actions[id].cmd;
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
    /* FIX, SCI and ENG open a digit prompt instead of running a command. */
    switch (id) {
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
