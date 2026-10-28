#include "calc_quantity.h"
#include "units.h"

static bool restricted(uint8_t tag)
{
    if (!tag)
        return false;
    uint8_t dim = unit_dim((uint8_t)(tag - 1));
    return dim == DIM_TEMP || dim == DIM_FUEL;
}

quantity_err_t calc_quantity_prepare(calc_cmd_t cmd, uint8_t uy,
                                    const real_t *x, uint8_t ux, real_t *converted_x,
                                    uint8_t *result_unit)
{
    *converted_x = *x;
    *result_unit = 0;
    if (uy > UNIT_COUNT || ux > UNIT_COUNT)
        return QUANTITY_TYPE;
    switch (cmd) {
        /* These operate on the displayed value, including TEMP and FUEL. */
        case CMD_INT_PART: case CMD_FRAC_PART: case CMD_FLOOR: case CMD_CEIL:
        case CMD_NEAR: case CMD_ROUND_FIX: case CMD_ROUND_N: case CMD_ROUND_G:
            *result_unit = ux;
            return QUANTITY_OK;
        default:
            break;
    }
    if (restricted(uy) || restricted(ux))
        return QUANTITY_TYPE;
    switch (cmd) {
        case CMD_ABS:
            *result_unit = ux;
            return QUANTITY_OK;
        case CMD_SIGN:
            return QUANTITY_OK;
        case CMD_PERCENT:
            if (ux) return QUANTITY_TYPE;  /* X is a plain percentage */
            *result_unit = uy;
            return QUANTITY_OK;
        case CMD_MUL:
            if (uy && ux) return QUANTITY_TYPE; /* compound units aren't supported */
            *result_unit = uy ? uy : ux;
            return QUANTITY_OK;
        case CMD_DIV:
            if (uy && !ux) {
                *result_unit = uy;
                return QUANTITY_OK;
            }
            if (!uy) return QUANTITY_TYPE; /* no inverse-unit objects */
            break;
        case CMD_ADD: case CMD_SUB: case CMD_MOD: case CMD_MIN: case CMD_MAX:
        case CMD_PERCENT_CHANGE:
            *result_unit = cmd == CMD_PERCENT_CHANGE ? 0 : uy;
            break;
        default:
            return QUANTITY_TYPE;
    }
    if (!uy || !ux)
        return QUANTITY_TYPE;
    if (unit_dim((uint8_t)(uy - 1)) != unit_dim((uint8_t)(ux - 1)))
        return QUANTITY_MISMATCH;
    *converted_x = unit_convert(x, (uint8_t)(ux - 1), (uint8_t)(uy - 1));
    return QUANTITY_OK;
}
