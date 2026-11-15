#include "bl_levels.h"

static const uint8_t levels[] = BL_LEVELS;
#define N_LEVELS (sizeof(levels) / sizeof(levels[0]))

uint8_t bl_next_level(uint8_t current) {
    for (unsigned i = 0; i < N_LEVELS; i++) {
        if (levels[i] > current)
            return levels[i];
    }
    return levels[0];
}
