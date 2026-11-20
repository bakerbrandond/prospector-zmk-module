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

enum pet_mood pet_mood_of(int wpm, uint32_t idle_ms, bool boost) {
    if (idle_ms >= PET_SLEEP_IDLE_MS)
        return PET_ASLEEP;
    enum pet_mood m = wpm >= PET_EXCITED_WPM ? PET_EXCITED
                      : wpm >= PET_HAPPY_WPM ? PET_HAPPY
                      : wpm > 0              ? PET_CALM
                                             : PET_SLEEPY;
    if (boost && m < PET_EXCITED)
        m = (enum pet_mood)(m + 1);
    return m;
}

bool pet_save_due(uint32_t since_save_ms, bool dirty) {
    return dirty && since_save_ms >= PET_SAVE_EVERY_MS;
}

static void put32(uint8_t *b, uint32_t v) {
    for (int i = 0; i < 4; i++)
        b[i] = (uint8_t)(v >> (8 * i));
}

static uint32_t get32(const uint8_t *b) {
    return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

void pet_pack(const struct pet_state *p, uint8_t out[PET_BLOB_LEN]) {
    out[0] = PET_BLOB_VERSION;
    put32(out + 1, p->keys);
    put32(out + 5, p->growth_half);
    put32(out + 9, p->fullness_ms);
    out[13] = p->shown_stage;
}

bool pet_unpack(const uint8_t *buf, size_t len, struct pet_state *out) {
    if (len != PET_BLOB_LEN || buf[0] != PET_BLOB_VERSION)
        return false;
    out->keys = get32(buf + 1);
    out->growth_half = get32(buf + 5);
    uint32_t full = get32(buf + 9);
    out->fullness_ms = full > PET_FULL_MS ? PET_FULL_MS : full;
    out->shown_stage = buf[13] > PET_ADULT ? PET_ADULT : buf[13];
    return true;
}
