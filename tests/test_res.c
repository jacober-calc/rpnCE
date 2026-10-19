/* Tests the RPNCEres appvar: the real src/ui/res.c reading the real
 * data/RPNCEres.bin, checked against the menu file and the unit table.
 *   cc -Itests/mock_fileioc -Itests/mock -Isrc/ui -Isrc/core -o test_res \
 *      tests/test_res.c src/ui/res.c && ./test_res
 * (run from the project root, after python3 tools/genres.py)
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "res.h"
#include "res_info.h"
#include "units.h"
#include "version.h"

uint8_t mock_res[65536];
size_t mock_res_size;
static int fails = 0;
static void ok(const char *name, int cond, const char *got)
{
    if (!cond) fails++;
    printf("%s %-50s %s\n", cond ? "ok  " : "FAIL", name, got ? got : "");
}

/* find a menu item by ID */
static const menu_node_t *find(menu_id_t id)
{
    for (uint16_t i = 0; i < MENU_NODE_COUNT; i++)
        if (menu_node(i)->id == id) return menu_node(i);
    return NULL;
}

int main(void)
{
    ok("struct sizes match the file layout",
       sizeof(menu_node_t) == 8 && sizeof(menu_group_t) == 6, NULL);

    bool none = !res_open();
    ok("missing RPNCEres is reported", none && strcmp(res_problem(), "RPNCEres not found") == 0, res_problem());

    FILE *f = fopen("data/RPNCEres.bin", "rb");
    mock_res_size = fread(mock_res, 1, sizeof mock_res, f);
    fclose(f);
    bool opened = res_open();
    ok("RPNCEres opens and matches this build", opened, opened ? "" : res_problem());

    ok("HOME is menu 0 with 3 rows", strcmp(res_text(menu_group(MENU_ROOT_HOME)->label), "HOME") == 0 &&
       menu_group(MENU_ROOT_HOME)->rows == 3, NULL);
    const menu_node_t *n = menu_node(menu_group(MENU_ROOT_HOME)->first);
    ok("HOME's first key is MATH (a submenu)", strcmp(res_text(n->label), "MATH") == 0 && n->kind == MENU_GROUP, res_text(n->label));
    ok("MODE menu's parent is HOME", menu_parent_of(MENU_GROUP_MODE) == MENU_ROOT_HOME, NULL);
    n = find(MID_DATE_CREATE);
    ok("DATE > D starts with D{}", n && strcmp(res_text(n->label), "D{}") == 0, n ? res_text(n->label) : "");
    n = find(MID_FORMAT_RECORD_STRING);
    ok("\"..\" is a mode item with its marked label", n && n->kind == MENU_ALT &&
       strcmp(res_text(n->alt_label), "\"..\"\x07") == 0, NULL);
    n = find(MID_CUBE);
    ok("X^3 is labeled as in the menu file", n && strcmp(res_text(n->label), "X^3") == 0, n ? res_text(n->label) : "");

    /* every unit item maps to a unit whose label matches the menu label */
    int units = 0, good = 1;
    for (uint16_t i = 0; i < MENU_NODE_COUNT; i++) {
        const menu_node_t *m = menu_node(i);
        int u = unit_for_menu((menu_id_t)m->id);
        if (u < 0) continue;
        units++;
        if (strcmp(unit_label((uint8_t)u), res_text(m->label)) != 0) good = 0;
    }
    char msg[40]; sprintf(msg, "%d unit items", units);
    ok("every UNIT item's label matches its unit", good && units >= UNIT_COUNT, msg);

    int u = unit_for_menu(MID_UNIT_FOOT);
    ok("ft is 0.3048 m", u >= 0 && strcmp(unit_factor((uint8_t)u), "3.0480000000000E-1") == 0, u >= 0 ? unit_factor((uint8_t)u) : "");
    u = unit_for_menu(MID_UNIT_FAHRENHEIT);
    ok("F has an offset (459.67)", u >= 0 && unit_offset((uint8_t)u) && strcmp(unit_offset((uint8_t)u), "4.5967000000000E2") == 0, NULL);
    u = unit_for_menu(MID_UNIT_MILES_PER_GALLON);
    ok("mpg is a reciprocal unit", u >= 0 && unit_reciprocal((uint8_t)u), NULL);
    ok("the base unit of length is m", strcmp(unit_label(unit_base_of(DIM_LENGTH)), "m") == 0, unit_label(unit_base_of(DIM_LENGTH)));
    ok("CUBE is not a unit", unit_for_menu(MID_CUBE) < 0, NULL);

    /* HELP pages, from help/help.txt */
    ok("19 HELP pages", res_help_pages() == 19, NULL);
    ok("page 1 explains HELP", strcmp(res_help_line(0, 0),
       "HELP 01/19 - USING HELP / MENUS") == 0, res_help_line(0, 0));
    const char *topics[] = { "THE RPN STACK", "VARIABLES", "KEY SHORTCUTS",
        "MATH", "NUM", "PROB", "CPLX", "TVM", "BASE", "HYP", "STAT",
        "CONV", "CLR", "MODE", "STK", "UNIT", "DATE", "ABOUT" };
    bool complete = true, numbered = true, fits = true, shortcuts = false;
    for (uint8_t p = 0; p < res_help_pages(); p++) {
        char prefix[24];
        snprintf(prefix, sizeof prefix, "HELP %02u/%u - ", p + 1, res_help_pages());
        if (strncmp(res_help_line(p, 0), prefix, strlen(prefix))) numbered = false;
        if (res_help_lines(p) < 1 || res_help_lines(p) > 8) fits = false;
        for (uint8_t l = 0; l < res_help_lines(p); l++) {
            const char *s = res_help_line(p, l);
            if (strlen(s) > 44) fits = false;
            for (; *s; s++)
                if ((unsigned char)*s < 32 || (unsigned char)*s > 126) fits = false;
        }
        if (strstr(res_help_line(p, 0), "KEY SHORTCUTS")) {
            shortcuts = true;
            for (uint8_t l = 0; l < res_help_lines(p); l++)
                if (strstr(res_help_line(p, l), "LINK") || strstr(res_help_line(p, l), "ANGLE"))
                    shortcuts = false;
        }
    }
    for (size_t t = 0; t < sizeof topics / sizeof topics[0]; t++) {
        bool found = false;
        for (uint8_t p = 0; p < res_help_pages(); p++) {
            const char *topic = strstr(res_help_line(p, 0), " - ");
            if (topic && strcmp(topic + 3, topics[t]) == 0) found = true;
        }
        if (!found) complete = false;
    }
    ok("every main menu and usage topic has a HELP page", complete, NULL);
    ok("HELP titles show their page number and total", numbered, NULL);
    ok("shortcuts omit reserved LINK and ANGLE keys", shortcuts, NULL);
    ok("every page fits (8 lines, 44 ASCII chars)", fits, NULL);
    ok("ABOUT has the current version and repository", ({
        bool version = false, repo = false;
        uint8_t p = res_help_pages() - 1;
        for (uint8_t l = 0; l < res_help_lines(p); l++) {
            if (strcmp(res_help_line(p, l), RPNCE_NAME " " RPNCE_VERSION) == 0) version = true;
            if (strcmp(res_help_line(p, l), "https://github.com/jacober-calc/rpnce") == 0) repo = true;
        }
        version && repo; }), NULL);
    ok("past the last line or page is empty, not garbage",
       res_help_line(0, 99)[0] == '\0' && res_help_lines(99) == 0, NULL);

    /* a changed file is caught */
    mock_res[RES_SIZE - 1] ^= 1;
    mock_res[12] ^= 1;
    bool rejected = !res_open();
    ok("a different RPNCEres is rejected", rejected && strcmp(res_problem(), "RPNCEres is the wrong version") == 0, res_problem());

    printf("%d failures\n", fails);
    return fails != 0;
}
