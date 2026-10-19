/* Whole-display drawing commands captured from display.c at 2e29e01.
 * GraphX text metrics are deterministic stand-ins; dot-font drawing uses
 * the actual renderer. Only intentional appearance changes may update the
 * fingerprint. Tests also verify formatting work saved across redraws. */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <graphx.h>
#include "display.h"
#include "res.h"
#include "../src/core/format.h"
#include "../src/core/dateobj.h"
#include "mock_calc_runtime.h"

uint8_t mock_res[65536];
size_t mock_res_size;
uint16_t gfx_palette[256];
static uint64_t fingerprint;
static uint8_t scale_x = 1;
static int fails;

static void record(uint32_t value)
{
    for (int i = 0; i < 4; i++) {
        fingerprint ^= (uint8_t)(value >> (i * 8));
        fingerprint *= UINT64_C(1099511628211);
    }
}

void gfx_Begin(void) { scale_x = 1; record(1); }
void gfx_End(void) { record(2); }
void gfx_SetDrawBuffer(void) { record(3); }
void gfx_SwapDraw(void) { record(4); }
uint8_t gfx_SetColor(uint8_t c) { record(5); record(c); return 0; }
void gfx_FillScreen(uint8_t c) { record(6); record(c); }
void gfx_FillRectangle(int x, int y, int w, int h)
{ record(7); record((uint32_t)x); record((uint32_t)y); record((uint32_t)w); record((uint32_t)h); }
void gfx_HorizLine(int x, int y, int w)
{ record(8); record((uint32_t)x); record((uint32_t)y); record((uint32_t)w); }
uint16_t gfx_RGBTo1555(uint8_t r, uint8_t g, uint8_t b)
{ return (uint16_t)((r >> 3) << 10 | (g >> 3) << 5 | b >> 3); }
uint8_t gfx_SetTextFGColor(uint8_t c) { record(9); record(c); return 0; }
uint8_t gfx_SetTextBGColor(uint8_t c) { record(10); record(c); return 0; }
uint8_t gfx_SetTextTransparentColor(uint8_t c) { record(11); record(c); return 0; }
void gfx_SetTextScale(uint8_t x, uint8_t y)
{ scale_x = x; record(12); record(x); record(y); }
void gfx_SetTextXY(int x, int y) { record(13); record((uint32_t)x); record((uint32_t)y); }
void gfx_SetTextConfig(uint8_t c) { record(14); record(c); }
void gfx_SetClipRegion(int a, int b, int c, int d)
{ record(15); record((uint32_t)a); record((uint32_t)b); record((uint32_t)c); record((uint32_t)d); }
unsigned int gfx_GetCharWidth(char c)
{ return (unsigned)(c == ' ' || c == '.' || c == ',' || c == ':' ? 3 : 6) * scale_x; }
unsigned int gfx_GetStringWidth(const char *s)
{ unsigned w = 0; for (; *s; s++) w += gfx_GetCharWidth(*s); return w; }
void gfx_PrintChar(char c) { record(16); record((uint8_t)c); }
void gfx_PrintStringXY(const char *s, int x, int y)
{
    record(17); record((uint32_t)x); record((uint32_t)y);
    for (; *s; s++) record((uint8_t)*s);
    record(0);
}

static void check(const char *name, bool pass)
{
    if (!pass) fails++;
    printf("%s %s\n", pass ? "ok  " : "FAIL", name);
}

static void draw(calc_t *c, menu_t *m, catalog_t *cat, const char *prompt, bool lift)
{
    display_draw(c, m, false, false, prompt, lift, cat);
}

