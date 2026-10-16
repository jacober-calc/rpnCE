#ifndef RPNCE_UI_ACTIONS_H
#define RPNCE_UI_ACTIONS_H

#include <stdbool.h>
#include "../core/calc.h"
#include "menu_data.h"

/* Calculator command for a menu item, or CMD_NONE if not implemented yet. */
calc_cmd_t action_for(menu_id_t id);

/* For mode items: is this mode currently on (show the alternate label)? */
bool action_is_active(const calc_t *c, menu_id_t id);

#endif
