/* Drawing commands are recorded by font and display tests. */
#ifndef MOCK_GRAPHX_H
#define MOCK_GRAPHX_H
#include <stddef.h>
#include <stdint.h>
uint8_t gfx_SetColor(uint8_t color);
void gfx_FillRectangle(int x, int y, int width, int height);
extern uint16_t gfx_palette[256];
enum { gfx_text_clip = 0 };
void gfx_Begin(void);
void gfx_End(void);
void gfx_SetDrawBuffer(void);
void gfx_SwapDraw(void);
void gfx_FillScreen(uint8_t color);
void gfx_HorizLine(int x, int y, int width);
uint16_t gfx_RGBTo1555(uint8_t r, uint8_t g, uint8_t b);
uint8_t gfx_SetTextFGColor(uint8_t color);
uint8_t gfx_SetTextBGColor(uint8_t color);
uint8_t gfx_SetTextTransparentColor(uint8_t color);
void gfx_SetTextScale(uint8_t x, uint8_t y);
void gfx_SetTextXY(int x, int y);
void gfx_SetTextConfig(uint8_t config);
void gfx_SetClipRegion(int xmin, int ymin, int xmax, int ymax);
unsigned int gfx_GetCharWidth(char c);
unsigned int gfx_GetStringWidth(const char *s);
void gfx_PrintChar(char c);
void gfx_PrintStringXY(const char *s, int x, int y);
#endif
