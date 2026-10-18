#ifndef RPNCE_SYS_STORAGE_H
#define RPNCE_SYS_STORAGE_H

#include <stdbool.h>
#include "../core/calc.h"

/* Saves the stack, LASTX, variables, and modes to an archived appvar. */
void storage_save(calc_t *c);

/* Restores saved state. Returns false (leaving c alone) if there is none
 * or it is from an incompatible version. */
bool storage_load(calc_t *c);

#endif
