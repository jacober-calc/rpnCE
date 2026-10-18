#ifndef RPNCE_UI_RES_H
#define RPNCE_UI_RES_H

/*
 * RPNCE's resources: the menu tree and unit tables, kept in the RPNCEres
 * appvar (built by tools/genres.py) and read straight from archive, so they
 * use no RAM. res_open() must succeed before anything else here is used.
 *
 * The pointer stays valid as long as no archived variable is created,
 * deleted or moved. RPNCE only writes to archive when it exits.
 */

#include <stdbool.h>
#include <stdint.h>
#include "menu_data.h"

/* Opens RPNCEres and checks it matches this build. On failure, res_problem()
 * says what's wrong in a few words. */
bool res_open(void);
const char *res_problem(void);

const menu_node_t *menu_node(uint16_t index);
const menu_group_t *menu_group(uint8_t group);
uint8_t menu_parent_of(uint8_t group);       /* 0xFF for a top-level menu */
uint8_t menu_parent_row_of(uint8_t group);

/* Text of a label offset (from menu_node_t / menu_group_t). */
const char *res_text(uint16_t offset);

/* The unit (index) for a UNIT menu item, or -1. */
int unit_for_menu(menu_id_t id);

#endif
