#ifndef RPNCE_CORE_VALUE_DATE_H
#define RPNCE_CORE_VALUE_DATE_H
#include "value.h"
#include "commands.h"
#include "value_math.h"
dt_val_t value_date_operand(const value_t *v, bool quarters);
real_t value_whole_real(dt_int n);
dt_err_t value_date_eval(calc_cmd_t cmd, const value_t *y, const value_t *x, value_result_t *out);
#endif
