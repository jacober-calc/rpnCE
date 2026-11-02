#ifndef RPNCE_CORE_CALC_H
#define RPNCE_CORE_CALC_H

/*
 * Calculator state and commands. This module knows nothing about the
 * screen or keypad: the UI turns keys into commands and reads state back.
 */

#include <stdbool.h>
#include <stdint.h>
#include <ti/real.h>

#include "tvm.h"
#include "stats.h"
#include "base.h"
#include "datetime.h"
#include "datefn.h"
#include "commands.h"
#include "value.h"

#define ENTRY_MAX 46      /* room for 44 binary digits */
/* Stack: 4 levels like the HP-32SII, up to 12 with SSIZ. */
#define STACK_MIN 4
#define STACK_MAX 12

/* Registers, numbered as on the HP-32SII:
 *   001-026 A-Z, 027 i, 028-033 n Sx Sy Sx2 Sy2 Sxy (the 32SII's six stat
 *   registers), 034-040 the log sums used for curve fits, 041-140 extra
 *   registers: RSIZ sets the total, 40 to 140 (reachable through (i)). */
#define REG_A          1
#define REG_I          27
#define REG_STAT_FIRST 28
#define REG_STAT_LAST  40
#define REG_EXTRA_FIRST 41
#define REG_EXTRA_MAX  100
#define REG_BASE_COUNT 27            /* A-Z and i */
#define REG_INDIRECT   0             /* register argument meaning (i) */
#define TVM_RECALL 0x80  /* with CMD_TVM_*: recall the value (2ND + key) */

/* Stack levels. */
enum { REG_X, REG_Y, REG_Z, REG_T };

/* What a message is: an error stays until a key is pressed, a notice may
 * time out, and SHOW's full-precision view stays like an error. */
typedef enum { MSG_ERROR, MSG_NOTICE, MSG_SHOW } message_kind_t;

typedef struct {
    value_t stack[STACK_MAX];    /* stack[0] is X, [1] Y, [2] Z, [3] T, ... */
    uint8_t depth;              /* stack levels in use: 4 to STACK_MAX */
    value_t lastx;

    value_t regs[REG_BASE_COUNT];        /* A-Z and i (registers 001-027) */
    value_t extra[REG_EXTRA_MAX];        /* registers 041 and up */
    uint8_t extra_count;
    tvm_settings_t tvm;
    stats_t stats;
    base_state_t base;

    /* Display and DATE settings */
    bool date_as_string;        /* ".." mode: show objects as RFC 3339 strings */
    bool decimal_comma;         /* numeric display: 1.234,56 instead of 1,234.56 */
    uint8_t epoch;              /* EPOCH_* */
    dt_obj_t custom_epoch;      /* a Date */
    int8_t app_tz;              /* application time zone, quarter hours */
    int8_t clock_tz;            /* time zone of the hardware clock */
    bool lift;                  /* stack lift enabled */
    bool complex_results;
    uint8_t complex_mode;
    bool degrees;               /* angle mode: true = DEG, false = RAD */
    uint8_t disp_style;         /* DISP_FIX, DISP_SCI, DISP_ENG */
    int8_t disp_digits;         /* 0-11, or DISP_FLOAT */

    bool editing;               /* X is being typed in */
    char entry[ENTRY_MAX + 1];  /* text being typed, plain ASCII */
    uint8_t entry_len;

    calc_cmd_t last_cmd;        /* previous command, for CLEAR CLEAR */

    const char *message;        /* text to show, or NULL */
    uint8_t message_kind;       /* message_kind_t of a non-NULL message */
    char show_buf[VALUE_FORMAT_MAX]; /* SHOW also needs room for raw DATE records */
} calc_t;

void calc_init(calc_t *c);

/* Shows text (NULL for none) with the way it should be treated. */
void calc_set_message(calc_t *c, message_kind_t kind, const char *text);
void calc_execute(calc_t *c, calc_cmd_t cmd, uint8_t arg);

/* Seeds the random generator (call once at startup, e.g. from the clock). */
void calc_seed_random(uint32_t seed);

/* Read/write complete values. Statistics registers accept plain reals only. */
bool calc_register_read(const calc_t *c, uint8_t num, value_t *out);
bool calc_register_write(calc_t *c, uint8_t num, const value_t *v);

/* Ends number entry, if any, so the state can be saved. */
void calc_finish_entry(calc_t *c);

/* Read the hardware clock using CTZ, converted to ATZ or UTC. Does not
 * change calculator state or write the hardware clock. */
bool calc_clock_now(const calc_t *c, bool utc, dt_obj_t *out);

#endif
