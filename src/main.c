/*
 * RPNCE: an RPN calculator for the TI-84 Plus CE.
 */

#include <stdbool.h>
#include <time.h>
#include <sys/timers.h>
#include <keypadc.h>
#include <sys/rtc.h>
#include <ti/flags.h>
#include <ti/getcsc.h>

#include "core/calc.h"
#include "sys/storage.h"
#include "ui/actions.h"
#include "ui/catalog.h"
#include "ui/res.h"
#include "core/version.h"
#include "ui/display.h"
#include "ui/keys.h"
#include "ui/menu.h"

/* The OS can't auto power-down while a program waits for keys, so save and
 * exit after this long without a key press; the OS then powers off as usual. */
#define IDLE_TIMEOUT (5UL * 60UL * CLOCKS_PER_SEC)

/* How long a finished STO/RCL/FIX prompt stays on screen. */
#define PROMPT_FLASH_MS 300

static calc_t calc;
static keys_t keys;
static menu_t menu;
static catalog_t catalog;
static uint8_t tint;            /* screen color preset */

/* Runs the item under a softkey, or opens its submenu. As in RPN83P, the
 * menu stays where it is, so a function can be pressed again. */
static void press_softkey(uint8_t slot, bool shifted)
{
    const menu_node_t *n = menu_press(&menu, slot);
    if (!n)
        return;

    switch (n->id) {
        case MID_CLEAR_DISPLAY: return;
        case MID_FIX: keys_open_prompt(&keys, PROMPT_FIX); return;
        case MID_SCI: keys_open_prompt(&keys, PROMPT_SCI); return;
        case MID_ENG: keys_open_prompt(&keys, PROMPT_ENG); return;
        case MID_STACK_EXCHANGE_REG: keys_open_prompt(&keys, PROMPT_XCH); return;
        case MID_SCREEN_TINT:                   /* next color preset */
            tint = (uint8_t)((tint + 1) % TINT_COUNT);
            display_set_tint(tint);
            calc.message = display_tint_name(tint);
            return;
        case MID_ROUND_TO_N: keys_open_prompt(&keys, PROMPT_RNDN); return;
        case MID_ROUND_TO_GUARD: keys_open_prompt(&keys, PROMPT_RNDG); return;
        default: break;
    }

    uint8_t arg;
    calc_cmd_t cmd = action_for((menu_id_t)n->id, shifted, &arg);
    if (cmd != CMD_NONE)
        calc_execute(&calc, cmd, arg);
    else
        calc.message = "Not available yet";
}

/* Keys while the VARS catalog is open. */
static void catalog_key(sk_key_t key)
{
    switch (key) {
        case sk_Up:   catalog_move(&catalog, -1); break;
        case sk_Down: catalog_move(&catalog, 1); break;
        case sk_Enter: {                        /* copy the value to X */
            uint8_t item = catalog.items[catalog.sel];
            catalog.open = false;
            calc.message = NULL;
            if (item == CAT_LASTX)
                calc_execute(&calc, CMD_LASTX, 0);
            else
                calc_execute(&calc, CMD_RCL, item);
            break;
        }
        case sk_Clear: case sk_Vars: case sk_Del:
            catalog.open = false;
            calc.message = NULL;
            break;
        default:
            break;
    }
}

static void redraw(void)
{
    /* A STO/RCL prompt ends number entry, so the typed value shows on the
     * stack (and in Y while an RCL prompt previews the lift). */
    if (keys_prompt_text(&keys) && calc.editing)
        calc_finish_entry(&calc);

    /* Preview the lift only if RCL will really lift (not right after ENTER). */
    display_draw(&calc, &menu, keys.second, keys.alpha, keys_prompt_text(&keys),
                 keys_prompt_lifts(&keys) && calc.lift, &catalog);
}

