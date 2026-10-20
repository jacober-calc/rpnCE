/* Screenshot engine: the subset of graphx that RPNCE uses, drawing into an
 * in-memory framebuffer instead of the LCD (see graphx_raster.c). */
#ifndef SHOT_GRAPHX_H
#define SHOT_GRAPHX_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint16_t gfx_palette[256];
enum { gfx_text_clip = 1, gfx_text_noclip };

#define gfx_RGBTo1555(r,g,b) \
((uint16_t)(((((r) & 255) >> 3) << 10) | ((((g) & 255) >> 3) << 5) | (((b) & 255) >> 3)))

void gfx_Begin(void);
void gfx_End(void);
void gfx_SetDrawBuffer(void);
void gfx_SwapDraw(void);
uint8_t gfx_SetColor(uint8_t index);
void gfx_FillScreen(uint8_t index);
void gfx_FillRectangle(int x, int y, int width, int height);
void gfx_HorizLine(int x, int y, int length);
uint8_t gfx_SetTextFGColor(uint8_t color);
uint8_t gfx_SetTextBGColor(uint8_t color);
uint8_t gfx_SetTextTransparentColor(uint8_t color);
void gfx_SetTextScale(uint8_t width_scale, uint8_t height_scale);
void gfx_SetTextXY(int x, int y);
void gfx_SetTextConfig(uint8_t config);
void gfx_SetClipRegion(int xmin, int ymin, int xmax, int ymax);
unsigned int gfx_GetCharWidth(const char c);
unsigned int gfx_GetStringWidth(const char *string);
void gfx_PrintChar(const char c);
void gfx_PrintStringXY(const char *string, int x, int y);

/* engine: the frame most recently shown (after gfx_SwapDraw) */
const uint8_t *shot_front(void);
#endif
