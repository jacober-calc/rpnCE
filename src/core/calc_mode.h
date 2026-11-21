#ifndef RPNCE_CORE_CALC_MODE_H
#define RPNCE_CORE_CALC_MODE_H

#include "calc.h"

/* Settings chosen directly by a command, such as RAD or BEG. calc_execute()
 * finishes entry first; the stack and stack lift are untouched. Returns
 * false for any other command. */
bool calc_mode_set(calc_t *c, calc_cmd_t cmd, uint8_t arg);

/* Whether the setting a command selects is in effect, for menu labels. */
bool calc_mode_active(const calc_t *c, calc_cmd_t cmd, uint8_t arg);

#endif
