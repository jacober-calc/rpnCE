#include <string.h>
#include <graphx.h>

#include "display.h"
#include "../core/format.h"
#include "actions.h"
#include "dotfont.h"
#include "../core/units.h"

/*
 * Screen layout (320 x 240), RPN83P style:
 *   status line   annunciators on the left, messages on the right
 *   stack         T: Z: Y: X: in a dot-matrix LCD font
 *   menu row      always showing: five dark boxes with inverted labels
 */

/* ---------- colors (sampled from a TI-84 Plus LCD) ---------- */

enum {
    COL_INK = 1,        /* dark slate "LCD ink" */
    COL_PANEL,          /* gray-green LCD background */
    COL_FAINT,          /* faded ink: register labels */
    COL_DOTS,           /* near-black: the stack's digits and units */
};

static void set_palette(void)
{
    gfx_palette[COL_INK]   = gfx_RGBTo1555(34, 46, 49);
    gfx_palette[COL_PANEL] = gfx_RGBTo1555(156, 170, 155);
    gfx_palette[COL_FAINT] = gfx_RGBTo1555(56, 68, 68);
    gfx_palette[COL_DOTS]  = gfx_RGBTo1555(26, 32, 32);
}

/* ---------- layout ---------- */

#define SCREEN_W     320
#define STATUS_H     18
#define MENU_H       24
#define MENU_Y       (240 - MENU_H - 3)
#define STACK_TOP    (STATUS_H + 2)
#define ROW_H        ((MENU_Y - 3 - STACK_TOP) / 4)
#define LABEL_X      6
#define VALUE_LEFT   36      /* values never draw left of this */
#define VALUE_RIGHT  (SCREEN_W - 6)

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
    { 0xA0, 0x40, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00 },  /* supx: superscript x */
};
static const uint8_t glyph_width[MSYM_COUNT] = { 4, 4, 3, 8, 6, 7, 6, 6, 5, 6, 6, 7, 6, 4 };

static bool is_glyph(char ch)
{
    return (uint8_t)ch >= 1 && (uint8_t)ch <= MSYM_COUNT;
}

/* Width in pixels of a label at a text scale. */
static int label_width_scaled(const char *s, uint8_t scale)
{
    int w = 0;
    gfx_SetTextScale(1, 1);
    for (; *s; s++)
        w += is_glyph(*s) ? glyph_width[(uint8_t)*s - 1] : (int)gfx_GetCharWidth(*s);
    return w * scale;
}

static int label_width(const char *s)
{
    return label_width_scaled(s, 1);
}

/* Draws a label, mixing font characters and symbols, at a text scale. */
static void draw_label_scaled(const char *s, int x, int y, uint8_t color, uint8_t scale)
{
    gfx_SetTextFGColor(color);
    gfx_SetColor(color);
    for (; *s; s++) {
        if (is_glyph(*s)) {
            uint8_t g = (uint8_t)*s - 1;
            for (int row = 0; row < 8; row++)
                for (int col = 0; col < 8; col++)
                    if (glyph_bits[g][row] & (0x80 >> col))
                        gfx_FillRectangle(x + col * scale, y + row * scale, scale, scale);
            x += glyph_width[g] * scale;
        } else {
            gfx_SetTextScale(1, 1);
            int w = (int)gfx_GetCharWidth(*s) * scale;
            gfx_SetTextScale(scale, scale);
            gfx_SetTextXY(x, y);
            gfx_PrintChar(*s);
            x += w;
        }
    }
    gfx_SetTextScale(1, 1);
}

static void draw_label(const char *s, int x, int y, uint8_t color)
{
    draw_label_scaled(s, x, y, color, 1);
}

void display_open(void)
{
    gfx_Begin();
    set_palette();
    gfx_SetDrawBuffer();            /* draw off-screen, then swap */
    gfx_SetTextTransparentColor(255);
    gfx_SetTextBGColor(255);        /* 255 = transparent text background */
    /* Clip text at the screen edges; unclipped text past an edge would
     * spill into the following lines. */
    gfx_SetClipRegion(0, 0, SCREEN_W, 240);
    gfx_SetTextConfig(gfx_text_clip);
}

