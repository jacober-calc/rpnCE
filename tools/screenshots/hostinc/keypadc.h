/* Screenshot engine: the ON key, driven by the script. */
#ifndef SHOT_KEYPADC_H
#define SHOT_KEYPADC_H
#include <stdbool.h>
extern bool shot_on_key;
#define kb_On shot_on_key
static inline void kb_DisableOnLatch(void) { }
#endif
