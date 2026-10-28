#ifndef RPNCE_CORE_FORMAT_H
#define RPNCE_CORE_FORMAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Display styles. FIX is DISP_FIX with digits 0-9; ALL is DISP_FIX with
 * digits = DISP_FLOAT. */
#define DISP_FIX    1
#define DISP_SCI    2
#define DISP_ENG    3
#define DISP_FLOAT  (-1)
#define DISP_MAX_DIGITS 11  /* FIX/SCI/ENG 0-11, like the HP-32SII */

#define DISPLAY_DIGITS 12   /* most digits shown on the stack, like the HP-32SII */
#define DISPLAY_EXP_DIGITS 10 /* ... when an exponent is shown too (it needs room) */
#define SHOW_DIGITS    14   /* SHOW: everything the calculator stores */

#define FORMAT_MAX 28       /* longest formatted number, with terminator */

/* Swap decimal/grouping marks on formatted numeric text, leaving the
 * canonical parser representation and DATE records alone. */
void format_separators(char *text, bool decimal_comma);

/* Display-only grouping for a canonical decimal entry (no rounding or
 * cursor). out needs strlen(entry) + strlen(entry)/3 + 1 bytes. */
void format_entry(char *out, const char *entry, bool decimal_comma);

/*
 * Formats a number given as 14 decimal digits (d[0] is the leading digit;
 * value = d0.d1d2...d13 x 10^exp) following HP-32SII display rules:
 *   ALL      as many digits as needed, up to max_digits, fixed if it fits
 *   FIX n    n decimals; fewer if the number needs the room; SCI if the
 *            number is too big, or too small to show as anything but 0
 *   SCI n    d.ddd...En
 *   ENG n    n+1 significant digits, exponent a multiple of 3
 * The decimal point is always shown, as on HP calculators ("3.").
 * max_exp_digits: most significant digits when an exponent is shown.
 * group: put commas between thousands in the whole-number part.
 */
void format_digits(char *out, bool negative, int exp, const uint8_t d[14],
                   uint8_t style, int8_t digits, int max_digits, int max_exp_digits,
                   bool group);

#if defined(__ez80__) || defined(RPNCE_HOST_TEST)
#include <ti/real.h>
/* Formats a TI real (see format_digits). */
void format_real(char *out, const real_t *v, uint8_t style, int8_t digits,
                 int max_digits, int max_exp_digits, bool group);
#endif

#endif
