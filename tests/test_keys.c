/* Translate actual CE scan codes, preserving modifiers and register prompts. */
#include <stdio.h>
#include <string.h>
#include "keys.h"
#include "menu.h"
#include "res.h"

uint8_t mock_res[65536];
size_t mock_res_size;
static int fails;
static void check(const char *name, bool pass)
{
    if (!pass) fails++;
    printf("%s %s\n", pass ? "ok  " : "FAIL", name);
}
static void all_menu_slots(void)
{
    unsigned checked = 0;
    bool actions = true, submenus = true, blanks = true;
    for (uint8_t g = 0; g < MENU_GROUP_COUNT; g++) {
        for (uint8_t row = 0; row < menu_group(g)->rows; row++) {
            for (uint8_t slot = 0; slot < 5; slot++) {
                menu_t m = {0};
                menu_jump(&m, g);
                m.row = row;
                menu_t before = m;
                const menu_node_t *n = menu_slot(&m, slot);
                const menu_node_t *selected = menu_press(&m, slot);
                checked++;
                if (menu_node_kind(n) == MENU_GROUP) {
                    if (selected || m.group != n->group || m.row != 0) submenus = false;
                    menu_back(&m);
                    if (m.group != before.group || m.row != before.row || m.depth != before.depth)
                        submenus = false;
                } else if (menu_node_kind(n) == MENU_BLANK) {
                    if (selected || memcmp(&m, &before, sizeof m)) blanks = false;
                } else if (selected != n || memcmp(&m, &before, sizeof m)) actions = false;
            }
        }
    }
    check("all menu slots are exercised", checked == MENU_NODE_COUNT);
    check("every submenu opens and ON returns to its original row", submenus);
    check("every action returns its node without changing menus", actions);
    check("every blank is inert", blanks);
}
int main(void)
{
    FILE *f = fopen("data/rpnCEres.bin", "rb");
    if (!f) return 1;
    mock_res_size = fread(mock_res, 1, sizeof mock_res, f);
    fclose(f);
    if (!res_open()) return 1;
    static const struct { sk_key_t key; uint8_t group; bool second; } cases[] = {
        { sk_Math, MENU_GROUP_HOME, false }, { sk_Mode, MENU_GROUP_MODE, false },
        { sk_Stat, MENU_GROUP_STAT, false }, { sk_GraphVar, MENU_GROUP_UNIT, false },
        { sk_Apps, MENU_GROUP_BASE, false }, { sk_Prgm, MENU_GROUP_HYPERBOLIC, false },
        { sk_Add, MENU_GROUP_CLEAR, true }, { sk_Stat, MENU_GROUP_STACK, true },
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        keys_t k = {0};
        menu_t m;
        menu_home(&m);
        if (cases[i].second) keys_translate(&k, sk_2nd);
        event_t e = keys_translate(&k, cases[i].key);
        check("key selects requested menu and consumes 2ND", e.kind == EV_MENU &&
              e.arg == cases[i].group && !k.second);
        menu_jump(&m, e.arg);
        check("shortcut opens first row of menu", m.group == cases[i].group && m.row == 0);
        if (m.group != MENU_GROUP_HOME) {
            menu_back(&m);
            check("ON returns from shortcut to correct HOME row", m.group == MENU_GROUP_HOME &&
                  m.row == menu_parent_row_of(e.arg));
        }
    }
    {
        keys_t k2 = {0};
        keys_translate(&k2, sk_2nd);
        event_t link = keys_translate(&k2, sk_GraphVar);
        check("2ND LINK combines or splits a value", link.kind == EV_CMD && link.cmd == CMD_COMPLEX_LINK && !k2.second);
        keys_translate(&k2, sk_2nd);
        event_t angle = keys_translate(&k2, sk_Apps);
        check("2ND ANGLE enters polar values", angle.kind == EV_CMD && angle.cmd == CMD_COMPLEX_POLAR_ENTRY && !k2.second);
        keys_translate(&k2, sk_2nd);
        event_t imag = keys_translate(&k2, sk_DecPnt);
        check("2ND i enters rectangular values", imag.kind == EV_CMD && imag.cmd == CMD_COMPLEX_RECT_ENTRY && !k2.second);
    }
    keys_t k = {0};
    {
        bool brighter = false;
        check("plain UP is not a brightness shortcut", !keys_brightness_key(&k, sk_Up, &brighter));
        keys_translate(&k, sk_2nd);
        check("2ND UP brightens and consumes 2ND",
              keys_brightness_key(&k, sk_Up, &brighter) && brighter && !k.second);
        keys_translate(&k, sk_2nd);
        check("2ND DOWN dims and consumes 2ND",
              keys_brightness_key(&k, sk_Down, &brighter) && !brighter && !k.second);
        keys_open_prompt(&k, PROMPT_STO);
        keys_translate(&k, sk_Add);
        keys_translate(&k, sk_2nd);
        check("brightness works inside STO+ without losing the prompt",
              keys_brightness_key(&k, sk_Up, &brighter) && brighter &&
              k.prompt == PROMPT_STO && k.prompt_op == 1 && !k.prompt_second);
        event_t stored = keys_translate(&k, sk_Math);
        check("STO+ still completes after brightness adjustment",
              stored.kind == EV_CMD && stored.cmd == CMD_STO_ADD && stored.arg == REG_A);
        keys_open_prompt(&k, PROMPT_FIX);
        keys_translate(&k, sk_DecPnt);
        keys_translate(&k, sk_2nd);
        check("brightness works inside FIX . without losing its precision prefix",
              keys_brightness_key(&k, sk_Down, &brighter) && !brighter &&
              k.prompt == PROMPT_FIX && k.prompt_count == 1);
        event_t fixed = keys_translate(&k, sk_1);
        check("FIX 11 completes after brightness adjustment",
              fixed.kind == EV_CMD && fixed.cmd == CMD_DISP_FIX && fixed.arg == 11);
        k = (keys_t){0};
        keys_translate(&k, sk_2nd);
        check("other shifted keys remain pending for their own handler",
              !keys_brightness_key(&k, sk_Mode, &brighter) && k.second);
        k = (keys_t){0};
    }
    keys_translate(&k, sk_2nd);
    check("2ND CATALOG opens HELP", keys_translate(&k, sk_0).kind == EV_HELP && !k.second);
    keys_translate(&k, sk_2nd);
    check("2ND QUIT still quits", keys_translate(&k, sk_Mode).kind == EV_QUIT);
    keys_translate(&k, sk_2nd);
    event_t e = keys_translate(&k, sk_Graph);
    check("shifted softkeys still request TVM recall", e.kind == EV_SOFTKEY && e.arg == 4 && e.shifted);
    keys_translate(&k, sk_Alpha); e = keys_translate(&k, sk_Apps);
    check("ALPHA APPS is still B", e.kind == EV_CMD && e.cmd == CMD_ALPHA_LETTER && e.arg == 1);
    keys_translate(&k, sk_Alpha); e = keys_translate(&k, sk_Prgm);
    check("ALPHA PRGM is still C", e.kind == EV_CMD && e.cmd == CMD_ALPHA_LETTER && e.arg == 2);
    keys_open_prompt(&k, PROMPT_STO); e = keys_translate(&k, sk_Apps);
    check("STO APPS still stores B", e.kind == EV_CMD && e.cmd == CMD_STO && e.arg == 2);
    keys_open_prompt(&k, PROMPT_RCL); e = keys_translate(&k, sk_Prgm);
    check("RCL PRGM still recalls C", e.kind == EV_CMD && e.cmd == CMD_RCL && e.arg == 3);
    keys_open_prompt(&k, PROMPT_STO); keys_translate(&k, sk_Add); e = keys_translate(&k, sk_Math);
    check("STO arithmetic still works", e.kind == EV_CMD && e.cmd == CMD_STO_ADD && e.arg == 1);
    menu_t m;
    menu_home(&m);
    const menu_node_t *n = menu_slot(&m, 4);
    check("TVM occupies HOME first row fifth slot", menu_node_kind(n) == MENU_GROUP && n->group == MENU_GROUP_TVM);
    menu_press(&m, 4);
    check("TVM submenu still opens", m.group == MENU_GROUP_TVM && menu_slot(&m, 0)->id == MID_TVM_N);
    menu_back(&m); menu_row_down(&m); n = menu_slot(&m, 4);
    check("HELP occupies HOME second row fifth slot", menu_node_kind(n) == MENU_ITEM && n->id == MID_HELP);
    all_menu_slots();
    printf("%d failures\n", fails);
    return fails != 0;
}
