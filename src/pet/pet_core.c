#include "pet_core.h"

#include <string.h>

/* growth (whole points) at which each stage starts */
static const uint32_t stage_at[PET_STAGES] = {0, 500, 10000, 100000, 1000000};

void pet_init(struct pet_state *p) {
    memset(p, 0, sizeof(*p));
    p->fullness_ms = PET_FULL_MS;
}

enum pet_stage pet_stage_of(uint32_t growth_half) {
    uint32_t growth = growth_half / 2;
    enum pet_stage s = PET_EGG;
    for (int i = 1; i < PET_STAGES; i++) {
        if (growth >= stage_at[i])
            s = (enum pet_stage)i;
    }
    return s;
}

uint32_t pet_next_threshold(uint32_t growth_half) {
    enum pet_stage s = pet_stage_of(growth_half);
    return s == PET_ADULT ? 0 : stage_at[s + 1];
}

bool pet_hungry(const struct pet_state *p) { return p->fullness_ms == 0; }

static uint32_t sat_add(uint32_t a, uint32_t b) { return a > UINT32_MAX - b ? UINT32_MAX : a + b; }

bool pet_add_presses(struct pet_state *p, uint32_t n) {
    enum pet_stage before = pet_stage_of(p->growth_half);
    uint32_t per = pet_hungry(p) ? 1 : 2;
    uint32_t gain = n > UINT32_MAX / 2 ? UINT32_MAX : n * per;
    p->keys = sat_add(p->keys, n);
    p->growth_half = sat_add(p->growth_half, gain);
    return pet_stage_of(p->growth_half) > before;
}

void pet_elapse(struct pet_state *p, uint32_t elapsed_ms) {
    p->fullness_ms = elapsed_ms >= p->fullness_ms ? 0 : p->fullness_ms - elapsed_ms;
}

void pet_feed(struct pet_state *p) { p->fullness_ms = PET_FULL_MS; }
