#pragma once

#include <stdint.h>

/* Backlight levels the brightness key cycles through (percent). */
#define BL_LEVELS {0, 5, 15, 30, 60, 100}

/* Next level after `current`: the first level above it, wrapping to 0 after
 * 100 (or anything out of range). */
uint8_t bl_next_level(uint8_t current);
