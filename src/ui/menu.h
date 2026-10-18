#ifndef RPNCE_UI_MENU_H
#define RPNCE_UI_MENU_H

#include <stdbool.h>
#include <stdint.h>
#include "menu_data.h"

#define MENU_MAX_DEPTH 8

/* The menu row is always showing, RPN83P style. */
typedef struct {
    uint8_t group;              /* current menu */
    uint8_t row;                /* current row within it */
    uint8_t depth;
    struct { uint8_t group, row; } back[MENU_MAX_DEPTH];
} menu_t;

/* Goes to the top of the tree (HOME). */
void menu_home(menu_t *m);

/* Goes straight to a menu (MENU_GROUP_*), as if navigated to from HOME. */
void menu_jump(menu_t *m, uint8_t group);

void menu_row_up(menu_t *m);
void menu_row_down(menu_t *m);

/* Goes up one level (nothing at HOME). */
void menu_back(menu_t *m);

/* Node shown in softkey slot 0..4 of the current row. */
const menu_node_t *menu_slot(const menu_t *m, uint8_t slot);

/* Handles a softkey press. Opens a submenu and returns NULL, or returns the
 * item for the caller to run (NULL for a blank slot too). */
const menu_node_t *menu_press(menu_t *m, uint8_t slot);

#endif
