/*
 * rpnCE: an RPN calculator for the TI-84 Plus CE.
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
#include "sys/brightness.h"
#include "ui/actions.h"
#include "ui/catalog.h"
#include "ui/res.h"
#include "core/version.h"
#include "ui/display.h"
#include "ui/keys.h"
#include "ui/menu.h"

/* The OS can't auto power-down while a program waits for keys, so save and
 * exit after this long without a key press; the OS then powers off as usual. */
#define IDLE_TIMEOUT ((clock_t)(5L * 60L * CLOCKS_PER_SEC))

/* How long a finished STO/RCL/FIX prompt stays on screen. */
#define PROMPT_FLASH_MS 150
#define NOTICE_TIMEOUT ((clock_t)(3L * CLOCKS_PER_SEC / 2L))

static calc_t calc;
static keys_t keys;
static menu_t menu;
static catalog_t catalog;
static uint8_t help_page;       /* 0: closed, 1..res_help_pages(): reference */
static uint8_t tint;            /* screen color preset */
static const char *notice;
static clock_t notice_started;

/* Runs the item under a softkey, or opens its submenu. As in RPN83P, the
 * menu stays where it is, so a function can be pressed again. Items with
 * no calculator command are submenus or are handled in the switch below. */
static void press_softkey(uint8_t slot, bool shifted)
{
    const menu_node_t *n = menu_press(&menu, slot);
    if (!n)
        return;

    switch (n->id) {
        case MID_HELP: help_page = 1; keys.second = keys.alpha = false; return;
        case MID_CLEAR_DISPLAY: return;
        case MID_FIX: keys_open_prompt(&keys, PROMPT_FIX); return;
        case MID_SCI: keys_open_prompt(&keys, PROMPT_SCI); return;
        case MID_ENG: keys_open_prompt(&keys, PROMPT_ENG); return;
        case MID_STACK_EXCHANGE_REG: keys_open_prompt(&keys, PROMPT_XCH); return;
        case MID_SCREEN_TINT:                   /* next color preset */
            tint = (uint8_t)((tint + 1) % TINT_COUNT);
            display_set_tint(tint);
            calc_set_message(&calc, MSG_NOTICE, display_tint_name(tint));
            return;
        case MID_ROUND_TO_N: keys_open_prompt(&keys, PROMPT_RNDN); return;
        case MID_ROUND_TO_GUARD: keys_open_prompt(&keys, PROMPT_RNDG); return;
        default: break;
    }

    uint8_t arg;
    calc_cmd_t cmd = action_for((menu_id_t)n->id, shifted, &arg);
    if (cmd != CMD_NONE)
        calc_execute(&calc, cmd, arg);
}

/* ---------- screens and input ----------
 * HELP and the VARS catalog take over the keyboard while open. Otherwise a
 * key first acknowledges any message, then keys_translate() turns it into
 * an event for the calculator, the menus or the display. */

static void close_catalog(void)
{
    catalog.open = false;
    calc.message = NULL;
}

/* Clears the message and restarts notice timing, so a repeated notice gets
 * its full time on screen. */
static void dismiss_message(void)
{
    if (display_has_message(&calc))
        display_dismiss_message(&calc);
    notice = NULL;
}

/* HELP: up/down turn pages, ENTER/CLEAR/DEL/MATH close it, 2ND MODE quits.
 * Returns false to quit. */
static bool help_key(sk_key_t key)
{
    if (key == sk_2nd) {
        keys.second = !keys.second;
        return true;
    }
    if (keys.second && key == sk_Mode)
        return false;
    keys.second = false;
    uint8_t pages = res_help_pages();
    if (key == sk_Up)
        help_page = help_page == 1 ? pages : help_page - 1;
    else if (key == sk_Down)
        help_page = help_page == pages ? 1 : help_page + 1;
    else if (key == sk_Enter || key == sk_Clear || key == sk_Del || key == sk_Math)
        help_page = 0;
    return true;
}

/* VARS: up/down select, ENTER copies the value to X, CLEAR/VARS/DEL close. */
static void catalog_key(sk_key_t key)
{
    if (key == sk_2nd) {                    /* for 2ND up/down brightness */
        keys.second = !keys.second;
        return;
    }
    keys.second = false;
    switch (key) {
        case sk_Up:   catalog_move(&catalog, -1); break;
        case sk_Down: catalog_move(&catalog, 1); break;
        case sk_Enter: {
            uint8_t item = catalog.items[catalog.sel];
            close_catalog();
            catalog_recall(&calc, item);
            break;
        }
        case sk_Clear: case sk_Vars: case sk_Del:
            close_catalog();
            break;
        default:
            break;
    }
}

/* Any key dismisses a message. CLEAR outside a prompt does only that, so
 * acknowledging a message never clears X. Returns true if CLEAR was used. */
static bool acknowledge_message(sk_key_t key)
{
    if (!display_has_message(&calc))
        return false;
    dismiss_message();
    if (key != sk_Clear || keys.prompt != PROMPT_NONE)
        return false;
    keys.second = keys.alpha = false;
    if (calc.last_cmd == CMD_CLEAR)
        calc.last_cmd = CMD_NONE;           /* the next CLEAR is CLX again */
    return true;
}

