#include <string.h>
#include "value_format.h"
#include "value_math.h"
#include "format.h"

static const char *complex_separator(uint8_t mode)
{
    static const char separators[][3] = {
        { TEXT_IMAG, 0, 0 }, { TEXT_ANGLE, 0, 0 },
        { TEXT_ANGLE, TEXT_DEGREE, 0 }
    };
    return separators[mode];
}

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
        strcat(out, complex_separator(complex_mode));
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
    strcat(out, complex_separator(*delimiter == 'i' ? COMPLEX_RECT :
                                  *delimiter == 'd' ? COMPLEX_DEG : COMPLEX_RAD));
    format_entry(second, delimiter + 1, decimal_comma);
    strcat(out, second);
}
