/* Font snapshots cover both sizes, all non-NUL bytes, exponents and clipping. */
#include <inttypes.h>
#include <stdio.h>
#include "dotfont.h"

static uint64_t fingerprint;
static int fails;

static void record(uint32_t value)
{
    for (int i = 0; i < 4; i++) {
        fingerprint ^= (uint8_t)(value >> (i * 8));
        fingerprint *= UINT64_C(1099511628211);
    }
}

uint8_t gfx_SetColor(uint8_t color)
{
    record(color);
    return 0;
}

void gfx_FillRectangle(int x, int y, int width, int height)
{
    record((uint32_t)x);
    record((uint32_t)y);
    record((uint32_t)width);
    record((uint32_t)height);
}

static void draw(const char *s, int x, int y, int clip, int small)
{
    record((uint32_t)dot_width(s));
    if (small)
        dot_draw_small(s, x, y, 4);
    else
        dot_draw(s, x, y, 4, clip);
}

static void check(const char *name, uint64_t want)
{
    int pass = fingerprint == want;
    if (!pass) fails++;
    printf("%s %s: %016" PRIx64 "\n", pass ? "ok  " : "FAIL", name, fingerprint);
}

int main(void)
{
    for (int small = 0; small < 2; small++) {
        fingerprint = UINT64_C(14695981039346656037);
        for (int ch = 1; ch < 256; ch++) {
            char text[] = { (char)ch, '\0' };
            draw(text, 6, 11, 0, small);
            /* A digit before E changes its size; a word before it doesn't. */
            char numeric[] = { '1', (char)ch, 'E', '-', '3', '\0' };
            char word[] = { 'E', 'N', (char)ch, 'E', '\0' };
            draw(numeric, -7, 19, 26, small);
            draw(word, 26, 23, 32, small);
        }
        check(small ? "small font, all bytes and contexts" : "stack font, all bytes and contexts",
              small ? UINT64_C(0xfb96e9d04cea1a35) : UINT64_C(0xeaf70b7e5db9024d));
    }

    static const char *const examples[] = {
        "", "1,234,567.89012", "-1.23456789012E-99", "ENG", "1E3 E 2.E-5",
        "T: Z: Y: X:", "D{2026-10-18}", "DZ{2026-10-18T09:30:00-07:00}",
        "11111111111111111111111111111111111111111111", "STO\x0b A", "\x05x\x01"
    };
    fingerprint = UINT64_C(14695981039346656037);
    for (unsigned i = 0; i < sizeof examples / sizeof examples[0]; i++)
        for (int x = -48; x <= 32; x += 8)
            for (int clip = 0; clip <= 104; clip += 26) {
                draw(examples[i], x, 48, clip, 0);
                draw(examples[i], x, 48, clip, 1);
            }
    check("mixed strings at clipping boundaries", UINT64_C(0xfe7263e3d6a24c2a));
    static const char *const complex_examples[] = {
        "3.0000\x80" "4.0000", "5.\x81" "0.9273", "5.\x81\x82" "53.1301"
    };
    fingerprint = UINT64_C(14695981039346656037);
    for (unsigned i = 0; i < sizeof complex_examples / sizeof *complex_examples; i++)
        for (int clip = 0; clip <= 104; clip += 26) {
            draw(complex_examples[i], -12, 48, clip, 0);
            draw(complex_examples[i], -12, 48, clip, 1);
        }
    check("complex symbols in both sizes at clipping boundaries", UINT64_C(0x438224abccc3cc67));
    int spacing = dot_width("2\x80" "3") == 54 &&
                  dot_width("2\x81" "3") == 57 &&
                  dot_width("2\x81\x82" "3") == 69 && dot_width("2.3") == 39;
    if (!spacing) fails++;
    printf("%s complex separators have twice the decimal-point gap\n", spacing ? "ok  " : "FAIL");
    printf("%d failures\n", fails);
    return fails != 0;
}
