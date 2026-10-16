#ifndef RPNCE_CORE_FORMAT_H
#define RPNCE_CORE_FORMAT_H

#include <stdbool.h>
#include <stddef.h>
#include <ti/real.h>

/* Formats a real as plain ASCII ('-' and 'E' instead of TI's special
 * characters). full = true shows all 14 significant digits. */
void format_real(char *out, size_t size, const real_t *v, bool full);

#endif
