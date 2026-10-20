#include <sys/lcd.h>
#include <ti/ui.h>
#include "brightness.h"

void sys_brightness_adjust(bool brighter)
{
    /* CE backlight: 0 is brightest, 255 darkest. Stop short of black. */
    int level = lcd_BacklightLevel + (brighter ? -8 : 8);
    if (level < 0) level = 0;
    if (level > 224) level = 224;
    lcd_BacklightLevel = (uint8_t)level;
    os_Brightness = (uint8_t)level;
}
