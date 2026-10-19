#ifndef RPNCE_UI_DISPLAY_H
#define RPNCE_UI_DISPLAY_H

#include <stdbool.h>
#include "../core/calc.h"
#include "menu.h"
#include "catalog.h"

void display_open(void);

void display_help(uint8_t page);

/* A full-screen notice (missing resources, say): a title and a detail line,
 * with "Press any key." below. */
void display_problem(const char *title, const char *detail);

/* Persistent order: LCD, LCD+, GRAY, INVERTED, AYU DARK, MONOKAI, PASTEL, COMMODORE. */
#define TINT_COUNT 8
void display_set_tint(uint8_t tint);
const char *display_tint_name(uint8_t tint);

/* Scrolling a value too long for the X row (long BIN numbers): delta > 0
 * shows digits further to the left. Reset when X changes. */
void display_scroll_x(int delta);
void display_reset_scroll(void);
void display_close(void);

/* Redraws the whole screen from the calculator state. */
/* cat: the VARS catalog, shown in place of the stack while open.
 * prompt: STO/RCL prompt to show in the X row, or NULL.
 * prompt_lifts: show the stack lifted under the prompt (plain RCL). */
void display_draw(const calc_t *c, const menu_t *m, bool second, bool alpha,
                  const char *prompt, bool prompt_lifts, const catalog_t *cat);

#endif
