#ifndef RPNCE_CORE_VALUE_FORMAT_H
#define RPNCE_CORE_VALUE_FORMAT_H
#include "value.h"
/* Numeric/record formatting shared by stack, catalog and SHOW. Unit labels
 * are laid out by the caller, using the unit owned by the value. */
void value_format(char out[VALUE_FORMAT_MAX], const value_t *v,
                  uint8_t style, int8_t digits, uint8_t complex_mode,
                  bool decimal_comma, bool date_as_string, bool show);
void value_format_entry(char out[VALUE_FORMAT_MAX], const char *text,
                        bool decimal_comma, bool base_mode);
#endif
