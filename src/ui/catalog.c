#include <string.h>

#include "catalog.h"
#include "menu_data.h"      /* MSYM_SIGMA, MSYM_SQ */
#include "../core/value.h"

/* Names of the stat registers 028-040. */
static const char *const stat_names[REG_STAT_LAST - REG_STAT_FIRST + 1] = {
    "n", "\x05x", "\x05y", "\x05x\x01", "\x05y\x01", "\x05xy",
    "\x05lnx", "\x05lnx\x01", "\x05lny", "\x05lny\x01",
    "\x05lnxlny", "\x05xlny", "\x05ylnx",
};

static const char *const tvm_names[TVM_VAR_COUNT] = {
    "N", "I%YR", "PV", "PMT", "FV", "P/YR", "C/YR",
};

static bool is_tvm(uint8_t item)
{
    return item >= CAT_TVM_FIRST && item <= CAT_TVM_LAST;
}

bool catalog_value(const calc_t *c, uint8_t item, value_t *out)
{
    if (item == CAT_LASTX) { *out = c->lastx; return true; }
    if (is_tvm(item)) { *out = value_real(*tvm_var((uint8_t)(item - CAT_TVM_FIRST))); return true; }
    return calc_register_read(c, item, out);
}

bool catalog_open(catalog_t *cat, calc_t *c)
{
    cat->count = 0;
    for (int num = REG_A; num < REG_EXTRA_FIRST + c->extra_count; num++) {
        value_t v;
        if (calc_register_read(c, (uint8_t)num, &v) && !value_is_zero(&v))
            cat->items[cat->count++] = (uint8_t)num;
    }
    /* Zero is meaningful in TVM, notably FV=0 for a paid-off loan. */
    for (uint8_t v = 0; v < TVM_VAR_COUNT; v++)
        cat->items[cat->count++] = (uint8_t)(CAT_TVM_FIRST + v);

    cat->items[cat->count++] = CAT_LASTX; /* always visible, always last */

    cat->sel = cat->top = 0;
    cat->open = cat->count > 0;
    return cat->open;
}

void catalog_move(catalog_t *cat, int delta)
{
    int s = (int)cat->sel + delta;
    if (s < 0)
        s = cat->count - 1;                 /* wrap around */
    if (s >= cat->count)
        s = 0;
    cat->sel = (uint8_t)s;
    if (cat->sel < cat->top)
        cat->top = cat->sel;
    if (cat->sel >= cat->top + 4)
        cat->top = (uint8_t)(cat->sel - 3);
}

void catalog_name(uint8_t item, char *out)
{
    if (item == CAT_LASTX) {
        strcpy(out, "LASTX");
    } else if (is_tvm(item)) {
        strcpy(out, tvm_names[item - CAT_TVM_FIRST]);
    } else if (item <= 26) {
        out[0] = (char)('A' + item - 1);
        out[1] = '\0';
    } else if (item == REG_I) {
        strcpy(out, "i");
    } else if (item <= REG_STAT_LAST) {
        strcpy(out, stat_names[item - REG_STAT_FIRST]);
    } else {
        out[0] = 'R';
        out[1] = (char)('0' + item / 100);
        out[2] = (char)('0' + item / 10 % 10);
        out[3] = (char)('0' + item % 10);
        out[4] = '\0';
    }
}

void catalog_recall(calc_t *c, uint8_t item)
{
    if (item == CAT_LASTX)
        calc_execute(c, CMD_LASTX, 0);
    else if (is_tvm(item))
        calc_execute(c, CMD_TVM_VAR, (uint8_t)((item - CAT_TVM_FIRST) | TVM_RECALL));
    else
        calc_execute(c, CMD_RCL, item);
}
