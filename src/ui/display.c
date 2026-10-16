#include <string.h>
#include <graphx.h>

#include "display.h"
#include "../core/format.h"
#include "../core/version.h"
#include "actions.h"

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
    COL_GROUP,
    COL_GROUP_TEXT,
    COL_BLANK,
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
    gfx_palette[COL_GROUP]    = gfx_RGBTo1555(70, 92, 120);
    gfx_palette[COL_GROUP_TEXT] = gfx_RGBTo1555(245, 248, 252);
    gfx_palette[COL_BLANK]    = gfx_RGBTo1555(236, 239, 243);
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
#define VALUE_MAX_SCALE 2   /* largest text scale for stack values */

/* ---------- menu label symbols ---------- */

/* 8x8 bitmaps (MSB = left pixel) for the MSYM_* codes, and their widths. */
static const uint8_t glyph_bits[MSYM_COUNT][8] = {
    { 0xC0, 0x20, 0x40, 0xE0, 0x00, 0x00, 0x00, 0x00 },  /* sq: superscript 2 */
    { 0xC0, 0x20, 0x40, 0x20, 0xC0, 0x00, 0x00, 0x00 },  /* cube: superscript 3 */
    { 0xC0, 0x20, 0x40, 0x20, 0xC0, 0x00, 0x00, 0x00 },  /* cuberoot: small 3 */
    { 0x0F, 0x08, 0x08, 0x90, 0x50, 0x30, 0x10, 0x00 },  /* root */
    { 0xF8, 0x80, 0x40, 0x20, 0x40, 0x80, 0xF8, 0x00 },  /* sigma */
    { 0x00, 0x10, 0x08, 0xFC, 0x08, 0x10, 0x00, 0x00 },  /* to: right arrow */
    { 0x00, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0x00, 0x00 },  /* block */
    { 0x00, 0x88, 0x88, 0x88, 0x98, 0xE8, 0x80, 0x80 },  /* mu */
    { 0x60, 0x90, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00 },  /* deg */
    { 0x20, 0x70, 0xA8, 0x20, 0x20, 0x20, 0x20, 0x00 },  /* up */
    { 0x20, 0x20, 0x20, 0x20, 0xA8, 0x70, 0x20, 0x00 },  /* down */
    { 0x00, 0x20, 0x40, 0xFC, 0x40, 0x20, 0x00, 0x00 },  /* left */
    { 0x00, 0x88, 0x50, 0x20, 0x50, 0x88, 0x00, 0x00 },  /* times */
};
static const uint8_t glyph_width[MSYM_COUNT] = { 4, 4, 3, 8, 6, 7, 6, 6, 5, 6, 6, 7, 6 };

static bool is_glyph(char ch)
{
    return (uint8_t)ch >= 1 && (uint8_t)ch <= MSYM_COUNT;
}

/* Width in pixels of a label at text scale 1. */
static int label_width(const char *s)
{
    int w = 0;
    for (; *s; s++)
        w += is_glyph(*s) ? glyph_width[(uint8_t)*s - 1] : (int)gfx_GetCharWidth(*s);
    return w;
}

/* Draws a label at text scale 1, mixing font characters and symbols. */
static void draw_label(const char *s, int x, int y, uint8_t color)
{
    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(color);
    gfx_SetColor(color);
    for (; *s; s++) {
        if (is_glyph(*s)) {
            uint8_t g = (uint8_t)*s - 1;
            for (int row = 0; row < 8; row++)
                for (int col = 0; col < 8; col++)
                    if (glyph_bits[g][row] & (0x80 >> col))
                        gfx_SetPixel(x + col, y + row);
            x += glyph_width[g];
        } else {
            gfx_SetTextXY(x, y);
            gfx_PrintChar(*s);
            x += gfx_GetCharWidth(*s);
        }
    }
}

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

/* Prints text right-aligned at the largest scale (up to VALUE_MAX_SCALE) that
 * fits between VALUE_LEFT and VALUE_RIGHT, vertically centered in a row. */
static void print_value(const char *s, int row_y, uint8_t color)
{
    uint8_t scale;
    unsigned int w;

    /* Measure at scale 1 and multiply, so the math doesn't depend on
     * whether graphx's width already includes the scale. */
    gfx_SetTextScale(1, 1);
    unsigned int base = gfx_GetStringWidth(s);

    for (scale = VALUE_MAX_SCALE; scale > 1; scale--) {
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

static void draw_status_bar(const calc_t *c, const menu_t *m, bool second)
{
    gfx_SetColor(COL_BAR);
    gfx_FillRectangle(0, 0, SCREEN_W, BAR_H);

    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(COL_BAR_TEXT);
    gfx_PrintStringXY(c->degrees ? "DEG" : "RAD", 8, 6);

    if (second) {
        gfx_SetColor(COL_SECOND);
        gfx_FillRectangle(44, 3, 34, 14);
        gfx_SetTextFGColor(COL_TEXT);
        gfx_PrintStringXY("2ND", 50, 6);
    }

    /* Menu name in the middle; "HOME" at the top level. */
    const char *title = (m->depth == 0) ? "HOME" : menu_groups[m->group].label;
    draw_label(title, (SCREEN_W - label_width(title)) / 2, 6, COL_BAR_TEXT);

    /* Row position on the right, with up/down arrows if there are more rows. */
    uint8_t rows = menu_groups[m->group].rows;
    if (rows > 1) {
        char pos[8] = { MSYM_UP, MSYM_DOWN, ' ',
                        (char)('1' + m->row), '/', (char)('0' + rows), '\0' };
        draw_label(pos, SCREEN_W - 8 - label_width(pos), 6, COL_BAR_TEXT);
    }
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

/* The current menu row: five boxes over the Y= ... GRAPH keys. Submenus
 * are dark boxes; empty slots are faint. */
static void draw_menu_row(const calc_t *c, const menu_t *m)
{
    const int gap = 4;
    const int w = (SCREEN_W - gap * 6) / 5;
    const int h = MENU_H - 4;

    for (uint8_t i = 0; i < 5; i++) {
        const menu_node_t *n = menu_slot(m, i);
        int x = gap + i * (w + gap);
        uint8_t box = COL_MENU, text = COL_TEXT;

        if (n->kind == MENU_BLANK) {
            box = COL_BLANK;
        } else if (n->kind == MENU_GROUP) {
            box = COL_GROUP;
            text = COL_GROUP_TEXT;
        }

        gfx_SetColor(box);
        gfx_FillRectangle(x, MENU_Y, w, h);

        if (n->kind == MENU_BLANK)
            continue;

        const char *label = n->label;
        if (n->kind == MENU_ALT && action_is_active(c, (menu_id_t)n->id))
            label = n->alt_label;

        int lw = label_width(label);
        draw_label(label, x + (w - lw) / 2, MENU_Y + (h - 8) / 2, text);

        /* small tab on submenu boxes */
        if (n->kind == MENU_GROUP) {
            gfx_SetColor(COL_GROUP_TEXT);
            gfx_HorizLine(x + w / 2 - 6, MENU_Y + 2, 12);
        }
    }
}

void display_draw(const calc_t *c, const menu_t *m, bool second)
{
    gfx_FillScreen(COL_BG);
    draw_status_bar(c, m, second);
    draw_message(c->message);
    draw_stack(c);
    draw_menu_row(c, m);
    gfx_SwapDraw();
}
