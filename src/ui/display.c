#include <string.h>
#include <graphx.h>

#include "display.h"
#include "../core/format.h"
#include "../core/version.h"

/* ---------- colors (custom palette entries) ---------- */

enum {
    COL_BG = 1,
    COL_TEXT,
    COL_BAR,
    COL_BAR_TEXT,
    COL_LABEL,
    COL_X_BG,
    COL_ERROR,
    COL_MENU,
    COL_SECOND,
    COL_EDIT,
    COL_RULE,
};

static void set_palette(void)
{
    gfx_palette[COL_BG]       = gfx_RGBTo1555(250, 250, 250);
    gfx_palette[COL_TEXT]     = gfx_RGBTo1555(25, 25, 30);
    gfx_palette[COL_BAR]      = gfx_RGBTo1555(40, 58, 80);
    gfx_palette[COL_BAR_TEXT] = gfx_RGBTo1555(240, 244, 248);
    gfx_palette[COL_LABEL]    = gfx_RGBTo1555(130, 136, 145);
    gfx_palette[COL_X_BG]     = gfx_RGBTo1555(226, 236, 248);
    gfx_palette[COL_ERROR]    = gfx_RGBTo1555(196, 40, 40);
    gfx_palette[COL_MENU]     = gfx_RGBTo1555(214, 220, 228);
    gfx_palette[COL_SECOND]   = gfx_RGBTo1555(245, 170, 0);
    gfx_palette[COL_EDIT]     = gfx_RGBTo1555(30, 100, 200);
    gfx_palette[COL_RULE]     = gfx_RGBTo1555(200, 206, 214);
}

/* ---------- layout (320 x 240) ---------- */

#define SCREEN_W     320
#define BAR_H        20
#define MSG_Y        24
#define STACK_TOP    40
#define ROW_H        40
#define MENU_Y       206
#define MENU_H       34
#define LABEL_X      8
#define VALUE_RIGHT  (SCREEN_W - 8)
#define VALUE_LEFT   36

void display_open(void)
{
    gfx_Begin();
    set_palette();
    gfx_SetDrawBuffer();            /* draw off-screen, then swap */
    gfx_SetTextTransparentColor(255);
    gfx_SetTextBGColor(255);        /* 255 = transparent text background */
}

void display_close(void)
{
    gfx_End();
}

/* Prints text right-aligned at the largest scale (3, then 2, then 1) that
 * fits between VALUE_LEFT and VALUE_RIGHT, vertically centered in a row. */
static void print_value(const char *s, int row_y, uint8_t color)
{
    uint8_t scale;
    unsigned int w;

    /* Measure at scale 1 and multiply, so the math doesn't depend on
     * whether graphx's width already includes the scale. */
    gfx_SetTextScale(1, 1);
    unsigned int base = gfx_GetStringWidth(s);

    for (scale = 3; scale > 1; scale--) {
        w = base * scale;
        if ((int)w <= VALUE_RIGHT - VALUE_LEFT)
            break;
    }
    w = base * scale;

    int x = VALUE_RIGHT - (int)w;
    if (x < 0)
        x = 0;      /* graphx doesn't clip text by default */

    gfx_SetTextScale(scale, scale);
    gfx_SetTextFGColor(color);
    gfx_PrintStringXY(s, x, row_y + (ROW_H - 8 * scale) / 2);
}

static void draw_status_bar(bool second)
{
    gfx_SetColor(COL_BAR);
    gfx_FillRectangle(0, 0, SCREEN_W, BAR_H);

    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(COL_BAR_TEXT);
    gfx_PrintStringXY("RAD", 8, 6);

    if (second) {
        gfx_SetColor(COL_SECOND);
        gfx_FillRectangle(44, 3, 34, 14);
        gfx_SetTextFGColor(COL_TEXT);
        gfx_PrintStringXY("2ND", 50, 6);
    }

    gfx_SetTextFGColor(COL_BAR_TEXT);
    const char *name = RPNCE_NAME " " RPNCE_VERSION;
    gfx_PrintStringXY(name, SCREEN_W - 8 - (int)gfx_GetStringWidth(name), 6);
}

static void draw_message(const char *msg)
{
    if (!msg)
        return;
    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(msg[0] == 'E' && msg[1] == 'r' ? COL_ERROR : COL_TEXT);
    gfx_PrintStringXY(msg, LABEL_X, MSG_Y + 2);
}

static void draw_stack(const calc_t *c)
{
    static const char labels[4] = { 'T', 'Z', 'Y', 'X' };
    const real_t *values[4] = { &c->t, &c->z, &c->y, &c->x };
    char buf[ENTRY_MAX + 4];

    for (int i = 0; i < 4; i++) {
        int y = STACK_TOP + i * ROW_H;
        bool is_x = (i == 3);

        if (is_x) {
            gfx_SetColor(COL_X_BG);
            gfx_FillRectangle(0, y, SCREEN_W, ROW_H);
        } else {
            gfx_SetColor(COL_RULE);
            gfx_HorizLine(0, y + ROW_H - 1, SCREEN_W);
        }

        char label[3] = { labels[i], ':', '\0' };
        gfx_SetTextScale(2, 2);
        gfx_SetTextFGColor(COL_LABEL);
        gfx_PrintStringXY(label, LABEL_X, y + (ROW_H - 16) / 2);

        if (is_x && c->editing) {
            strcpy(buf, c->entry);
            strcat(buf, "_");
            print_value(buf, y, COL_EDIT);
        } else {
            format_real(buf, sizeof buf, values[i], false);
            print_value(buf, y, COL_TEXT);
        }
    }
}

/* Menu row placeholder: five boxes over the Y= ... GRAPH keys. */
static void draw_menu_row(void)
{
    const int gap = 4;
    const int w = (SCREEN_W - gap * 6) / 5;

    gfx_SetColor(COL_MENU);
    for (int i = 0; i < 5; i++)
        gfx_FillRectangle(gap + i * (w + gap), MENU_Y, w, MENU_H - 4);
}

void display_draw(const calc_t *c, bool second)
{
    gfx_FillScreen(COL_BG);
    draw_status_bar(second);
    draw_message(c->message);
    draw_stack(c);
    draw_menu_row();
    gfx_SwapDraw();
}
