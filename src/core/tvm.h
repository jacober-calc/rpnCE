#ifndef RPNCE_CORE_TVM_H
#define RPNCE_CORE_TVM_H

/*
 * Time value of money, RPN83P / HP-12C style.
 *
 * N, I%YR, PV, PMT, FV, P/YR and C/YR live in the calculator's own TVM
 * variables, so they are shared with TI's TVM Solver app (APPS > Finance).
 * Cash flow signs follow the usual convention: money received is positive,
 * money paid out is negative. The equation, with i the rate per payment:
 *
 *   0 = PV + (1 + i*p) * PMT * (1 - (1+i)^-N) / i + FV * (1+i)^-N
 *
 * p is 1 for payments at the beginning of each period (BEG), 0 at the end.
 * i comes from I%YR compounded C/YR times a year, paid P/YR times a year.
 */

#include <stdbool.h>
#include <stdint.h>
#include <ti/real.h>

enum { TVM_N, TVM_IYR, TVM_PV, TVM_PMT, TVM_FV, TVM_PYR, TVM_CYR, TVM_VAR_COUNT };

typedef struct {
    bool begin;             /* payments at the beginning of each period */
    real_t iyr1, iyr2;      /* starting guesses for solving I%YR, in % */
    uint8_t iter_max;       /* most iterations when solving I%YR */
    bool iyr1_set, iyr2_set, iter_max_set;   /* changed from the defaults */
} tvm_settings_t;

void tvm_settings_reset(tvm_settings_t *s);     /* RSTV */
void tvm_clear(tvm_settings_t *s);              /* CLTV: values and BEG/END */

real_t *tvm_var(uint8_t which);                  /* the stored value */

/* Solves for one of N, I%YR, PV, PMT, FV from the other four. Call under the
 * error handler: raises OS_E_SIGNCHANGE or OS_E_ITERATIONS when there is no
 * solution or it can't be found, OS_E_DIVBY0 / OS_E_DOMAIN for bad inputs. */
real_t tvm_solve(uint8_t which, const tvm_settings_t *s);

#endif
