#ifndef RPNCE_CORE_CALC_INTERNAL_H
#define RPNCE_CORE_CALC_INTERNAL_H

/* Private stack operations for the calculator's implementation modules.
 * UI and system callers use calc.h instead. */
#include "calc.h"

static inline bool calc_base_mode(const calc_t *c)
{
    return c->base.radix != 10;
}

/* Errors may be NULL (no error), so evaluator results pass straight in. */
static inline void calc_error(calc_t *c, const char *text) { calc_set_message(c, MSG_ERROR, text); }
static inline void calc_notice(calc_t *c, const char *text) { calc_set_message(c, MSG_NOTICE, text); }

/* Evaluates a DATE-only or ordinary math command against the stack and
 * applies its result, as for a key press after entry has finished. */
void calc_apply(calc_t *c, calc_cmd_t cmd, uint8_t arg);

void calc_stack_push(calc_t *c, const value_t *v);
void calc_stack_enter(calc_t *c, const value_t *v);
void calc_stack_set_x(calc_t *c, const value_t *v);
void calc_stack_set_result(calc_t *c, const value_t *v, bool drop);

#endif
