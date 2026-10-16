/*
 * RPNCE: an RPN calculator for the TI-84 Plus CE.
 * Design and behavior inspired by RPN83P by Brian T. Park.
 */

#include <stdbool.h>
#include <keypadc.h>
#include <ti/flags.h>
#include <ti/getcsc.h>

#include "core/calc.h"
#include "ui/actions.h"
#include "ui/display.h"
#include "ui/keys.h"
#include "ui/menu.h"

static calc_t calc;
static keys_t keys;
static menu_t menu;

/* Runs the item under a softkey, or opens its submenu. */
static void press_softkey(uint8_t slot)
{
    const menu_node_t *n = menu_press(&menu, slot);
    if (!n)
        return;

    if (n->id == MID_CLEAR_DISPLAY) {
        calc.message = NULL;
        return;
    }

    calc_cmd_t cmd = action_for((menu_id_t)n->id);
    if (cmd != CMD_NONE)
        calc_execute(&calc, cmd, 0);
    else
        calc.message = "Not available yet";
}

int main(void)
{
    bool on_was_down = false;

    calc_init(&calc);
    menu_home(&menu);
    kb_DisableOnLatch();
    display_open();
    display_draw(&calc, &menu, false);

    for (;;) {
        event_t ev = { EV_NONE, CMD_NONE, 0 };

        /* ON is not in the normal key scan; watch it separately as EXIT. */
        bool on_down = kb_On;
        if (on_down && !on_was_down) {
            ev.kind = EV_BACK;
            os_ResetFlag(ON, INTERRUPT);
        }
        on_was_down = on_down;

        if (ev.kind == EV_NONE) {
            sk_key_t key = os_GetCSC();
            if (!key)
                continue;
            ev = keys_translate(&keys, key);
            if (key == sk_2nd) {
                display_draw(&calc, &menu, keys.second);
                continue;
            }
        }

        if (ev.kind == EV_QUIT)
            break;

        calc.message = NULL;
        switch (ev.kind) {
            case EV_CMD:      calc_execute(&calc, ev.cmd, ev.arg); break;
            case EV_SOFTKEY:  press_softkey(ev.arg); break;
            case EV_ROW_UP:   menu_row_up(&menu); break;
            case EV_ROW_DOWN: menu_row_down(&menu); break;
            case EV_HOME:     menu_home(&menu); break;
            case EV_BACK:     menu_back(&menu); break;
            default:          break;
        }

        display_draw(&calc, &menu, keys.second);
    }

    os_ResetFlag(ON, INTERRUPT);
    display_close();
    return 0;
}
