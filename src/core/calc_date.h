#ifndef RPNCE_CORE_CALC_DATE_H
#define RPNCE_CORE_CALC_DATE_H

/* Private DATE integration used by command and register execution. */
#include "calc.h"

void calc_date_error(calc_t *c, dt_err_t e);
dt_err_t calc_date_op(calc_cmd_t op, const real_t *y, const real_t *x, real_t *out);
void calc_date_binary(calc_t *c, calc_cmd_t op);
void calc_date_execute(calc_t *c, date_fn_t fn);

#endif
