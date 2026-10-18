#include <stddef.h>
#include "units.h"

static real_t factor_of(uint8_t u)
{
    return os_StrToReal(unit_table[u].factor, NULL);
}

static real_t to_base(const real_t *v, uint8_t u)
{
    const unit_t *t = &unit_table[u];
    real_t f = factor_of(u);
    if (t->reciprocal)
        return os_RealDiv(&f, v);
    if (t->offset) {
        real_t off = os_StrToReal(t->offset, NULL);
        real_t shifted = os_RealAdd(v, &off);
        return os_RealMul(&shifted, &f);
    }
    return os_RealMul(v, &f);
}

static real_t from_base(const real_t *b, uint8_t u)
{
    const unit_t *t = &unit_table[u];
    real_t f = factor_of(u);
    if (t->reciprocal)
        return os_RealDiv(&f, b);
    real_t v = os_RealDiv(b, &f);
    if (t->offset) {
        real_t off = os_StrToReal(t->offset, NULL);
        v = os_RealSub(&v, &off);
    }
    return v;
}

real_t unit_convert(const real_t *value, uint8_t from, uint8_t to)
{
    if (from == to)
        return *value;
    real_t b = to_base(value, from);
    return from_base(&b, to);
}
