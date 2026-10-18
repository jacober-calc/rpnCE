#ifndef RPNCE_UI_DOTFONT_H
#define RPNCE_UI_DOTFONT_H

#include <stdint.h>

/*
 * A bold 5x7 LCD font for the stack, in the style of the SwissMicros
 * calculators: each font pixel is a solid block, a little taller than it is
 * wide, with no gaps. Characters are proportional: digits and letters are 5
 * columns wide, the decimal point and comma 1. The E of an exponent is drawn
 * small and centered, HP style.
 */

#define DOT_PITCH_X 3                       /* block width */
#define DOT_PITCH_Y 4                       /* block height */
#define DOT_ROWS    7
#define DOT_HEIGHT  (DOT_ROWS * DOT_PITCH_Y)

/* Width in pixels of a string. */
int dot_width(const char *s);

/* The same font with smaller blocks (2 x 2 pixels: half the height), for
 * labels beside the stack values. */
#define DOT_SMALL_PITCH  2
#define DOT_SMALL_HEIGHT (DOT_ROWS * DOT_SMALL_PITCH)
void dot_draw_small(const char *s, int x, int y, uint8_t color);

/* Draws a string with its top-left corner at (x, y). Dots left of
 * clip_left are not drawn (long entries scroll off to the left). */
void dot_draw(const char *s, int x, int y, uint8_t color, int clip_left);

#endif
