#ifndef RPNCE_CORE_CALC_DATE_H
#define RPNCE_CORE_CALC_DATE_H

/* Private DATE integration used by command and register execution. */
#include "calc.h"

void calc_date_execute(calc_t *c, date_fn_t fn);

#endif
