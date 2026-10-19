/* Drawing commands are recorded by test_dotfont.c instead of a screen. */
#ifndef MOCK_GRAPHX_H
#define MOCK_GRAPHX_H
#include <stddef.h>
#include <stdint.h>
uint8_t gfx_SetColor(uint8_t color);
void gfx_FillRectangle(int x, int y, int width, int height);
#endif
