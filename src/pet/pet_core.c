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

uint8_t pet_progress_pct(uint32_t growth_half) {
    enum pet_stage s = pet_stage_of(growth_half);
    if (s == PET_ADULT)
        return 100;
    uint32_t from = stage_at[s], to = stage_at[s + 1];
    return (uint8_t)((uint64_t)(growth_half / 2 - from) * 100 / (to - from));
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

bool pet_within(uint32_t start_ms, uint32_t now_ms, uint32_t dur_ms) {
    return now_ms - start_ms < dur_ms;
}

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

enum pet_side pet_side_of_position(uint8_t pos) {
    if (pos >= 36)
        return pos <= 38 ? PET_LEFT : PET_RIGHT;
    return pos % 12 < 6 ? PET_LEFT : PET_RIGHT;
}

static uint32_t xorshift(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}

static void new_round(struct pet_game *g, uint32_t now_ms) {
    g->phase = PET_GAME_ROUND;
    g->hide = (xorshift(&g->rng) >> 16) & 1 ? PET_RIGHT : PET_LEFT;
    g->phase_end_ms = now_ms + PET_ROUND_MS;
}

void pet_game_start(struct pet_game *g, uint32_t seed, uint32_t now_ms) {
    memset(g, 0, sizeof(*g));
    /* spread small seeds so consecutive ones don't start alike */
    g->rng = (seed * 2654435761u) | 1;
    xorshift(&g->rng);
    g->start_ms = now_ms;
    new_round(g, now_ms);
}

static void reveal(struct pet_game *g, bool hit, uint32_t now_ms) {
    g->last_hit = hit;
    if (hit)
        g->score++;
    g->round++;
    g->phase = PET_GAME_REVEAL;
    g->phase_end_ms = now_ms + PET_REVEAL_MS;
}

void pet_game_guess(struct pet_game *g, enum pet_side side, uint32_t now_ms) {
    if (g->phase == PET_GAME_ROUND)
        reveal(g, side == g->hide, now_ms);
}

/* deadline reached, wrap-safe */
static bool due(uint32_t now_ms, uint32_t end_ms) { return (int32_t)(now_ms - end_ms) >= 0; }

void pet_game_step(struct pet_game *g, uint32_t now_ms) {
    if (g->phase == PET_GAME_OFF)
        return;
    if (now_ms - g->start_ms >= PET_GAME_CAP_MS) {
        g->phase = PET_GAME_OFF;
        return;
    }
    if (!due(now_ms, g->phase_end_ms))
        return;
    switch (g->phase) {
    case PET_GAME_ROUND:
        reveal(g, false, now_ms);
        break;
    case PET_GAME_REVEAL:
        if (g->round >= PET_GAME_ROUNDS) {
            g->phase = PET_GAME_RESULT;
            g->phase_end_ms = now_ms + PET_RESULT_MS;
        } else {
            new_round(g, now_ms);
        }
        break;
    case PET_GAME_RESULT:
        g->phase = PET_GAME_OFF;
        break;
    default:
        break;
    }
}

bool pet_game_capturing(const struct pet_game *g) {
    return g->phase == PET_GAME_ROUND || g->phase == PET_GAME_REVEAL;
}

bool pet_game_won(const struct pet_game *g) {
    return g->phase == PET_GAME_RESULT && g->score >= PET_GAME_WIN;
}
