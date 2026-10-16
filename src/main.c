/*
 * RPNCE: an RPN calculator for the TI-84 Plus CE.
 * Design and behavior inspired by RPN83P by Brian T. Park.
 */

#include <ti/getcsc.h>

#include "core/calc.h"
#include "ui/display.h"
#include "ui/keys.h"

static calc_t calc;
static keys_t keys;

int main(void)
{
    calc_init(&calc);
    display_open();
    display_draw(&calc, false);

    while (!keys.quit) {
        sk_key_t key = os_GetCSC();
        if (!key)
            continue;

        uint8_t arg;
        calc_cmd_t cmd = keys_translate(&keys, key, &arg);
        if (cmd != CMD_NONE)
            calc_execute(&calc, cmd, arg);
        else if (key != sk_2nd)
            calc.message = NULL;

        display_draw(&calc, keys.second);
    }

    display_close();
    return 0;
}
