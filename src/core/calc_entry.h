#ifndef RPNCE_CORE_CALC_ENTRY_H
#define RPNCE_CORE_CALC_ENTRY_H

/* Private editing operations used by calc.c. */
#include "calc.h"

bool calc_entry_is_object(const calc_t *c);
bool calc_entry_in_record(const calc_t *c);
void calc_entry_clear(calc_t *c);
void calc_entry_append(calc_t *c, char ch);
void calc_entry_digit(calc_t *c, uint8_t d);
void calc_entry_dot(calc_t *c);
void calc_entry_ee(calc_t *c);
void calc_entry_chs(calc_t *c);
void calc_entry_backspace(calc_t *c);
void calc_entry_record_open(calc_t *c);
void calc_entry_record_tag(calc_t *c, uint8_t type);
void calc_entry_alpha(calc_t *c, uint8_t letter);

#endif
