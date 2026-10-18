#ifndef RPNCE_CORE_ROUNDING_H
#define RPNCE_CORE_ROUNDING_H

/* Exact decimal rounding on the calculator's 14-digit numbers, done on
 * their stored digits (half away from zero), so 2.675 rounds to 2.68. */

#include <ti/real.h>

real_t round_significant(const real_t *v, int digits);  /* RNDG: 1 to 14 */
real_t round_decimals(const real_t *v, int decimals);   /* RNDN */

#endif
