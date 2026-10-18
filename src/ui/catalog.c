#include <string.h>

#include "catalog.h"
#include "menu_data.h"      /* MSYM_SIGMA, MSYM_SQ */
#include "../core/dateobj.h"

/* Names of the stat registers 028-040. */
static const char *const stat_names[REG_STAT_LAST - REG_STAT_FIRST + 1] = {
    "n", "\x05x", "\x05y", "\x05x\x01", "\x05y\x01", "\x05xy",
    "\x05lnx", "\x05lnx\x01", "\x05lny", "\x05lny\x01",
    "\x05lnxlny", "\x05xlny", "\x05ylnx",
};

static bool is_zero_real(const real_t *v)
{
    if (obj_is(v))
        return false;                       /* a DATE object is never "zero" */
    real_t zero = os_Int24ToReal(0);
    return os_RealCompare(v, &zero) == 0;
}

const real_t *catalog_value(calc_t *c, uint8_t item)
{
    if (item == CAT_LASTX)
        return &c->lastx;
    return calc_register(c, item);
}

bool catalog_open(catalog_t *cat, calc_t *c)
{
    cat->count = 0;
    for (int num = REG_A; num < REG_EXTRA_FIRST + c->extra_count; num++) {
        const real_t *v = calc_register(c, (uint8_t)num);
        if (v && !is_zero_real(v))
            cat->items[cat->count++] = (uint8_t)num;
    }
    if (!is_zero_real(&c->lastx))
        cat->items[cat->count++] = CAT_LASTX;

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
