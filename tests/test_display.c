/* Display snapshots use fixed GraphX metrics and the actual dot renderer.
 * Update snapshots only for an intended appearance change. */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <graphx.h>
#include "display.h"
#include "res.h"
#include "../src/core/format.h"
#include "../src/core/value.h"
#include "../src/core/units.h"
#include "mock_calc_runtime.h"

uint8_t mock_res[65536];
size_t mock_res_size;
uint16_t gfx_palette[256];
static uint64_t fingerprint;
static uint8_t scale_x = 1, scale_y = 1;
static uint8_t text_config;
static int text_y;
static unsigned large_message_chars, bad_status_scale, arrow_pixels, bad_arrow_size;
static int fails;
static char printed[4096];
static size_t printed_count;
static struct { char text[24]; int x, width; } status_text[8];
static unsigned status_count;
static bool wide_font;

static void printed_char(char c)
{
    if (printed_count + 1 < sizeof printed) {
        printed[printed_count++] = c;
        printed[printed_count] = '\0';
    }
}

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
{
    if (x == 0 && y == 0 && w == 320 && h == 18) status_count = 0;
    record(7); record((uint32_t)x); record((uint32_t)y); record((uint32_t)w); record((uint32_t)h);
    if (x >= 302 && y >= 18 && y < 48) {
        arrow_pixels++;
        if (w != 1 || h != 2) bad_arrow_size++;
    }
}
void gfx_HorizLine(int x, int y, int w)
{ record(8); record((uint32_t)x); record((uint32_t)y); record((uint32_t)w); }
uint16_t gfx_RGBTo1555(uint8_t r, uint8_t g, uint8_t b)
{ return (uint16_t)((r >> 3) << 10 | (g >> 3) << 5 | b >> 3); }
uint8_t gfx_SetTextFGColor(uint8_t c) { record(9); record(c); return 0; }
uint8_t gfx_SetTextBGColor(uint8_t c) { record(10); record(c); return 0; }
uint8_t gfx_SetTextTransparentColor(uint8_t c) { record(11); record(c); return 0; }
void gfx_SetTextScale(uint8_t x, uint8_t y)
{ scale_x = x; scale_y = y; record(12); record(x); record(y); }
void gfx_SetTextXY(int x, int y) { text_y = y; record(13); record((uint32_t)x); record((uint32_t)y); }
void gfx_SetTextConfig(uint8_t c) { text_config = c; record(14); record(c); }
void gfx_SetClipRegion(int a, int b, int c, int d)
{ record(15); record((uint32_t)a); record((uint32_t)b); record((uint32_t)c); record((uint32_t)d); }
unsigned int gfx_GetCharWidth(char c)
{ return (unsigned)(c == ' ' || c == '.' || c == ',' || c == ':' ? 3 : wide_font ? 8 : 6) * scale_x; }
unsigned int gfx_GetStringWidth(const char *s)
{ unsigned w = 0; for (; *s; s++) w += gfx_GetCharWidth(*s); return w; }
void gfx_PrintChar(char c)
{
    record(16); record((uint8_t)c); printed_char(c);
    if (text_y >= 18 && text_y < 48 && text_config == gfx_text_noclip &&
        scale_x == 1 && scale_y == 2)
        large_message_chars++;
}
void gfx_PrintStringXY(const char *s, int x, int y)
{
    if (y < 18 && (scale_x != 1 || scale_y != 1)) bad_status_scale++;
    if (y < 18 && status_count < sizeof status_text / sizeof *status_text) {
        snprintf(status_text[status_count].text, sizeof status_text[status_count].text, "%s", s);
        status_text[status_count].x = x;
        status_text[status_count].width = gfx_GetStringWidth(s);
        status_count++;
    }
    record(17); record((uint32_t)x); record((uint32_t)y);
    for (; *s; s++) { record((uint8_t)*s); printed_char(*s); }
    record(0);
}

