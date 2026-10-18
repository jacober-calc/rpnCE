#ifndef RPNCE_UI_CATALOG_H
#define RPNCE_UI_CATALOG_H

/* The VARS catalog, HP-32SII style: every register holding a nonzero value,
 * listed as NAME=value across the four stack lines. Up/down move through
 * it, ENTER copies the selected value to X, ON / left / CLEAR close it. */

#include <stdbool.h>
#include <stdint.h>
#include "../core/calc.h"

#define CAT_LASTX 0xFF      /* list item for LASTX (not a numbered register) */

typedef struct {
    bool open;
    uint8_t count;
    uint8_t sel;            /* selected item */
    uint8_t top;            /* first item shown */
    uint8_t items[REG_EXTRA_FIRST + REG_EXTRA_MAX];
} catalog_t;

/* Opens the catalog. Returns false (and stays closed) if every register is 0. */
bool catalog_open(catalog_t *cat, calc_t *c);
void catalog_move(catalog_t *cat, int delta);

/* The name ("A", "i", "\x05x\x01" for Sx2, "R041", "LASTX") and value of an item. */
void catalog_name(uint8_t item, char *out);
const real_t *catalog_value(calc_t *c, uint8_t item);

#endif
