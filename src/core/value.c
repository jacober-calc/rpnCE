#include <string.h>
#include "value.h"
#include "mathx.h"
#include "units.h"

/* Keep one constructor body per value kind. CE LTO otherwise duplicates
 * the complete container initialization across many callers. */
__attribute__((noinline)) value_t value_real(real_t n)
{
    value_t v;
    memset(&v, 0, sizeof v);
    v.real = n;
    return v;
}

__attribute__((noinline)) value_t value_complex(real_t re, real_t im)
{
    value_t v = value_real(re);
    v.imag = im;
    v.type = VALUE_COMPLEX;
    return v;
}

__attribute__((noinline)) value_t value_date(const dt_obj_t *date)
{
    value_t v;
    memset(&v, 0, sizeof v);
    v.type = VALUE_DATE;
    v.date = *date;
    return v;
}

bool value_is_real(const value_t *v) { return v->type == VALUE_REAL; }
bool value_is_plain(const value_t *v) { return value_is_real(v) && !v->unit; }
bool value_is_date(const value_t *v) { return v->type == VALUE_DATE; }
const real_t *value_get_real(const value_t *v)
{
    return value_is_plain(v) ? &v->real : NULL;
}

bool value_is_zero(const value_t *v)
{
    return !v->unit && (value_is_real(v) || v->type == VALUE_COMPLEX) &&
           mx_is_zero(&v->real) && (value_is_real(v) || mx_is_zero(&v->imag));
}

bool value_valid(const value_t *v)
{
    if (value_is_real(v)) return v->unit <= UNIT_COUNT;
    if (v->unit) return false;
    if (v->type == VALUE_COMPLEX) return true;
    return value_is_date(v) && dt_validate(&v->date) == DT_OK;
}