static bool status_has(const char *text)
{
    for (unsigned i = 0; i < status_count; i++)
        if (!strcmp(status_text[i].text, text)) return true;
    return false;
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
    for (int i = 0; i < STACK_MAX; i++) c.stack[i] = value_real(mk(i + 0.125));
    c.stack[0] = value_real(mk(123456789012.0));
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
    c.stack[0].unit = 1;
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
        c.stack[0] = value_real(mk(17592186044415.0)); c.stack[0].unit = 1;
        display_reset_scroll();
        draw(&c, &m, &cat, NULL, false);
        for (int scroll = 0; scroll < 48; scroll++) {
            display_scroll_x(1);
            draw(&c, &m, &cat, NULL, false);
        }
        display_scroll_x(-100);
        c.base.bits = 8;
        draw(&c, &m, &cat, NULL, false); /* word-size warning and decimal fallback */
        c.stack[0] = value_real(mk(-5)); c.stack[0].unit = 1;
        draw(&c, &m, &cat, NULL, false);
        c.stack[0] = value_real(mk(0.25)); c.stack[0].unit = 1;
        draw(&c, &m, &cat, NULL, false);
    }
    c.base.radix = 10;
    dt_obj_t date = { .type = OBJ_DZ, .year = 2026, .month = 10, .day = 18,
                     .hour = 9, .minute = 30, .tz_hour = -7 };
    c.stack[0] = value_date(&date);
    c.date_as_string = false;
    draw(&c, &m, &cat, NULL, false);
    c.date_as_string = true;
    draw(&c, &m, &cat, NULL, false);
    date.day++;
    c.stack[0] = value_date(&date);
    draw(&c, &m, &cat, NULL, false);
    c.message = "Err: Domain";
    draw(&c, &m, &cat, NULL, false);
    c.message = NULL;
    c.regs[0] = value_real(mk(123.5));
    c.regs[1] = c.stack[0];
    catalog_open(&cat, &c);
    /* This snapshot isolates the seeded A and B registers. */
    cat.count = 2;
    draw(&c, &m, &cat, NULL, false);
    catalog_move(&cat, 1);
    draw(&c, &m, &cat, NULL, false);
    cat.open = false;
    /* Palette snapshots cover the first four preset slots. */
    for (uint8_t tint = 0; tint < 4; tint++) {
        display_set_tint(tint);
        draw(&c, &m, &cat, NULL, false);
    }
    display_close();
    display_open();
    draw(&c, &m, &cat, NULL, false);
    display_close();
    printf("display fingerprint: %016" PRIx64 "\n", fingerprint);
    /* Format remains visible; default mode indicators are omitted. Labels
     * set the text scale once per label rather than once per character. */
    check("whole-display drawing commands match approved layout", fingerprint == UINT64_C(0xf3277bfe6b89cab4));
}

static void formatting_work(void)
{
    calc_t c;
    menu_t m;
    catalog_t cat = { 0 };
    calc_init(&c);
    menu_home(&m);
    for (int i = 0; i < 4; i++) c.stack[i] = value_real(mk(i + 0.125));
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
    c.stack[0] = value_real(mk(10));
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("changing X formats just X", mock_format_calls == 1);
    c.stack[0].unit = 1;
    c.degrees = !c.degrees;
    c.base.carry = !c.base.carry;
    mock_format_calls = 0;
    printed_count = 0; printed[0] = '\0';
    draw(&c, &m, &cat, NULL, false);
    check("units and annunciators remain live without reformatting numbers", mock_format_calls == 0);
    check("carry remains visible with the implicit DEC mode", status_has("CF") && !status_has("DEC"));
    calc_execute(&c, CMD_CLEAR_CARRY, 0);
    printed_count = 0; printed[0] = '\0';
    draw(&c, &m, &cat, NULL, false);
    check("CCF removes the carry indicator", !status_has("CF"));
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
    c.decimal_comma = true;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("numeric separator changes invalidate every cached row", mock_format_calls == 4);
    /* Fractions stay decimal in BASE mode, so format_real calls also reveal
     * whether radix and word-size changes invalidate the cache. */
    for (int i = 0; i < 4; i++) c.stack[i] = value_real(mk(i + 0.125));
    c.base.radix = 16;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("radix changes invalidate all rows", mock_format_calls == 4);
    c.base.bits = 8;
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("word-size changes invalidate all rows", mock_format_calls == 4);
    c.base.radix = 10;
    c.regs[0] = value_real(mk(1));
    catalog_open(&cat, &c);
    check("catalog fixture opens", cat.open);
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
    c.stack[0] = value_complex(mk(3),mk(4));
    mock_format_calls = 0; draw(&c,&m,&cat,NULL,false);
    check("complex X formats both components", mock_format_calls == 2);
    c.stack[0].imag = mk(5);
    mock_format_calls = 0; draw(&c,&m,&cat,NULL,false);
    check("imaginary changes invalidate the complete value cache", mock_format_calls == 2);
    c.complex_mode = COMPLEX_RAD;
    mock_format_calls = 0; draw(&c,&m,&cat,NULL,false);
    check("complex mode changes invalidate formatting", mock_format_calls == 5);
    display_close();
}

