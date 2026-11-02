#include <stddef.h>
#include "calc_mode.h"

enum { REVERSE = 1, HIGHLIGHT = 2 };
typedef struct {
    uint16_t offset;
    uint8_t first, count, flags;
} calc_mode_t;

/* Command IDs stay unchanged. DATE subcommands occupy the following range
 * only within this private lookup. Every target field is one byte; bool
 * assignments use only the canonical representations 0 and 1. */
#define DATE_KEY(fn) (CMD_COUNT + (fn))
#define MODE(first, last, field, flags) \
    { offsetof(calc_t, field), first, (last) - (first) + 1, flags }
static const calc_mode_t modes[] = {
    MODE(CMD_MODE_RAD, CMD_MODE_DEG, degrees, HIGHLIGHT),
    MODE(CMD_CLEAR_CARRY, CMD_SET_CARRY, base.carry, 0),
    MODE(CMD_TVM_BEGIN, CMD_TVM_END, tvm.begin, REVERSE | HIGHLIGHT),
    MODE(CMD_DECIMAL_POINT, CMD_DECIMAL_COMMA, decimal_comma, HIGHLIGHT),
    MODE(CMD_RESULT_REAL, CMD_RESULT_COMPLEX, complex_results, HIGHLIGHT),
    MODE(CMD_COMPLEX_RECT, CMD_COMPLEX_DEG, complex_mode, HIGHLIGHT),
    MODE(DATE_KEY(DF_EPOCH_UNIX), DATE_KEY(DF_EPOCH_CUSTOM), epoch, HIGHLIGHT),
    MODE(DATE_KEY(DF_FORMAT_RAW), DATE_KEY(DF_FORMAT_STRING), date_as_string, HIGHLIGHT)
};
#undef MODE

_Static_assert(sizeof(bool) == 1, "mode flags require byte booleans");
_Static_assert(sizeof(calc_t) <= UINT16_MAX, "mode offsets require 16 bits");
_Static_assert(DATE_KEY(DF_FORMAT_STRING) <= UINT8_MAX, "mode keys require 8 bits");
_Static_assert(CMD_TVM_END == CMD_TVM_BEGIN + 1, "reversed TVM choices require two modes");

static const calc_mode_t *find_mode(calc_cmd_t cmd, uint8_t arg, uint8_t *value)
{
    if ((unsigned)cmd >= CMD_COUNT) return NULL;
    unsigned key = cmd == CMD_DATE_FN ? DATE_KEY(arg) : (unsigned)cmd;
    for (const calc_mode_t *m = modes; m < modes + sizeof modes / sizeof modes[0]; m++) {
        unsigned v = key - m->first;
        if (v < m->count) {
            *value = (uint8_t)(v ^ (m->flags & REVERSE));
            return m;
        }
    }
    return NULL;
}

bool calc_mode_set(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    uint8_t value;
    const calc_mode_t *m = find_mode(cmd, arg, &value);
    if (!m) return false;
    *((unsigned char *)c + m->offset) = value;
    return true;
}

bool calc_mode_active(const calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    uint8_t value;
    const calc_mode_t *m = find_mode(cmd, arg, &value);
    return m && (m->flags & HIGHLIGHT) &&
           *((const unsigned char *)c + m->offset) == value;
}
