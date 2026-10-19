#ifndef RPNCE_CORE_CALC_QUANTITY_H
#define RPNCE_CORE_CALC_QUANTITY_H

#include "calc.h"

typedef enum { QUANTITY_OK, QUANTITY_TYPE, QUANTITY_MISMATCH } quantity_err_t;

/* Prepare numeric operands under the OS error handler. Compatible pairs
 * use Y's unit; the result tag says whether the answer is a quantity.
 * The original operands remain untouched for LASTX and error recovery. */
quantity_err_t calc_quantity_prepare(calc_cmd_t cmd, uint8_t uy,
                                    const real_t *x, uint8_t ux, real_t *converted_x,
                                    uint8_t *result_unit);

#endif