static void quantity_catalog_and_help(void)
{
    calc_t c;
    menu_t m;
    catalog_t cat = {0};
    calc_init(&c);
    menu_home(&m);
    for (uint8_t i = 0; i < UNIT_COUNT; i++) {
        if (!strcmp(unit_label(i), "ft")) c.regs[0].unit = i + 1;
        if (!strcmp(unit_label(i), "in")) c.lastx.unit = i + 1;
    }
    display_open();
    catalog_open(&cat, &c);
    check("zero quantities appear in catalog", cat.open && cat.count == 2 + TVM_VAR_COUNT);
    printed_count = 0; printed[0] = '\0';
    draw(&c, &m, &cat, NULL, false);
    catalog_move(&cat, -1);  /* LASTX now follows every TVM entry. */
    draw(&c, &m, &cat, NULL, false);
    check("catalog draws register and LASTX unit labels", strstr(printed, "ft") && strstr(printed, "in"));
    cat.open = false;
    draw(&c, &m, &cat, NULL, false);
    calc_t before = c;
    for (uint8_t page = 0; page < res_help_pages(); page++) {
        printed_count = 0; printed[0] = '\0';
        display_help(page);
        check("HELP pages include navigation and expected title", strstr(printed, res_help_line(page, 0)) &&
              strstr(printed, "UP / DOWN") && strstr(printed, "CLEAR / ON"));
    }
    mock_format_calls = 0;
    draw(&c, &m, &cat, NULL, false);
    check("HELP preserves calculator state and formatted stack cache",
          !memcmp(&before, &c, sizeof c) && !mock_format_calls);
    display_close();
}

static void application_clock(void)
{
    calc_t c;
    menu_t m;
    catalog_t cat = {0};
    int saved[6];
    memcpy(saved, mock_clock, sizeof saved);
    const int local[6] = {2026, 12, 31, 23, 30, 0};
    memcpy(mock_clock, local, sizeof local);
    calc_init(&c);
    menu_home(&m);
    c.clock_tz = -28;
    display_open();
    static const struct { int8_t zone; const char *text; } cases[] = {
        {-28, "2026-12-31 23:30"},
        {36, "2027-01-01 15:30"},
        {-14, "2027-01-01 03:00"},
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        c.app_tz = cases[i].zone;
        calc_t before = c;
        printed_count = 0; printed[0] = '\0';
        draw(&c, &m, &cat, NULL, false);
        check("status clock follows ATZ including fractional offsets and year rollover",
              strstr(printed, cases[i].text) != NULL);
        check("status clock preserves calculator state and hardware time",
              !memcmp(&before, &c, sizeof c) && !memcmp(mock_clock, local, sizeof local));
    }
    mock_clock[1] = 0;
    printed_count = 0; printed[0] = '\0';
    draw(&c, &m, &cat, NULL, false);
    check("invalid hardware clock is omitted", strstr(printed, "2026-") == NULL &&
          strstr(printed, "2027-") == NULL);
    display_close();
    memcpy(mock_clock, saved, sizeof saved);
}