/* A key on the stack screen, as an event. */
static event_t stack_key(sk_key_t key)
{
    if (acknowledge_message(key)) {
        event_t ev = { EV_REDRAW, CMD_NONE, 0, false };
        return ev;
    }
    event_t ev = keys_translate(&keys, key);

    /* A finished prompt shows its answer for a moment ("STO A") before the
     * command runs. */
    bool flash_lifts;
    const char *flash = keys_take_flash(&keys, &flash_lifts);
    if (flash) {
        display_draw(&calc, &menu, false, false, flash, flash_lifts && calc.lift, &catalog);
        delay(PROMPT_FLASH_MS);
    }
    return ev;
}

/* ON is EXIT: it closes HELP or VARS, cancels a prompt, or backs out of a
 * menu. 2ND ON quits, except in VARS, where it just closes the catalog. */
static event_t on_key(void)
{
    event_t ev = { EV_REDRAW, CMD_NONE, 0, false };
    os_ResetFlag(ON, INTERRUPT);
    if (help_page) {
        if (keys.second)
            ev.kind = EV_QUIT;
        else
            help_page = 0;
    } else if (catalog.open) {
        close_catalog();
    } else if (keys.second) {
        ev.kind = EV_QUIT;
    } else if (!keys_cancel_prompt(&keys)) {
        ev.kind = EV_BACK;
    }
    keys.second = false;
    return ev;
}

/* Carries out an event. Returns false to quit. */
static bool run_event(event_t ev)
{
    if (ev.kind == EV_QUIT)
        return false;
    if (ev.kind != EV_REDRAW)
        dismiss_message();

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
        case EV_HELP:     help_page = 1; keys.alpha = false; break;
        case EV_VARS:
            calc_finish_entry(&calc);
            catalog_open(&catalog, &calc);  /* never empty: TVM and LASTX */
            calc_set_message(&calc, MSG_NOTICE, "VARS");
            break;
        case EV_BACK:     menu_back(&menu); break;
        default:          break;
    }
    return true;
}

/* A key press, routed to the screen that owns the keyboard. 2ND up/down
 * brightness works everywhere. Returns false to quit. */
static bool handle_key(sk_key_t key)
{
    bool brighter;
    if (keys_brightness_key(&keys, key, &brighter)) {
        sys_brightness_adjust(brighter);
        return true;
    }
    if (help_page)
        return help_key(key);
    if (catalog.open) {
        catalog_key(key);
        return true;
    }
    return run_event(stack_key(key));
}

/* Brings derived state up to date after input, before drawing. */
static void settle(void)
{
    /* A STO/RCL prompt ends number entry, so the typed value shows on the
     * stack (and in Y while an RCL prompt previews the lift). */
    if (keys_prompt_text(&keys) && calc.editing)
        calc_finish_entry(&calc);
    /* A new notice starts its timeout. */
    if (!display_message_is_notice(&calc))
        notice = NULL;
    else if (notice != calc.message) {
        notice = calc.message;
        notice_started = clock();
    }
}

/* Draws the screen from the current state, changing nothing. */
static void draw(void)
{
    if (help_page) {
        display_help((uint8_t)(help_page - 1));
        return;
    }
    /* Preview the lift only if RCL will really lift (not right after ENTER). */
    display_draw(&calc, &menu, keys.second, keys.alpha, keys_prompt_text(&keys),
                 keys_prompt_lifts(&keys) && calc.lift, &catalog);
}

/* Between key presses: time out a notice and keep the status bar's clock
 * current. Returns false once the calculator has been idle long enough to
 * save and exit. */
static bool idle(clock_t last_key, uint8_t *shown_minute)
{
    bool stale = false;
    if (notice && clock() - notice_started >= NOTICE_TIMEOUT) {
        calc.message = NULL;
        notice = NULL;
        stale = true;
    }
    if (rtc_Minutes != *shown_minute) {
        *shown_minute = rtc_Minutes;
        stale = true;
    }
    if (stale)
        draw();
    return clock() - last_key <= IDLE_TIMEOUT;
}

int main(void)
{
    /* The menu and unit tables live in the RPNCEres appvar. Without the
     * right one there's nothing to run, so say what's needed and stop. */
    if (!res_open()) {
        display_open();
        display_problem(res_problem(),
                        "Send rpnCEres.8xv with rpnCE " RPNCE_VERSION ".");
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
    settle();
    draw();

    bool on_was_down = false;
    clock_t last_key = clock();
    uint8_t shown_minute = rtc_Minutes;
    for (;;) {
        /* ON is not in the normal key scan; watch it for presses. */
        bool on_down = kb_On;
        bool on_pressed = on_down && !on_was_down;
        on_was_down = on_down;

        sk_key_t key = on_pressed ? 0 : os_GetCSC();
        if (!on_pressed && !key) {
            if (!idle(last_key, &shown_minute))
                break;
            continue;
        }
        last_key = clock();
        if (!(on_pressed ? run_event(on_key()) : handle_key(key)))
            break;
        settle();
        draw();
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
