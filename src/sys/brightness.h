#ifndef RPNCE_SYS_BRIGHTNESS_H
#define RPNCE_SYS_BRIGHTNESS_H

#include <stdbool.h>

/* Adjust the shared TI-OS brightness, keeping a visible minimum level. */
void sys_brightness_adjust(bool brighter);

#endif
