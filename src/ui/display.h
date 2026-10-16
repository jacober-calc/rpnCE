#ifndef RPNCE_UI_DISPLAY_H
#define RPNCE_UI_DISPLAY_H

#include <stdbool.h>
#include "../core/calc.h"

void display_open(void);
void display_close(void);

/* Redraws the whole screen from the calculator state. */
void display_draw(const calc_t *c, bool second);

#endif
