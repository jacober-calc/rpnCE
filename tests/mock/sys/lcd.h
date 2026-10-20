#ifndef MOCK_SYS_LCD_H
#define MOCK_SYS_LCD_H
#include <stdint.h>
extern uint8_t mock_backlight;
#define lcd_BacklightLevel mock_backlight
#endif
