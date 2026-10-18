#ifndef RPNCE_UI_ACTIONS_H
#define RPNCE_UI_ACTIONS_H

#include <stdbool.h>
#include "../core/calc.h"
#include "menu_data.h"

/* Calculator command for a menu item, or CMD_NONE if not implemented yet.
 * *arg gets the command's argument (a unit or TVM variable); shifted is
 * true when the item was pressed after 2ND (recall, for TVM values). */
calc_cmd_t action_for(menu_id_t id, bool shifted, uint8_t *arg);

/* For mode items: is this mode currently on (show the alternate label)? */
bool action_is_active(const calc_t *c, menu_id_t id);

#endif
