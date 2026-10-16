#ifndef RPNCE_UI_KEYS_H
#define RPNCE_UI_KEYS_H

#include <stdbool.h>
#include <ti/getcsc.h>
#include "../core/calc.h"

typedef struct {
    bool second;    /* 2ND is pending */
    bool quit;      /* 2ND QUIT was pressed */
} keys_t;

/* Turns a scan code into a calculator command (CMD_NONE if unused). */
calc_cmd_t keys_translate(keys_t *k, sk_key_t key, uint8_t *arg);

#endif