void display_close(void)
{
    gfx_End();
}

/* ---------- status line ---------- */

/* Left: annunciators, RPN83P style ("up/down" when the menu has more rows,
 * the display format such as FIX4 or FIX- for as many digits as needed,
 * then RAD or DEG, then 2ND when pending). Right: messages. */
static void draw_status(const calc_t *c, const menu_t *m, bool second, bool alpha)
{
    const int y = (STATUS_H - 8) / 2;
    int x = LABEL_X;
    char buf[8];

    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(COL_INK);

    if (menu_groups[m->group].rows > 1) {
        char arrows[3] = { MSYM_DOWN, MSYM_UP, '\0' };
        draw_label(arrows, x, y, COL_INK);
    }
    x += 16;

    static const char names[][4] = { "", "FIX", "SCI", "ENG" };
    memcpy(buf, names[c->disp_style], 3);
    buf[3] = (c->disp_digits == DISP_FLOAT) ? '-' : (char)('0' + c->disp_digits);
    buf[4] = '\0';
    gfx_PrintStringXY(buf, x, y);
    x += (int)gfx_GetStringWidth(buf) + 8;

    gfx_PrintStringXY(c->degrees ? "DEG" : "RAD", x, y);
    x += (int)gfx_GetStringWidth("DEG") + 8;

    /* HEX / OCT / BIN, with C when the carry flag is set */
    if (c->base.radix != 10) {
        const char *rn = (c->base.radix == 16) ? "HEX" : (c->base.radix == 8) ? "OCT" : "BIN";
        gfx_PrintStringXY(rn, x, y);
        x += (int)gfx_GetStringWidth(rn) + 4;
        if (c->base.carry) {
            gfx_PrintStringXY("C", x, y);
            x += (int)gfx_GetStringWidth("C");
        }
        x += 4;
    }

    /* stack depth (4STK) and total registers (R040), always shown */
    {
        buf[0] = (c->depth >= 10) ? '1' : (char)('0' + c->depth);
        int k = 1;
        if (c->depth >= 10)
            buf[k++] = (char)('0' + c->depth - 10);
        strcpy(buf + k, "STK");
        gfx_PrintStringXY(buf, x, y);
        x += (int)gfx_GetStringWidth(buf) + 8;
    }
    {
        int last = REG_EXTRA_FIRST - 1 + c->extra_count;   /* highest register */
        buf[0] = 'R';
        buf[1] = (char)('0' + last / 100);
        buf[2] = (char)('0' + last / 10 % 10);
        buf[3] = (char)('0' + last % 10);
        buf[4] = '\0';
        gfx_PrintStringXY(buf, x, y);
        x += (int)gfx_GetStringWidth(buf) + 8;
    }

    const char *shift = second ? "2ND" : alpha ? "ALPHA" : NULL;
    if (shift) {
        gfx_SetColor(COL_INK);
        gfx_FillRectangle(x - 2, y - 2, (int)gfx_GetStringWidth(shift) + 4, 12);
        gfx_SetTextFGColor(COL_PANEL);
        gfx_PrintStringXY(shift, x, y);
        gfx_SetTextFGColor(COL_INK);
        x += (int)gfx_GetStringWidth(shift) + 10;
    }

    if (c->message) {
        int mx = VALUE_RIGHT - label_width(c->message);
        if (mx < x)
            mx = x;
        draw_label(c->message, mx, y, COL_INK);
    }
}

/* ---------- stack ---------- */

/* Characters of a too-long X value hidden off the right edge. */
static int g_x_scroll;

void display_scroll_x(int delta)
{
    g_x_scroll += delta;
    if (g_x_scroll < 0)
        g_x_scroll = 0;
}

void display_reset_scroll(void)
{
    g_x_scroll = 0;
}