static void appearance(void)
{
    calc_t c;
    menu_t m;
    catalog_t cat = { 0 };
    calc_init(&c);
    menu_home(&m);
    for (int i = 0; i < STACK_MAX; i++) c.stack[i] = mk(i + 0.125);
    c.stack[0] = mk(123456789012.0);
    fingerprint = UINT64_C(14695981039346656037);
    display_open();
    draw(&c, &m, &cat, NULL, false);
    draw(&c, &m, &cat, NULL, false); /* unchanged values */
    display_draw(&c, &m, true, false, NULL, false, &cat);
    display_draw(&c, &m, false, true, NULL, false, &cat);
    menu_jump(&m, MENU_GROUP_MODE);
    draw(&c, &m, &cat, NULL, false);
    for (uint8_t style = DISP_FIX; style <= DISP_ENG; style++) {
        c.disp_style = style;
        for (int digits = -1; digits <= 11; digits += 4) {
            c.disp_digits = (int8_t)digits;
            draw(&c, &m, &cat, NULL, false);
        }
    }
    c.unit[0] = 1;
    draw(&c, &m, &cat, NULL, false);
    draw(&c, &m, &cat, "STO _", false);
    draw(&c, &m, &cat, "RCL _", true);
    c.editing = true;
    strcpy(c.entry, "-1.23456789012E-99");
    c.entry_len = (uint8_t)strlen(c.entry);
    draw(&c, &m, &cat, NULL, false);
    c.editing = false;
    static const uint8_t radices[] = { 2, 8, 16 };
    for (unsigned radix = 0; radix < sizeof radices / sizeof radices[0]; radix++) {
        c.base.radix = radices[radix];
        c.base.bits = BASE_MAX_BITS;
        c.stack[0] = mk(17592186044415.0);
        display_reset_scroll();
        draw(&c, &m, &cat, NULL, false);
        for (int scroll = 0; scroll < 48; scroll++) {
            display_scroll_x(1);
            draw(&c, &m, &cat, NULL, false);
        }
        display_scroll_x(-100);
        c.base.bits = 8;
        draw(&c, &m, &cat, NULL, false); /* word-size warning and decimal fallback */
        c.stack[0] = mk(-5);
        draw(&c, &m, &cat, NULL, false);
        c.stack[0] = mk(0.25);
        draw(&c, &m, &cat, NULL, false);
    }
    c.base.radix = 10;
    dt_obj_t date = { .type = OBJ_DZ, .year = 2026, .month = 10, .day = 18,
                     .hour = 9, .minute = 30, .tz_hour = -7 };
    c.stack[0] = obj_pack(&date);
    c.date_as_string = false;
    draw(&c, &m, &cat, NULL, false);
    c.date_as_string = true;
    draw(&c, &m, &cat, NULL, false);
    date.day++;
    c.stack[0] = obj_pack(&date);
    draw(&c, &m, &cat, NULL, false);
    c.message = "Err: Domain";
    draw(&c, &m, &cat, NULL, false);
    c.message = NULL;
    c.regs[0] = mk(123.5);
    c.regs[1] = c.stack[0];
    catalog_open(&cat, &c);
    draw(&c, &m, &cat, NULL, false);
    catalog_move(&cat, 1);
    draw(&c, &m, &cat, NULL, false);
    cat.open = false;
    for (uint8_t tint = 0; tint < TINT_COUNT; tint++) {
        display_set_tint(tint);
        draw(&c, &m, &cat, NULL, false);
    }
    display_close();
    display_open();
    draw(&c, &m, &cat, NULL, false);
    display_close();
    printf("display fingerprint: %016" PRIx64 "\n", fingerprint);
    check("whole-display drawing commands match baseline", fingerprint == UINT64_C(0x2c060cd2d4b4f9fa));
}

static void formatting_work(void)
{
    calc_t c;
    menu_t m;
    catalog_t cat = { 0 };
    calc_init(&c);
    menu_home(&m);
    for (int i = 0; i < 4; i++) c.stack[i] = mk(i + 0.125);
    display_open();
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("first frame formats all four stack values", mock_format_calls == 4);
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    display_draw(&c, &m, true, false, NULL, false, &cat);
    display_draw(&c, &m, false, true, NULL, false, &cat);
    menu_row_down(&m);
    draw(&c, &m, &cat, NULL, false);
    mock_clock[4]++;
    draw(&c, &m, &cat, NULL, false);
    check("unchanged, modifier, menu and clock redraws reuse formatting", mock_format_calls == 0);
    c.stack[0] = mk(10);
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("changing X formats just X", mock_format_calls == 1);
    c.unit[0] = 1;
    c.degrees = !c.degrees;
    c.base.carry = !c.base.carry;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("units and annunciators remain live without reformatting numbers", mock_format_calls == 0);
    c.editing = true;
    strcpy(c.entry, "123");
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    draw(&c, &m, &cat, "STO _", false);
    check("entry and store prompts reuse the other three rows", mock_format_calls == 0);
    c.editing = false;
    draw(&c, &m, &cat, "RCL _", true);
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("leaving a lifted prompt restores actual stack formatting", mock_format_calls == 3);
    c.disp_digits = 4;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("precision changes invalidate all rows", mock_format_calls == 4);
    c.disp_style = DISP_SCI;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("display style changes invalidate all rows", mock_format_calls == 4);
    c.date_as_string = true;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("DATE representation changes invalidate all rows", mock_format_calls == 4);
    /* Fractions stay decimal in BASE mode, so format_real calls also reveal
     * whether radix and word-size changes invalidate the cache. */
    for (int i = 0; i < 4; i++) c.stack[i] = mk(i + 0.125);
    c.base.radix = 16;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("radix changes invalidate all rows", mock_format_calls == 4);
    c.base.bits = 8;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("word-size changes invalidate all rows", mock_format_calls == 4);
    c.base.radix = 10;
    c.regs[0] = mk(1);
    check("catalog fixture opens", catalog_open(&cat, &c));
    draw(&c, &m, &cat, NULL, false);
    c.disp_style = DISP_ENG;
    draw(&c, &m, &cat, NULL, false);
    cat.open = false;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("mode changes in the catalog refresh stack formatting on return", mock_format_calls == 4);
    display_close();
    display_open();
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("reopening the display invalidates the cache", mock_format_calls == 4);
    display_close();
}

int main(void)
{
    FILE *f = fopen("data/RPNCEres.bin", "rb");
    if (!f) return 1;
    mock_res_size = fread(mock_res, 1, sizeof mock_res, f);
    fclose(f);
    if (!res_open()) return 1;
    appearance();
    formatting_work();
    printf("%d failures\n", fails);
    return fails != 0;
}
