#ifndef RPNCE_SYS_STORAGE_H
#define RPNCE_SYS_STORAGE_H

#include <stdbool.h>
#include "../core/calc.h"

/* Saves the stack, LASTX, variables, and modes to an archived appvar. */
void storage_save(calc_t *c);

/* Restores saved state. Returns false (leaving c alone) if there is none
 * or it is from an incompatible version. */
bool storage_load(calc_t *c);

/* Settings kept in their own small appvar: the screen tint and the DATE
 * settings (display mode, epoch, time zones), plus numeric separators.
 * Loading returns the tint (0 if none saved) and fills in those settings. */
void storage_save_settings(uint8_t tint, const calc_t *c);
uint8_t storage_load_settings(calc_t *c);

#endif
