/* A faithful software version of the graphx calls rpnCE makes, following
 * graphx.asm (CE toolchain v15.0): two 320x240 8-bit buffers swapped by
 * gfx_SwapDraw, palette colors, and the default font drawn as graphx does
 * (each character's spacing-table width in columns, 8 rows, high bit first,
 * transparent pixels skipped). As in GraphX v15, the clipped text path
 * ignores text scaling; only the unclipped path repeats scaled pixels. */
#include <string.h>
#include "graphx.h"
#include "font.h"           /* generated from graphx.asm by run.py */

#define W 320
#define H 240

uint16_t gfx_palette[256];
static uint8_t buf[2][W * H];
static int back = 1;                     /* index of the draw buffer */
static uint8_t color, fg = 0, bg = 255, tp = 0;
static int sx = 1, sy = 1, tx, ty;
static bool clip_text;
static int cx0 = 0, cy0 = 0, cx1 = W, cy1 = H;

const uint8_t *shot_front(void) { return buf[1 - back]; }

void gfx_Begin(void) { memset(buf, 0, sizeof buf); back = 1; }
void gfx_End(void) { }
void gfx_SetDrawBuffer(void) { }
void gfx_SwapDraw(void) { back = 1 - back; }
uint8_t gfx_SetColor(uint8_t index) { uint8_t o = color; color = index; return o; }

static void put(int x, int y, uint8_t c)
{
    if (x >= 0 && x < W && y >= 0 && y < H)
        buf[back][y * W + x] = c;
}

void gfx_FillScreen(uint8_t index) { memset(buf[back], index, W * H); }

void gfx_FillRectangle(int x, int y, int w, int h)
{
    /* graphx clips shapes to the clip region */
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            if (i >= cx0 && i < cx1 && j >= cy0 && j < cy1)
                put(i, j, color);
}

void gfx_HorizLine(int x, int y, int length) { gfx_FillRectangle(x, y, length, 1); }

uint8_t gfx_SetTextFGColor(uint8_t c) { uint8_t o = fg; fg = c; return o; }
uint8_t gfx_SetTextBGColor(uint8_t c) { uint8_t o = bg; bg = c; return o; }
uint8_t gfx_SetTextTransparentColor(uint8_t c) { uint8_t o = tp; tp = c; return o; }
void gfx_SetTextScale(uint8_t w, uint8_t h) { sx = w; sy = h; }
void gfx_SetTextXY(int x, int y) { tx = x; ty = y; }
void gfx_SetTextConfig(uint8_t config) { clip_text = config == gfx_text_clip; }
void gfx_SetClipRegion(int x0, int y0, int x1, int y1) { cx0 = x0; cy0 = y0; cx1 = x1; cy1 = y1; }

unsigned int gfx_GetCharWidth(const char c) { return font_spacing[(uint8_t)c & 127] * sx; }

unsigned int gfx_GetStringWidth(const char *s)
{
    unsigned w = 0;
    for (; *s; s++)
        w += gfx_GetCharWidth(*s);
    return w;
}

void gfx_PrintChar(const char ch)
{
    uint8_t c = (uint8_t)ch;
    int width = font_spacing[c & 127];
    int draw_sx = clip_text ? 1 : sx, draw_sy = clip_text ? 1 : sy;
    const uint8_t *g = font_data[c & 127];
    for (int row = 0; row < 8; row++)
        for (int col = 0; col < width; col++) {
            uint8_t v = (g[row] & (0x80 >> col)) ? fg : bg;
            if (v == tp)
                continue;
            for (int a = 0; a < draw_sy; a++)
                for (int b = 0; b < draw_sx; b++) {
                    int px = tx + col * draw_sx + b, py = ty + row * draw_sy + a;
                    if (clip_text && (px < cx0 || px >= cx1 || py < cy0 || py >= cy1))
                        continue;
                    put(px, py, v);
                }
        }
    tx += width * draw_sx;
}

void gfx_PrintStringXY(const char *s, int x, int y)
{
    gfx_SetTextXY(x, y);
    for (; *s; s++)
        gfx_PrintChar(*s);
}
