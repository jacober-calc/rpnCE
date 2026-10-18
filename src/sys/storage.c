#include <string.h>
#include <fileioc.h>

#include "storage.h"
#include "../core/format.h"
#include "../core/units.h"

#define APPVAR_NAME   "RPNCEdat"
#define STATE_MAGIC   0x52   /* 'R' */
#define STATE_VERSION 5      /* bump when the saved layout changes */

/* Everything that survives quitting. The TVM values themselves live in the
 * OS's own TVM variables, so they aren't stored here. */
typedef struct {
    uint8_t magic;
    uint8_t version;
    uint8_t depth;
    real_t stack[STACK_MAX];
    uint8_t unit[STACK_MAX];
    real_t lastx;
    uint8_t unit_lastx;
    real_t regs[REG_BASE_COUNT];
    uint8_t extra_count;
    uint8_t lift;
    uint8_t degrees;
    uint8_t disp_style;
    int8_t disp_digits;
    tvm_settings_t tvm;
    stats_t stats;
    base_state_t base;
    real_t extra[REG_EXTRA_MAX];    /* last, so only the used part is saved */
} saved_state_t;

#define SAVED_FIXED_SIZE (sizeof(saved_state_t) - sizeof(real_t) * REG_EXTRA_MAX)

void storage_save(calc_t *c)
{
    static saved_state_t s;     /* static: it's large for the stack */

    calc_finish_entry(c);

    s.magic = STATE_MAGIC;
    s.version = STATE_VERSION;
    s.depth = c->depth;
    memcpy(s.stack, c->stack, sizeof s.stack);
    memcpy(s.unit, c->unit, sizeof s.unit);
    s.lastx = c->lastx;
    s.unit_lastx = c->unit_lastx;
    memcpy(s.regs, c->regs, sizeof s.regs);
    s.extra_count = c->extra_count;
    s.lift = c->lift;
    s.degrees = c->degrees;
    s.disp_style = c->disp_style;
    s.disp_digits = c->disp_digits;
    s.tvm = c->tvm;
    s.stats = c->stats;
    s.base = c->base;
    memcpy(s.extra, c->extra, sizeof(real_t) * c->extra_count);

    uint8_t h = ti_Open(APPVAR_NAME, "w");
    if (!h)
        return;
    ti_Write(&s, SAVED_FIXED_SIZE + sizeof(real_t) * c->extra_count, 1, h);
    ti_SetArchiveStatus(true, h);   /* archived: survives a RAM clear */
    ti_Close(h);
}

bool storage_load(calc_t *c)
{
    static saved_state_t s;

    uint8_t h = ti_Open(APPVAR_NAME, "r");
    if (!h)
        return false;
    size_t n = ti_Read(&s, 1, sizeof s, h);
    ti_Close(h);

    if (n < SAVED_FIXED_SIZE || s.magic != STATE_MAGIC || s.version != STATE_VERSION)
        return false;
    if (s.depth < STACK_MIN || s.depth > STACK_MAX || s.extra_count > REG_EXTRA_MAX)
        return false;
    if (n < SAVED_FIXED_SIZE + sizeof(real_t) * s.extra_count)
        return false;

    c->depth = s.depth;
    memcpy(c->stack, s.stack, sizeof c->stack);
    for (int i = 0; i < STACK_MAX; i++)
        c->unit[i] = (s.unit[i] <= UNIT_COUNT) ? s.unit[i] : 0;
    c->lastx = s.lastx;
    c->unit_lastx = (s.unit_lastx <= UNIT_COUNT) ? s.unit_lastx : 0;
    memcpy(c->regs, s.regs, sizeof c->regs);
    c->extra_count = s.extra_count;
    memcpy(c->extra, s.extra, sizeof(real_t) * s.extra_count);
    c->lift = s.lift != 0;
    c->degrees = s.degrees != 0;
    if (s.disp_style >= DISP_FIX && s.disp_style <= DISP_ENG &&
        s.disp_digits >= DISP_FLOAT && s.disp_digits <= 9) {
        c->disp_style = s.disp_style;
        c->disp_digits = s.disp_digits;
    }
    c->tvm = s.tvm;
    if (c->tvm.iter_max == 0)
        tvm_settings_reset(&c->tvm);
    c->stats = s.stats;
    if (c->stats.model >= FIT_COUNT)
        c->stats.model = FIT_LIN;
    c->base = s.base;
    base_validate(&c->base);
    return true;
}
