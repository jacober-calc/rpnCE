/* The calculator's real number layout, for testing code that works on its
 * digits directly. */
#ifndef BCD_TI_REAL_H
#define BCD_TI_REAL_H
#include <stdint.h>
#include <stdbool.h>
typedef struct real_t { int8_t sign, exp; uint8_t mant[7]; } real_t;
#endif