static void message_acknowledgement(void)
{
    calc_t c;
    menu_t m;
    catalog_t cat = {0};
    calc_init(&c);
    menu_home(&m);
    display_open();
    c.stack[0] = value_real(mk(123)); c.stack[1] = value_real(mk(456));
    c.lastx = value_real(mk(7)); c.stack[0].unit = 1;
    calc_execute(&c, CMD_SHOW, 0);
    check("SHOW is a persistent message", display_has_message(&c) && !display_message_is_notice(&c));
    calc_t expected = c;
    expected.message = NULL;
    display_dismiss_message(&c);
    check("dismissing SHOW preserves all calculator state", !memcmp(&expected, &c, sizeof c));
    c.stack[0] = value_real(mk(-1));
    calc_execute(&c, CMD_SQRT, 0);
    check("errors persist until a key",
          c.message && !strcmp(c.message, "Err: Domain") && !display_message_is_notice(&c));
    expected = c; expected.message = NULL;
    display_dismiss_message(&c);
    check("dismissing an error changes only the message", !memcmp(&expected, &c, sizeof c));
    c.stack[0] = value_real(mk(0));
    calc_execute(&c, CMD_TVM_VAR, TVM_PMT);          /* stores PMT = 0 */
    check("ordinary notices can time out",
          c.message && !strcmp(c.message, "TVM Stored") && display_message_is_notice(&c));
    os_Fin_I = mk(0); os_Fin_PY = os_Fin_CY = mk(12);
    calc_execute(&c, CMD_TVM_VAR, TVM_N);            /* no payments: no N */
    check("TVM solver failures remain warnings",
          c.message && !strcmp(c.message, "TVM Not Found") && !display_message_is_notice(&c));
    c.message = NULL; c.stack[0].unit = 0;
    c.base.radix = 16; c.base.bits = 8; c.stack[0] = value_real(mk(256));
    check("automatic BASE warnings are messages", display_has_message(&c));
    expected = c;
    display_dismiss_message(&c);
    printed_count = 0; printed[0] = '\0';
    draw(&c, &m, &cat, NULL, false);
    draw(&c, &m, &cat, NULL, false);
    check("dismissed BASE warnings stay cleared across redraws",
          !display_has_message(&c) && !strstr(printed, "Exceeds") && !memcmp(&expected, &c, sizeof c));
    c.stack[0] = value_real(mk(257));
    check("a changed value can produce a new BASE warning", display_has_message(&c));
    display_dismiss_message(&c);
    c.base.bits = 7;
    check("a changed word size can produce a new BASE warning", display_has_message(&c));
    display_close();
}

/* Verify semantic indicators and geometry, including fonts as wide as the
 * CE font. No mode may collide with the clock or another status item. */
static void number_mode_status(void)
{
    calc_t c;
    menu_t m;
    catalog_t cat = {0};
    int saved_clock[6];
    memcpy(saved_clock, mock_clock, sizeof saved_clock);
    const int now[6] = {8888, 12, 28, 23, 58, 0};
    memcpy(mock_clock, now, sizeof now);
    calc_init(&c); menu_home(&m); display_open();
    c.disp_style = DISP_FIX;
    static const int8_t precisions[] = {DISP_FLOAT, 4, 11};
    static const char *const formats[] = {"ALL", "FIX4", "FIX11"};
    static const int radices[] = {10, 16, 8, 2};
    bool labels = true, geometry = true, unchanged = true, full_clock = false, time_only = false;
    for (int font = 0; font < 2; font++)
    for (unsigned precision = 0; precision < sizeof precisions / sizeof *precisions; precision++)
    for (int result = 0; result < 2; result++)
    for (int mode = COMPLEX_RECT; mode <= COMPLEX_DEG; mode++)
    for (int angle = 0; angle < 2; angle++)
    for (unsigned radix = 0; radix < sizeof radices / sizeof *radices; radix++)
    for (int carry = 0; carry < 2; carry++)
    for (int modifier = 0; modifier < 3; modifier++) {
        wide_font = font != 0;
        c.disp_digits = precisions[precision];
        c.complex_results = result != 0; c.complex_mode = mode;
        c.degrees = angle != 0; c.base.radix = radices[radix]; c.base.carry = carry != 0;
        calc_t before = c;
        status_count = 0;
        display_draw(&c, &m, modifier == 1, modifier == 2, NULL, false, &cat);
        unchanged &= !memcmp(&before, &c, sizeof c);
        unsigned expected = 2 + !angle + (radix != 0) + carry + result + (mode != COMPLEX_RECT) + (modifier != 0);
        labels &= status_count == expected && status_has(formats[precision]) &&
            !status_has("DEG") && status_has("RAD") == !angle && !status_has("DEC") &&
            !status_has("RRES") && !status_has("RECT") &&
            status_has("CRES") == (result != 0) &&
            status_has("PRAD") == (mode == COMPLEX_RAD) &&
            status_has("PDEG") == (mode == COMPLEX_DEG) &&
            status_has("CF") == (carry != 0) &&
            status_has("HEX") == (radix == 1) &&
            status_has("OCT") == (radix == 2) &&
            status_has("BIN") == (radix == 3);
        if (modifier) labels &= status_has(modifier == 1 ? "2ND" : "A");
        full_clock |= !strcmp(status_text[0].text, "8888-12-28 23:58");
        time_only |= !strcmp(status_text[0].text, "23:58");
        for (unsigned i = 0; i < status_count; i++) {
            geometry &= status_text[i].x >= 4 && status_text[i].x + status_text[i].width <= 316;
            if (i) geometry &= status_text[i - 1].x + status_text[i - 1].width + 2 <= status_text[i].x;
        }
    }
    check("status shows format, RAD and only active optional indicators", labels);
    check("status indicators and clock fit without overlap", geometry);
    check("crowded status bars keep time while ordinary states keep the full date", full_clock && time_only);
    check("drawing number-mode indicators preserves calculator state", unchanged);
    wide_font = false;
    c.disp_style = DISP_SCI; c.disp_digits = DISP_FLOAT;
    draw(&c, &m, &cat, NULL, false);
    check("floating SCI retains its own format indicator", status_has("SCI-") && !status_has("ALL"));
    c.disp_style = DISP_ENG;
    draw(&c, &m, &cat, NULL, false);
    check("floating ENG retains its own format indicator", status_has("ENG-") && !status_has("ALL"));
    display_close();
    memcpy(mock_clock, saved_clock, sizeof saved_clock);
}

