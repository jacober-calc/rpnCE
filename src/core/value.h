#ifndef RPNCE_CORE_VALUE_H
#define RPNCE_CORE_VALUE_H

#include <stdbool.h>
#include <stdint.h>
#include <ti/real.h>
#include "datetime.h"

/* Every user value owns its type, numeric/record payload and optional unit.
 * Arithmetic kernels use real_t internally; containers always move value_t. */
typedef enum { VALUE_REAL, VALUE_COMPLEX, VALUE_DATE } value_type_t;
typedef enum { COMPLEX_RECT, COMPLEX_RAD, COMPLEX_DEG } complex_mode_t;
typedef struct {
    union {
        struct { real_t real, imag; };
        dt_obj_t date;
    };
    uint8_t type;
    uint8_t unit;             /* real quantities: unit index + 1, otherwise 0 */
} value_t;

#define VALUE_FORMAT_MAX 80

value_t value_real(real_t n);
value_t value_complex(real_t re, real_t im);
value_t value_date(const dt_obj_t *date);
bool value_is_real(const value_t *v);
bool value_is_plain(const value_t *v);
bool value_is_date(const value_t *v);
bool value_is_zero(const value_t *v);
bool value_valid(const value_t *v);
/* Checked real boundary for real-only consumers. */
const real_t *value_get_real(const value_t *v);

#endif
