#ifndef RPNCE_UI_RES_H
#define RPNCE_UI_RES_H

/* Resources are read directly from archive after res_open(). Creating,
 * deleting or moving archived variables invalidates the resource pointer;
 * archive writes must wait until exit. */

#include <stdbool.h>
#include <stdint.h>
#include "menu_data.h"

/* Opens RPNCEres and checks it matches this build. On failure, res_problem()
 * says what's wrong in a few words. */
bool res_open(void);
const char *res_problem(void);

const menu_node_t *menu_node(uint16_t index);
const menu_group_t *menu_group(uint8_t group);
/* Submenu target, blank ID and alternate label already identify the kind;
 * storing a second discriminator would duplicate that information. */
static inline menu_kind_t menu_node_kind(const menu_node_t *n)
{
    if (n->group != UINT8_MAX) return MENU_GROUP;
    if (n->id == MID_NONE) return MENU_BLANK;
    return n->alt_label == UINT16_MAX ? MENU_ITEM : MENU_ALT;
}
uint8_t menu_parent_of(uint8_t group);       /* 0xFF for a top-level menu */
uint8_t menu_parent_row_of(uint8_t group);

/* Text of a label offset (from menu_node_t / menu_group_t). */
const char *res_text(uint16_t offset);

/* HELP pages (from help/help.txt): how many there are, how many lines a
 * page has (its title first), and the text of each line. */
uint8_t res_help_pages(void);
uint8_t res_help_lines(uint8_t page);
const char *res_help_line(uint8_t page, uint8_t line);

#endif
