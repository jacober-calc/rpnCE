#ifndef RPNCE_CORE_MATHX_H
#define RPNCE_CORE_MATHX_H

/* Extra real-number math shared by the calculator, TVM and units. All of it
 * may raise OS math errors, so call it under the error handler. */

#include <stdbool.h>
#include <ti/real.h>

void mathx_init(void);

extern real_t mx_zero, mx_one, mx_two, mx_ten, mx_hundred;

bool mx_is_zero(const real_t *v);
bool mx_is_neg(const real_t *v);
real_t mx_abs(const real_t *v);

/* e^x - 1 and ln(1 + x), accurate for tiny x. ln1p needs x > -1. */
real_t mx_expm1(const real_t *x);
real_t mx_ln1p(const real_t *x);

/* Hyperbolic functions, accurate near 0. acosh needs x >= 1 and atanh
 * |x| < 1 (they raise OS_E_DOMAIN otherwise). */
real_t mx_sinh(const real_t *x);
real_t mx_cosh(const real_t *x);
real_t mx_tanh(const real_t *x);
real_t mx_asinh(const real_t *x);
real_t mx_acosh(const real_t *x);
real_t mx_atanh(const real_t *x);

/* Hours and H.MMSS (hours, minutes, seconds: 1.3045 is 1:30:45). */
real_t mx_hms_to_hours(const real_t *hms);
real_t mx_hours_to_hms(const real_t *hours);

#endif