static void warning_size(void)
{
    calc_t c;
    menu_t m;
    catalog_t cat = {0};
    calc_init(&c);
    menu_home(&m);
    display_open();
    c.message = "Err: Unit Mismatch";
    large_message_chars = bad_status_scale = arrow_pixels = bad_arrow_size = 0;
    draw(&c, &m, &cat, NULL, false);
    check("error glyphs are taller, with their original width and spacing",
          large_message_chars == strlen(c.message));
    check("status bar font and menu arrows retain their original size",
          !bad_status_scale && arrow_pixels && !bad_arrow_size && text_config == gfx_text_clip);
    c.message = NULL;
    c.base.radix = 16; c.base.bits = 8; c.stack[0] = value_real(mk(256));
    large_message_chars = 0;
    draw(&c, &m, &cat, NULL, false);
    check("BASE warnings use the larger text", large_message_chars == strlen("Exceeds 8-bit word"));
    static const char *const messages[] = {
        "Stack cleared", "Registers Cleared", "Stats Cleared",
        "Register Size Set", "Stack Size Set", "Word Size Set",
        "Epoch Set", "Time Zone Set", "Clock Set",
        "TVM Stored", "TVM Calculated", "TVM Not Found", "TVM Cleared", "TVM Reset",
        "VARS",
        "LINF", "LOGF", "EXPF", "PWRF",
        "Err: Division By 0", "Err: Domain", "Err: Overflow", "Err: Stat",
        "Err: Data Type", "Err: Unit Mismatch", "Err: Math", "Err: Index",
        "Err: Word Size", "Err: Syntax", "Err: Invalid", "Err: Clock",
        "1,234.5678901234", "DZ{9999,12,31,23,59,59,-23,-45}",
    };
    const unsigned message_count = sizeof messages / sizeof messages[0];
    for (unsigned i = 0; i < message_count + TINT_COUNT; i++) {
        c.message = i < message_count ? messages[i] : display_tint_name((uint8_t)(i - message_count));
        large_message_chars = bad_status_scale = arrow_pixels = bad_arrow_size = 0;
        draw(&c, &m, &cat, NULL, false);
        check(c.message, large_message_chars == strlen(c.message) &&
              !bad_status_scale && arrow_pixels && !bad_arrow_size && text_config == gfx_text_clip);
    }
    display_close();
}

int main(void)
{
    FILE *f = fopen("data/rpnCEres.bin", "rb");
    if (!f) return 1;
    mock_res_size = fread(mock_res, 1, sizeof mock_res, f);
    fclose(f);
    if (!res_open()) return 1;
    appearance();
    formatting_work();
    quantity_catalog_and_help();
    application_clock();
    number_mode_status();
    warning_size();
    message_acknowledgement();
    printf("%d failures\n", fails);
    return fails != 0;
}
