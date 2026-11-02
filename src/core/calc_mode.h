#ifndef RPNCE_CORE_CALC_MODE_H
#define RPNCE_CORE_CALC_MODE_H

#include "calc.h"

/* Neutral byte-valued settings share their assignment and menu selection
 * rules. Entry completion remains the responsibility of calc_execute(). */
bool calc_mode_set(calc_t *c, calc_cmd_t cmd, uint8_t arg);
bool calc_mode_active(const calc_t *c, calc_cmd_t cmd, uint8_t arg);

#endif
