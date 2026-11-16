/*
 * Zappy Birds game rules: fixed-point integer physics, one call to
 * zappy_step() per ZAPPY_TICK_MS frame.
 */
#include "zappy_core.h"

#define FP(px) ((int32_t)(px) << 8)

/* Feel constants, per tick (24.8 fixed point). */
#define GRAVITY_FP 115     /* ~0.45 px/tick^2 */
#define HARD_GRAVITY_FP 80 /* ~70%: time to find the next key */
#define FLAP_VEL_FP (-1600) /* ~-6.25 px/tick */
#define MAX_FALL_FP 2304   /* 9 px/tick */
#define SCROLL_PX 2        /* ~60 px/s */
#define FLAP_FRAMES 4

#define PYLON_SPACING (ZAPPY_W / ZAPPY_NUM_PYLONS + ZAPPY_PYLON_W)
#define FIRST_PYLON_X (ZAPPY_W + 40)
#define GAP_TOP_MIN ZAPPY_MARGIN

#define BOB_PERIOD 48 /* ticks */
#define BOB_AMPL 6    /* px */

static uint32_t next_rand(struct zappy_game *g) {
    uint32_t x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

static int16_t random_gap_top(struct zappy_game *g) {
    int32_t range = ZAPPY_H - ZAPPY_MARGIN - g->gap_h - GAP_TOP_MIN + 1;
    return (int16_t)(GAP_TOP_MIN + (int32_t)(next_rand(g) % (uint32_t)range));
}

static void pick_target(struct zappy_game *g) {
    /* uniform over the other n-1 letters, so it never repeats */
    uint8_t skip = (uint8_t)(1 + next_rand(g) % (uint32_t)(g->pool.n - 1));
    g->target = (uint8_t)((g->target + skip) % g->pool.n);
}

char zappy_usage_letter(uint32_t usage) {
    uint32_t page = (usage >> 16) & 0xFF, id = usage & 0xFFFF;
    if (page != 0x07 || id < 0x04 || id > 0x1D)
        return 0;
    return (char)('A' + (id - 0x04));
}

void zappy_pool_add(struct zappy_pool *p, uint8_t position, uint32_t usage) {
    char c = zappy_usage_letter(usage);
    if (!c || p->n >= ZAPPY_MAX_POOL)
        return;
    p->pos[p->n] = position;
    p->letter[p->n] = c;
    p->n++;
    if (c == 'H')
        p->h_pos = position;
}

char zappy_target_letter(const struct zappy_game *g) {
    return g->mode == ZAPPY_HARD ? g->pool.letter[g->target] : 0;
}

static int32_t start_y_fp(void) { return FP(ZAPPY_H / 2 - ZAPPY_BIRD_SIZE / 2); }

static void flap(struct zappy_game *g) {
    g->vel_fp = FLAP_VEL_FP;
    g->flap_frames = FLAP_FRAMES;
}

static void start_run(struct zappy_game *g, enum zappy_mode mode) {
    g->mode = mode;
    g->gap_h = mode == ZAPPY_HARD ? ZAPPY_HARD_GAP_H : ZAPPY_GAP_H;
    if (mode == ZAPPY_HARD)
        pick_target(g);
    g->phase = ZAPPY_PLAYING;
    g->score = 0;
    g->new_best = false;
    g->bird_y_fp = start_y_fp();
    for (int i = 0; i < ZAPPY_NUM_PYLONS; i++) {
        g->pylons[i].x = (int16_t)(FIRST_PYLON_X + i * PYLON_SPACING);
        g->pylons[i].gap_top = random_gap_top(g);
        g->pylons[i].scored = false;
    }
    flap(g);
}

void zappy_init(struct zappy_game *g, uint16_t best, uint16_t best_hard, uint32_t seed,
                const struct zappy_pool *pool) {
    *g = (struct zappy_game){0};
    g->phase = ZAPPY_TITLE;
    g->best = best;
    g->best_hard = best_hard;
    g->gap_h = ZAPPY_GAP_H;
    g->pool.h_pos = ZAPPY_NO_POS;
    if (pool)
        g->pool = *pool;
    g->rng = seed ? seed : 0x5eed1234u;
    g->bird_y_fp = start_y_fp();
}

static bool is_hard_key(const struct zappy_game *g, uint8_t position) {
    return position == g->pool.h_pos && g->pool.h_pos != ZAPPY_NO_POS && g->pool.n >= 2;
}

bool zappy_press(struct zappy_game *g, uint32_t now_ms, uint8_t position) {
    switch (g->phase) {
    case ZAPPY_TITLE:
        start_run(g, is_hard_key(g, position) ? ZAPPY_HARD : ZAPPY_NORMAL);
        g->run_start_ms = now_ms;
        return true;
    case ZAPPY_PLAYING:
        /* a roll onto H right after starting: the player meant hard mode */
        if (g->mode == ZAPPY_NORMAL && is_hard_key(g, position) &&
            (uint32_t)(now_ms - g->run_start_ms) < ZAPPY_HARD_GRACE_MS) {
            start_run(g, ZAPPY_HARD);
            return true;
        }
        if (g->mode == ZAPPY_HARD) {
            if (position != g->pool.pos[g->target])
                return false; /* wrong key: no flap */
            pick_target(g);
        }
        flap(g);
        return true;
    case ZAPPY_CRASHED:
        if ((uint32_t)(now_ms - g->crash_ms) < ZAPPY_CRASH_LOCKOUT_MS)
            return false;
        start_run(g, g->mode);
        return true;
    }
    return false;
}

static void crash(struct zappy_game *g, uint32_t now_ms) {
    g->phase = ZAPPY_CRASHED;
    g->crash_ms = now_ms;
    uint16_t *best = g->mode == ZAPPY_HARD ? &g->best_hard : &g->best;
    if (g->score > *best) {
        *best = g->score;
        g->new_best = true;
    }
}

static int16_t rightmost_x(const struct zappy_game *g) {
    int16_t x = g->pylons[0].x;
    for (int i = 1; i < ZAPPY_NUM_PYLONS; i++)
        if (g->pylons[i].x > x)
            x = g->pylons[i].x;
    return x;
}

static bool hits_pylon(const struct zappy_pylon *p, int16_t by, int16_t gap_h) {
    bool x_overlap = ZAPPY_BIRD_X + ZAPPY_BIRD_SIZE > p->x && ZAPPY_BIRD_X < p->x + ZAPPY_PYLON_W;
    if (!x_overlap)
        return false;
    return by < p->gap_top || by + ZAPPY_BIRD_SIZE > p->gap_top + gap_h;
}

void zappy_step(struct zappy_game *g, uint32_t now_ms) {
    g->ticks++;
    if (g->flap_frames)
        g->flap_frames--;

    if (g->phase == ZAPPY_TITLE) {
        /* triangle-wave bob around the start height */
        int t = g->ticks % BOB_PERIOD;
        int half = BOB_PERIOD / 2;
        int off = (t < half ? t : BOB_PERIOD - t) * 2 * BOB_AMPL / half - BOB_AMPL;
        g->bird_y_fp = start_y_fp() + FP(off);
        return;
    }
    if (g->phase != ZAPPY_PLAYING)
        return;

    /* bird */
    g->vel_fp += g->mode == ZAPPY_HARD ? HARD_GRAVITY_FP : GRAVITY_FP;
    if (g->vel_fp > MAX_FALL_FP)
        g->vel_fp = MAX_FALL_FP;
    g->bird_y_fp += g->vel_fp;
    if (g->bird_y_fp < 0) {
        g->bird_y_fp = 0;
        g->vel_fp = 0;
    }

    /* pylons: scroll, recycle, score */
    for (int i = 0; i < ZAPPY_NUM_PYLONS; i++) {
        struct zappy_pylon *p = &g->pylons[i];
        p->x -= SCROLL_PX;
        if (p->x + ZAPPY_PYLON_W < 0) {
            p->x = (int16_t)(rightmost_x(g) + PYLON_SPACING);
            p->gap_top = random_gap_top(g);
            p->scored = false;
        }
        if (!p->scored && p->x + ZAPPY_PYLON_W < ZAPPY_BIRD_X) {
            p->scored = true;
            if (g->score < ZAPPY_SCORE_MAX)
                g->score++;
        }
    }

    /* collisions */
    int16_t by = zappy_bird_y(g);
    if (by + ZAPPY_BIRD_SIZE >= ZAPPY_H) {
        crash(g, now_ms);
        return;
    }
    for (int i = 0; i < ZAPPY_NUM_PYLONS; i++) {
        if (hits_pylon(&g->pylons[i], by, g->gap_h)) {
            crash(g, now_ms);
            return;
        }
    }
}

int16_t zappy_bird_y(const struct zappy_game *g) { return (int16_t)(g->bird_y_fp >> 8); }

bool zappy_exit_allowed(uint32_t now_ms, uint32_t last_flap_ms) {
    return (int32_t)(now_ms - last_flap_ms) >= ZAPPY_EXIT_IDLE_MS;
}

uint8_t zappy_steps_due(uint32_t *acc, uint32_t elapsed_ms) {
    uint8_t n = 0;
    *acc += elapsed_ms;
    while (*acc >= ZAPPY_TICK_MS) {
        if (n == ZAPPY_MAX_CATCHUP_STEPS) {
            *acc = 0; /* drop the rest of a long stall */
            break;
        }
        *acc -= ZAPPY_TICK_MS;
        n++;
    }
    return n;
}

bool zappy_idle_expired(uint32_t now_ms, uint32_t last_input_ms) {
    return (int32_t)(now_ms - last_input_ms) > ZAPPY_IDLE_EXIT_MS;
}
