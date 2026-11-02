#include <string.h>
#include "value_format.h"
#include "value_math.h"
#include "format.h"

void value_format(char out[VALUE_FORMAT_MAX], const value_t *v,
                  uint8_t style, int8_t digits, uint8_t complex_mode,
                  bool decimal_comma, bool date_as_string, bool show)
{
    if (value_is_date(v)) {
        dt_format(out, &v->date, show ? false : date_as_string);
        return;
    }
    real_t first = v->real, second = v->imag;
    if (v->type == VALUE_COMPLEX && complex_mode != COMPLEX_RECT) {
        const char *e = value_polar(v, complex_mode == COMPLEX_DEG, &first, &second);
        if (e) { strcpy(out, e); return; }
    }
    if (show) { style = DISP_FIX; digits = DISP_FLOAT; }
    int max = show ? SHOW_DIGITS : DISPLAY_DIGITS;
    int exp_max = show ? SHOW_DIGITS : DISPLAY_EXP_DIGITS;
    format_real(out, &first, style, digits, max, exp_max, true);
    format_separators(out, decimal_comma);
    if (v->type == VALUE_COMPLEX) {
        char part[FORMAT_MAX];
        format_real(part, &second, style, digits, max, exp_max, true);
        format_separators(part, decimal_comma);
        strcat(out, complex_mode == COMPLEX_RECT ? " i " : complex_mode == COMPLEX_RAD ? " < " : " <d ");
        strcat(out, part);
    }
}

void value_format_entry(char out[VALUE_FORMAT_MAX], const char *text,
                        bool decimal_comma, bool base_mode)
{
    if (base_mode || dt_text_is_object(text)) { strcpy(out, text); return; }
    const char *delimiter = strpbrk(text, "idr");
    if (!delimiter) { format_entry(out, text, decimal_comma); return; }
    char first[VALUE_FORMAT_MAX], second[VALUE_FORMAT_MAX];
    size_t n = (size_t)(delimiter - text);
    memcpy(first, text, n); first[n] = '\0';
    format_entry(out, first, decimal_comma);
    strcat(out, *delimiter == 'i' ? " i " : *delimiter == 'd' ? " <d " : " < ");
    format_entry(second, delimiter + 1, decimal_comma);
    strcat(out, second);
}
