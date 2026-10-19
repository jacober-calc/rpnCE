#ifndef RPNCE_UI_CATALOG_H
#define RPNCE_UI_CATALOG_H

/* The VARS catalog, HP-32SII style: TVM values and every register holding
 * a nonzero value or a unit,
 * listed as NAME=value across the four stack lines. Up/down move through
 * it, ENTER copies the selected value to X, ON / CLEAR / VARS close it. */

#include <stdbool.h>
#include <stdint.h>
#include "../core/calc.h"

#define CAT_TVM_FIRST 0xE0 /* TVM_N..TVM_CYR; outside numbered registers */
#define CAT_TVM_LAST (CAT_TVM_FIRST + TVM_VAR_COUNT - 1)
#define CAT_LASTX 0xFF      /* list item for LASTX (not a numbered register) */

typedef struct {
    bool open;
    uint8_t count;
    uint8_t sel;            /* selected item */
    uint8_t top;            /* first item shown */
    uint8_t items[REG_EXTRA_FIRST + REG_EXTRA_MAX + TVM_VAR_COUNT];
} catalog_t;

/* Opens the catalog. TVM and the final LASTX entry are included even if zero. */
bool catalog_open(catalog_t *cat, calc_t *c);
void catalog_move(catalog_t *cat, int delta);

/* Names include A, i, stat sums, R041, LASTX and the seven TVM labels. */
void catalog_name(uint8_t item, char *out);
const real_t *catalog_value(calc_t *c, uint8_t item);
uint8_t catalog_unit(calc_t *c, uint8_t item);

/* Recall through the appropriate calculator command, preserving lift rules. */
void catalog_recall(calc_t *c, uint8_t item);

#endif
