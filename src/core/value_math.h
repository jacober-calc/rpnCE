#ifndef RPNCE_CORE_VALUE_MATH_H
#define RPNCE_CORE_VALUE_MATH_H
#include "value.h"
#include "commands.h"
#include "stats.h"
#include "tvm.h"
#include "base.h"

typedef struct {
    bool degrees, complex_results;
    uint8_t complex_mode, disp_style;
    int8_t disp_digits;
    uint8_t arg;             /* command argument: precision, unit, TVM variable, DATE alias or BASE op */
    const tvm_settings_t *tvm;
    stats_t *stats;
    base_state_t *base;
} value_context_t;

/* x/y are the new values. pair returns both; drop consumes old Y; split
 * expands one operand into Y and X instead of replacing two levels. */
typedef struct { value_t x, y; bool pair, drop, split; } value_result_t;
void value_math_init(void);
/* NULL on success; errors leave operands and the caller's state untouched.
 * Statistics callers supply a trial accumulator for transactional updates. */
const char *value_eval(const value_context_t *ctx, calc_cmd_t cmd,
                       const value_t *y, const value_t *x, value_result_t *out);
/* Formatting uses the same protected conversions as arithmetic. */
const char *value_polar(const value_t *v, bool degrees, real_t *r, real_t *angle);
#endif
