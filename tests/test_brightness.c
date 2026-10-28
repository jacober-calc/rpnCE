#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/sys/brightness.h"

uint8_t mock_backlight, mock_brightness;

int main(void)
{
    int fails = 0;
    /* Across the hardware byte range, the setting stays visible, tracks
     * the LCD, and moves in the requested direction within that range. */
    for (int level = 0; level <= 255; level++)
        for (int up = 0; up <= 1; up++) {
            mock_backlight = (uint8_t)level;
            mock_brightness = 123;
            sys_brightness_adjust(up != 0);
            if (mock_backlight > 224 || mock_brightness != mock_backlight ||
                (up && mock_backlight > level) ||
                (!up && level <= 224 && mock_backlight < level))
                fails++;
        }
    mock_backlight = 100;
    sys_brightness_adjust(true);
    if (mock_backlight != 92) fails++;
    sys_brightness_adjust(false);
    if (mock_backlight != 100) fails++;
    mock_backlight = 100;
    for (int i = 0; i < 100; i++) sys_brightness_adjust(false);
    if (mock_backlight != 224) fails++;
    for (int i = 0; i < 100; i++) sys_brightness_adjust(true);
    if (mock_backlight != 0) fails++;
    printf("%d failures\n", fails);
    return fails != 0;
}
