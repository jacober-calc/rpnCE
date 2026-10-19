#ifndef RPNCE_UI_KEYS_H
#define RPNCE_UI_KEYS_H

#include <stdbool.h>
#include <ti/getcsc.h>
#include "../core/calc.h"

typedef enum {
    EV_NONE,
    EV_REDRAW,      /* only the screen needs updating (2ND, prompts) */
    EV_CMD,         /* run cmd / arg */
    EV_SOFTKEY,     /* arg = 0..4 for Y= WINDOW ZOOM TRACE GRAPH */
    EV_ROW_UP,
    EV_ROW_DOWN,
    EV_MENU,        /* go to menu arg (MENU_GROUP_*): MATH = HOME, MODE */
    EV_BACK,        /* ON */
    EV_QUIT,        /* 2ND QUIT or 2ND OFF */
    EV_HELP,        /* quick reference: HELP or 2ND CATALOG */
    EV_VARS,        /* VARS: the register catalog */
    EV_SCROLL_LEFT, /* left / right arrows: scroll a long X value */
    EV_SCROLL_RIGHT,
} event_kind_t;

typedef struct {
    event_kind_t kind;
    calc_cmd_t cmd;
    uint8_t arg;
    bool shifted;   /* EV_SOFTKEY pressed after 2ND */
} event_t;

typedef enum {
    PROMPT_NONE,
    PROMPT_STO, PROMPT_RCL, PROMPT_XCH,     /* wait for a register */
    PROMPT_FIX, PROMPT_SCI, PROMPT_ENG,     /* wait for two digits */
    PROMPT_RNDN, PROMPT_RNDG,
} prompt_kind_t;

typedef struct {
    bool second;            /* 2ND is pending */
    prompt_kind_t prompt;   /* waiting for a letter or digit */
    uint8_t prompt_op;      /* 0 none, 1 +, 2 -, 3 x, 4 / */
    bool prompt_alpha;      /* ALPHA pressed: next key is a letter */
    bool prompt_second;     /* 2ND pressed: . means (i) */
    uint8_t prompt_count;   /* FIX/SCI/ENG/RND: 1 after the . key (10-19) */
    char prompt_text[12];
    bool alpha;             /* ALPHA pending: next key is its green letter */
    char flash_text[12];    /* a finished prompt, shown briefly ("STO A") */
    bool flash;
    bool flash_lifts;       /* ... over a lifted stack (plain RCL) */
} keys_t;

/* Turns a scan code into an event. */
event_t keys_translate(keys_t *k, sk_key_t key);

/* Opens a prompt (used by menu items such as FIX). */
void keys_open_prompt(keys_t *k, prompt_kind_t kind);

/* Cancels a prompt. Returns true if one was open. */
bool keys_cancel_prompt(keys_t *k);

/* Text for an open prompt, or NULL. */
const char *keys_prompt_text(const keys_t *k);

/* True while the prompt is a plain RCL, which lifts the stack. */
bool keys_prompt_lifts(const keys_t *k);

/* A prompt just completed: its final text ("STO A", "FIX 04") to show for a
 * moment before the command runs, or NULL. Clears it. */
const char *keys_take_flash(keys_t *k, bool *lifts);

#endif
