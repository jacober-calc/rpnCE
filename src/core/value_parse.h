#ifndef RPNCE_CORE_VALUE_PARSE_H
#define RPNCE_CORE_VALUE_PARSE_H
#include "value.h"
#include "base.h"
/* Canonical entry text: real, rectangular/polar pair, BASE word or DATE.
 * On failure the output is untouched; incomplete numeric entry follows the
 * calculator's editing conventions. */
const char *value_parse(const char *text, const base_state_t *base, value_t *out);
const char *value_date_error(dt_err_t e);
#endif