int main(void)
{
    bool on_was_down = false;
    clock_t last_key = clock();
    uint8_t shown_minute = rtc_Minutes;

    /* The menu and unit tables live in the RPNCEres appvar. Without the
     * right one there's nothing to run, so say what's needed and stop. */
    if (!res_open()) {
        display_open();
        display_problem(res_problem(),
                        "Send RPNCEres.8xv with RPNCE " RPNCE_VERSION ".");
        while (!os_GetCSC())
            ;
        display_close();
        return 1;
    }

    calc_init(&calc);
    menu_home(&menu);
    calc_seed_random(rtc_Time());   /* different RAND results each run */
    storage_load(&calc);
    kb_DisableOnLatch();
    display_open();
    tint = storage_load_settings(&calc);
    if (tint >= TINT_COUNT)
        tint = 0;
    display_set_tint(tint);
    redraw();

    for (;;) {
        event_t ev = { EV_NONE, CMD_NONE, 0, false };

        /* ON is not in the normal key scan; watch it separately.
         * ON = EXIT (cancel prompt, back out of menus); 2ND ON = OFF. */
        bool on_down = kb_On;
        if (on_down && !on_was_down) {
            os_ResetFlag(ON, INTERRUPT);
            if (catalog.open) {
                catalog.open = false;
                calc.message = NULL;
                ev.kind = EV_REDRAW;
            } else if (keys.second) {
                ev.kind = EV_QUIT;
            } else if (keys_cancel_prompt(&keys)) {
                ev.kind = EV_REDRAW;
            } else {
                ev.kind = EV_BACK;
            }
            keys.second = false;
        }
        on_was_down = on_down;

        if (ev.kind == EV_NONE) {
            sk_key_t key = os_GetCSC();
            if (!key) {
                /* keep the status bar's clock current */
                if (rtc_Minutes != shown_minute) {
                    shown_minute = rtc_Minutes;
                    redraw();
                }
                if (clock() - last_key > IDLE_TIMEOUT)
                    break;
                continue;
            }
            if (catalog.open) {
                last_key = clock();
                catalog_key(key);
                redraw();
                continue;
            }
            ev = keys_translate(&keys, key);

            /* A finished prompt shows its answer for a moment ("STO A")
             * before the command runs. */
            bool flash_lifts;
            const char *flash = keys_take_flash(&keys, &flash_lifts);
            if (flash) {
                display_draw(&calc, &menu, false, false, flash,
                             flash_lifts && calc.lift, &catalog);
                delay(PROMPT_FLASH_MS);
            }
        }
        last_key = clock();

        if (ev.kind == EV_QUIT)
            break;

        if (ev.kind != EV_REDRAW)
            calc.message = NULL;

        /* a long X value starts at its right end again once X can change */
        if (ev.kind != EV_SCROLL_LEFT && ev.kind != EV_SCROLL_RIGHT &&
            ev.kind != EV_REDRAW && ev.kind != EV_NONE)
            display_reset_scroll();

        switch (ev.kind) {
            case EV_CMD:      calc_execute(&calc, ev.cmd, ev.arg); break;
            case EV_SOFTKEY:  press_softkey(ev.arg, ev.shifted); break;
            case EV_SCROLL_LEFT:  display_scroll_x(1); break;
            case EV_SCROLL_RIGHT: display_scroll_x(-1); break;
            case EV_ROW_UP:   menu_row_up(&menu); break;
            case EV_ROW_DOWN: menu_row_down(&menu); break;
            case EV_MENU:     menu_jump(&menu, ev.arg); break;
            case EV_VARS:
                calc_finish_entry(&calc);
                if (catalog_open(&catalog, &calc))
                    calc.message = "VARS";
                else
                    calc.message = "All Vars = 0";
                break;
            case EV_BACK:     menu_back(&menu); break;
            default:          break;
        }

        redraw();
    }

    /* Hand the screen back to the OS before writing to archive: archiving
     * can trigger a garbage collection, whose prompt needs the OS screen
     * (and which moves the RPNCEres data we no longer need by then). */
    os_ResetFlag(ON, INTERRUPT);
    display_close();
    storage_save(&calc);
    storage_save_settings(tint, &calc);
    return 0;
}
