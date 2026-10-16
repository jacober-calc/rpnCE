#ifndef RPNCE_CORE_CALC_H
#define RPNCE_CORE_CALC_H

/*
 * Calculator state and commands. This module knows nothing about the
 * screen or keypad: the UI turns keys into commands and reads state back.
 */

#include <stdbool.h>
#include <stdint.h>
#include <ti/real.h>

#define ENTRY_MAX 20

typedef enum {
    CMD_NONE,
    /* number entry */
    CMD_DIGIT,          /* arg = 0..9 */
    CMD_DOT,
    CMD_EE,
    CMD_CHS,
    CMD_BACKSPACE,
    CMD_CLEAR,
    /* stack */
    CMD_ENTER,
    CMD_SWAP,           /* X<>Y */
    CMD_ROLL_DOWN,      /* R-down */
    CMD_LASTX,
    CMD_SHOW,           /* show X with all 14 digits */
    /* two-argument math: Y op X */
    CMD_ADD, CMD_SUB, CMD_MUL, CMD_DIV, CMD_POW,
    /* one-argument math */
    CMD_SQUARE, CMD_SQRT, CMD_RECIP,
    CMD_LOG, CMD_ALOG, CMD_LN, CMD_EXP,
    CMD_SIN, CMD_COS, CMD_TAN, CMD_ASIN, CMD_ACOS, CMD_ATAN,
    /* constants */
    CMD_PI, CMD_E,
} calc_cmd_t;

typedef struct {
    real_t x, y, z, t;
    real_t lastx;
    bool lift;                  /* stack lift enabled */

    bool editing;               /* X is being typed in */
    char entry[ENTRY_MAX + 1];  /* text being typed, plain ASCII */
    uint8_t entry_len;

    const char *message;        /* error or info text, or NULL */
    char show_buf[24];          /* backing store for CMD_SHOW */
} calc_t;

void calc_init(calc_t *c);
void calc_execute(calc_t *c, calc_cmd_t cmd, uint8_t arg);

#endif