#define ARROW_W 8       /* room for a scroll arrow at each end */

/* Small filled triangle pointing left or right, vertically centered. */
static void draw_arrow(int x, int top, bool left)
{
    int cy = top + DOT_HEIGHT / 2;
    gfx_SetColor(COL_DOTS);
    for (int i = 0; i < 4; i++) {
        int col = left ? x + i : x + 3 - i;
        gfx_FillRectangle(col, cy - i, 1, 2 * i + 1);
    }
}

static int substring_width(const char *s, int from, int to)
{
    char tmp[48];
    int n = to - from;
    if (n > (int)sizeof tmp - 1)
        n = (int)sizeof tmp - 1;
    memcpy(tmp, s + from, (size_t)n);
    tmp[n] = '\0';
    return dot_width(tmp);
}

/* Draws a value right-aligned and vertically centered in a stack row, with
 * its unit (tag: 0 for none) in small text after it. A value too long for
 * the row stays on one line: the part that fits is shown, with arrows at
 * the ends where more digits are hidden. scroll (X row only, else NULL)
 * says how many characters are hidden at the right. */
static void draw_value(const char *s, int row_y, uint8_t tag, int *scroll)
{
    int right = VALUE_RIGHT;
    int top = row_y + (ROW_H - DOT_HEIGHT) / 2;

    if (tag) {
        const char *label = unit_table[tag - 1].label;
        int lw = label_width(label);
        draw_label(label, VALUE_RIGHT - lw, top + DOT_HEIGHT - 8, COL_DOTS);
        right = VALUE_RIGHT - lw - 5;
    }

    int w = dot_width(s);
    if (w <= right - VALUE_LEFT) {
        dot_draw(s, right - w, top, COL_DOTS, 0);
        return;
    }

    /* Too long: show a window of whole characters. */
    int len = (int)strlen(s);
    int area_left = VALUE_LEFT + ARROW_W, area_right = right - ARROW_W;
    int area = area_right - area_left;

    int first_end = 1;          /* the window can't scroll past the start */
    while (first_end < len && substring_width(s, 0, first_end + 1) <= area)
        first_end++;
    int max_scroll = len - first_end;
    int sc = scroll ? *scroll : 0;
    if (sc > max_scroll)
        sc = max_scroll;
    if (scroll)
        *scroll = sc;

    int end = len - sc, start = end - 1;
    while (start > 0 && substring_width(s, start - 1, end) <= area)
        start--;

    char tmp[48];
    int n = end - start;
    memcpy(tmp, s + start, (size_t)n);
    tmp[n] = '\0';
    dot_draw(tmp, area_right - dot_width(tmp), top, COL_DOTS, 0);
    if (start > 0)
        draw_arrow(VALUE_LEFT, top, true);
    if (sc > 0)
        draw_arrow(right - 4, top, false);
}

/* Formats a stack value for the display: in HEX, OCT or BIN when that mode
 * is on and the value is a word that fits the word size. Returns true when
 * it's a base number. */
static bool format_value(char *buf, const calc_t *c, const real_t *v)
{
    uint32_t w;
    if (c->base.radix != 10 && base_word(&c->base, v, &w)) {
        base_format(buf, &c->base, w);
        return true;
    }
    format_real(buf, v, c->disp_style, c->disp_digits,
                DISPLAY_DIGITS, DISPLAY_EXP_DIGITS, true);
    return false;
}


/* T, Z, Y, X with their labels. With a STO/RCL prompt open, the prompt takes
 * the X row; for a plain RCL the other rows show the stack as it will be
 * after the lift. */
