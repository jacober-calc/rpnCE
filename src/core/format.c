#include "format.h"

#define TI_NEG 0x1A   /* TI-ASCII negative sign */
#define TI_EXP 0x1B   /* TI-ASCII small E */

void format_real(char *out, size_t size, const real_t *v, bool full)
{
    char buf[32];

    os_RealToStr(buf, v, full ? 20 : 14, 1, -1);

    size_t i = 0;
    for (const char *p = buf; *p && i + 1 < size; p++) {
        char ch = *p;
        if (ch == TI_NEG)
            ch = '-';
        else if (ch == TI_EXP)
            ch = 'E';
        out[i++] = ch;
    }
    out[i] = '\0';
}
