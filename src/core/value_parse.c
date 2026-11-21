#include <string.h>
#include <ti/error.h>
#include "value_parse.h"
#include "value_math.h"
#include "mathx.h"

const char *value_date_error(dt_err_t e)
{
    switch (e) {
        case DT_OK: return NULL;
        case DT_ERR_TYPE: return "Err: Data Type";
        case DT_ERR_DOMAIN: return "Err: Domain";
        case DT_ERR_SYNTAX: return "Err: Syntax";
        default: return "Err: Invalid";
    }
}
static const char *parse_real(const char *text, size_t n, real_t *out)
{
    if (n >= VALUE_FORMAT_MAX) return "Err: Syntax";
    char token[VALUE_FORMAT_MAX];
    memcpy(token, text, n);
    /* EE and its sign may still be waiting for digits when entry ends. */
    if (n && token[n - 1] == '-') {
        if (n == 1) n = 0;
        else if (token[n - 2] == 'E') n -= 2;
    } else if (n && token[n - 1] == 'E') n--;
    token[n] = '\0';
    int error = os_PushErrorHandler();
    if (error) return (error & OS_E_MASK) == (OS_E_OVERFLOW & OS_E_MASK) ? "Err: Overflow" :
                      (error & OS_E_MASK) == (MX_E_UNDERFLOW & OS_E_MASK) ? "Err: Underflow" : "Err: Syntax";
    const char *e = NULL;
    char *end;
    real_t result = n ? os_StrToReal(token, &end) : mx_zero;
    if (n && (end == token || *end)) {
        e = "Err: Syntax";
    } else {
        mx_check_range(&result);
        *out = result;
    }
    os_PopErrorHandler();
    return e;
}
const char *value_parse(const char *text, const base_state_t *base, value_t *out)
{
    if (dt_text_is_object(text)) {
        dt_obj_t date;
        dt_err_t e = dt_parse(text, &date);
        if (e) return value_date_error(e);
        *out = value_date(&date);
        return NULL;
    }
    if (base->radix != 10) {
        base_word_t word = 0;
        for (const char *p = text; *p; p++) {
            int digit = base_digit_value(base, *p);
            if (digit < 0) return "Err: Syntax";
            word = word * base->radix + digit;
            if (word > base_mask(base)) return "Err: Word Size";
        }
        *out = value_real(base_real(word));
        return NULL;
    }
    const char *delimiter = strpbrk(text, "idr");
    real_t first, second;
    const char *e = parse_real(text, delimiter ? (size_t)(delimiter - text) : strlen(text), &first);
    if (e) return e;
    if (!delimiter) { *out = value_real(first); return NULL; }
    e = parse_real(delimiter + 1, strlen(delimiter + 1), &second);
    if (e) return e;
    if (delimiter == text && !delimiter[1] && *delimiter == 'i') second = mx_one;
    value_context_t ctx = {0};
    ctx.complex_mode = *delimiter == 'i' ? COMPLEX_RECT : *delimiter == 'd' ? COMPLEX_DEG : COMPLEX_RAD;
    value_t y = value_real(first), x = value_real(second);
    value_result_t result;
    e = value_eval(&ctx, CMD_COMPLEX_LINK, &y, &x, &result);
    if (!e) *out = result.x;
    return e;
}