static void draw_stack(const calc_t *c, const char *prompt, bool lifted)
{
    static const char *const labels[4] = { "T:", "Z:", "Y:", "X:" };
    static const uint8_t regs[4] = { REG_T, REG_Z, REG_Y, REG_X };
    const real_t *values[4] = { &c->stack[3], &c->stack[2], &c->stack[1], &c->stack[0] };
    const real_t *shown_lifted[4] = { &c->stack[2], &c->stack[1], &c->stack[0], &c->stack[0] };
    char buf[FORMAT_MAX > ENTRY_MAX + 2 ? FORMAT_MAX : ENTRY_MAX + 2];

    for (int i = 0; i < 4; i++) {
        int y = STACK_TOP + i * ROW_H;
        bool is_x = (i == 3);

        dot_draw(labels[i], LABEL_X, y + (ROW_H - DOT_HEIGHT) / 2, COL_FAINT, 0);

        if (is_x && prompt) {
            draw_value(prompt, y, 0, NULL);
        } else if (is_x && c->editing) {
            strcpy(buf, c->entry);
            strcat(buf, "_");
            draw_value(buf, y, 0, NULL);        /* typing: the end is shown */
        } else {
            bool shift = prompt && lifted;
            const real_t *v = shift ? shown_lifted[i] : values[i];
            uint8_t tag = c->unit[shift && i < 3 ? regs[i + 1] : regs[i]];
            format_value(buf, c, v);
            draw_value(buf, y, tag, is_x ? &g_x_scroll : NULL);
        }
    }
}

/* ---------- VARS catalog ---------- */

/* Four lines of NAME=value in the stack font; the selected line is shown
 * inverted. */
static void draw_catalog(const calc_t *c, const catalog_t *cat)
{
    char name[12], buf[40];             /* room for 32 binary digits */

    for (int row = 0; row < 4; row++) {
        int idx = cat->top + row;
        if (idx >= cat->count)
            break;
        int y = STACK_TOP + row * ROW_H;
        int top = y + (ROW_H - DOT_HEIGHT) / 2;
        bool sel = (idx == cat->sel);
        uint8_t ink = COL_DOTS;

        if (sel) {
            gfx_SetColor(COL_INK);
            gfx_FillRectangle(0, top - 4, SCREEN_W, DOT_HEIGHT + 8);
            ink = COL_PANEL;
        }

        uint8_t item = cat->items[idx];
        catalog_name(item, name);
        strcat(name, "=");
        dot_draw(name, LABEL_X, top, ink, 0);
        int name_end = LABEL_X + dot_width(name) + 6;

        format_value(buf, c, catalog_value((calc_t *)c, item));
        dot_draw(buf, VALUE_RIGHT - dot_width(buf), top, ink, name_end);
    }
}

/* ---------- menu row ---------- */

/* Five dark boxes with inverted labels over the Y= ... GRAPH keys. A small
 * tab along the top marks a submenu; empty slots have no box. */
static void draw_menu_row(const calc_t *c, const menu_t *m)
{
    const int gap = 3;
    const int w = (SCREEN_W - gap * 6) / 5;

    for (uint8_t i = 0; i < 5; i++) {
        const menu_node_t *n = menu_slot(m, i);
        if (n->kind == MENU_BLANK)
            continue;

        int x = gap + i * (w + gap);
        gfx_SetColor(COL_INK);
        gfx_FillRectangle(x, MENU_Y, w, MENU_H);

        const char *label = n->label;
        if (n->kind == MENU_ALT && action_is_active(c, (menu_id_t)n->id))
            label = n->alt_label;

        int lw = label_width(label);
        draw_label(label, x + (w - lw) / 2, MENU_Y + (MENU_H - 8) / 2 + 1, COL_PANEL);

        if (n->kind == MENU_GROUP) {
            gfx_SetColor(COL_PANEL);
            gfx_HorizLine(x + w / 2 - 8, MENU_Y + 2, 16);
        }
    }
}

void display_draw(const calc_t *c, const menu_t *m, bool second, bool alpha,
                  const char *prompt, bool prompt_lifts, const catalog_t *cat)
{
    gfx_FillScreen(COL_PANEL);
    draw_status(c, m, second, alpha);
    if (cat && cat->open)
        draw_catalog(c, cat);
    else
        draw_stack(c, prompt, prompt_lifts);
    draw_menu_row(c, m);
    gfx_SwapDraw();
}
