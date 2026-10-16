#ifndef RPNCE_UI_KEYS_H
#define RPNCE_UI_KEYS_H

#include <stdbool.h>
#include <ti/getcsc.h>
#include "../core/calc.h"

typedef enum {
    EV_NONE,
    EV_CMD,         /* run cmd / arg */
    EV_SOFTKEY,     /* arg = 0..4 for Y= WINDOW ZOOM TRACE GRAPH */
    EV_ROW_UP,
    EV_ROW_DOWN,
    EV_HOME,        /* MATH */
    EV_BACK,        /* ON */
    EV_QUIT,        /* 2ND QUIT */
} event_kind_t;

typedef struct {
    event_kind_t kind;
    calc_cmd_t cmd;
    uint8_t arg;
} event_t;

typedef struct {
    bool second;    /* 2ND is pending */
} keys_t;

/* Turns a scan code into an event. */
event_t keys_translate(keys_t *k, sk_key_t key);

#endif
