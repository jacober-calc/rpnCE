#ifndef RPNCE_CORE_CALC_INTERNAL_H
#define RPNCE_CORE_CALC_INTERNAL_H

/* Private stack operations for the calculator's implementation modules.
 * UI and system callers use calc.h instead. */
#include "calc.h"

static inline bool calc_base_mode(const calc_t *c)
{
    return c->base.radix != 10;
}

void calc_stack_push(calc_t *c, const real_t *v);
void calc_stack_enter(calc_t *c, const real_t *v);
void calc_stack_set_x(calc_t *c, const real_t *v);
void calc_stack_set_result(calc_t *c, const real_t *v, bool drop);

#endif
