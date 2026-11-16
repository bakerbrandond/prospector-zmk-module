/* Host tests for the Zappy Birds game core. Run: make -C tests/zappy test */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "zappy_core.h"

#define FP(px) ((int32_t)(px) << 8)
#define GAP_TOP_MIN ZAPPY_MARGIN
#define GAP_TOP_MAX (ZAPPY_H - ZAPPY_MARGIN - ZAPPY_GAP_H)

static struct zappy_game g;
static uint32_t now;

static void start(uint16_t best) {
    zappy_init(&g, best, 0, 1, NULL);
    now = 0;
    assert(zappy_press(&g, now, 0));
}

static void step(void) {
    now += ZAPPY_TICK_MS;
    zappy_step(&g, now);
}

static void crash_on_floor(void) {
    for (int i = 0; i < 500 && g.phase == ZAPPY_PLAYING; i++) {
        /* keep pylons out of the way so only the floor can kill */
        for (int p = 0; p < ZAPPY_NUM_PYLONS; p++)
            g.pylons[p].x = ZAPPY_W + 100;
        step();
    }
    assert(g.phase == ZAPPY_CRASHED);
}

/* hold the bird mid-gap of whichever pylon is nearest ahead of/at it */
static void hold_in_gap(void) {
    int best = -1;
    for (int p = 0; p < ZAPPY_NUM_PYLONS; p++) {
        if (g.pylons[p].x + ZAPPY_PYLON_W < ZAPPY_BIRD_X)
            continue;
        if (best < 0 || g.pylons[p].x < g.pylons[best].x)
            best = p;
    }
    assert(best >= 0);
    g.bird_y_fp = FP(g.pylons[best].gap_top + g.gap_h / 2 - ZAPPY_BIRD_SIZE / 2);
    g.vel_fp = 0;
}

static void test_init_title(void) {
    zappy_init(&g, 7, 0, 1, NULL);
    assert(g.phase == ZAPPY_TITLE);
    assert(g.best == 7);
    assert(g.score == 0);
}

static void test_press_starts_game(void) {
    zappy_init(&g, 0, 0, 1, NULL);
    assert(zappy_press(&g, 0, 0));
    assert(g.phase == ZAPPY_PLAYING);
    assert(g.vel_fp < 0);
}

static void test_gravity_pulls_down(void) {
    start(0);
    for (int i = 0; i < 10; i++)
        step();
    int16_t y10 = zappy_bird_y(&g);
    for (int i = 0; i < 20; i++)
        step();
    assert(g.phase == ZAPPY_PLAYING);
    assert(zappy_bird_y(&g) > y10);
}

static void test_flap_moves_up(void) {
    start(0);
    for (int i = 0; i < 20; i++)
        step();
    assert(g.phase == ZAPPY_PLAYING);
    int16_t y = zappy_bird_y(&g);
    assert(zappy_press(&g, now, 0));
    for (int i = 0; i < 3; i++)
        step();
    assert(zappy_bird_y(&g) < y);
}

static void test_ceiling_clamps(void) {
    start(0);
    for (int i = 0; i < 40; i++) {
        zappy_press(&g, now, 0);
        step();
        assert(zappy_bird_y(&g) >= 0);
    }
    assert(g.phase == ZAPPY_PLAYING);
}

static void test_floor_crashes(void) {
    start(0);
    crash_on_floor();
}

static void test_pylon_crash(void) {
    start(0);
    g.bird_y_fp = FP(ZAPPY_H / 2);
    g.vel_fp = 0;
    g.pylons[0].x = ZAPPY_BIRD_X;
    g.pylons[0].gap_top = ZAPPY_MARGIN; /* gap ends at 116, bird at 140 */
    step();
    assert(g.phase == ZAPPY_CRASHED);
}

static void test_score_once_per_pylon(void) {
    start(0);
    g.pylons[0].x = ZAPPY_BIRD_X + 1 - ZAPPY_PYLON_W; /* right edge 1px right of bird x */
    g.pylons[0].gap_top = 100;
    for (int p = 1; p < ZAPPY_NUM_PYLONS; p++)
        g.pylons[p].x = ZAPPY_W + 100;
    for (int i = 0; i < 5; i++) {
        hold_in_gap();
        step();
    }
    assert(g.phase == ZAPPY_PLAYING);
    assert(g.score == 1);
    for (int i = 0; i < 10; i++) {
        hold_in_gap();
        step();
    }
    assert(g.score == 1);
}

static void test_crash_lockout(void) {
    start(0);
    crash_on_floor();
    uint32_t c = g.crash_ms;
    assert(!zappy_press(&g, c + ZAPPY_CRASH_LOCKOUT_MS - 1, 0));
    assert(g.phase == ZAPPY_CRASHED);
    assert(zappy_press(&g, c + ZAPPY_CRASH_LOCKOUT_MS, 0));
    assert(g.phase == ZAPPY_PLAYING);
    assert(g.score == 0);
}

static void test_new_best(void) {
    start(0);
    g.score = 3;
    crash_on_floor();
    assert(g.best == 3);
    assert(g.new_best);
    assert(zappy_press(&g, g.crash_ms + ZAPPY_CRASH_LOCKOUT_MS, 0));
    assert(!g.new_best);
    assert(g.best == 3);
}

static void test_no_new_best_when_lower(void) {
    start(10);
    g.score = 3;
    crash_on_floor();
    assert(g.best == 10);
    assert(!g.new_best);
}

static void test_long_run_stays_in_bounds(void) {
    start(0);
    for (long i = 0; i < 100000; i++) {
        hold_in_gap();
        step();
        assert(g.phase == ZAPPY_PLAYING);
        for (int p = 0; p < ZAPPY_NUM_PYLONS; p++) {
            assert(g.pylons[p].gap_top >= GAP_TOP_MIN);
            assert(g.pylons[p].gap_top <= GAP_TOP_MAX);
            assert(g.pylons[p].x >= -ZAPPY_PYLON_W);
            assert(g.pylons[p].x <= ZAPPY_W + 2 * ZAPPY_W);
        }
    }
    assert(g.score > 255);
    assert(g.score <= ZAPPY_SCORE_MAX);
}

static void test_deterministic_seed(void) {
    struct zappy_game a, b;
    zappy_init(&a, 0, 0, 42, NULL);
    zappy_init(&b, 0, 0, 42, NULL);
    zappy_press(&a, 0, 0);
    zappy_press(&b, 0, 0);
    for (int i = 0; i < 2000; i++) {
        uint32_t t = (uint32_t)(i + 1) * ZAPPY_TICK_MS;
        a.bird_y_fp = b.bird_y_fp = FP(ZAPPY_H / 2);
        a.vel_fp = b.vel_fp = 0;
        a.phase = b.phase = ZAPPY_PLAYING; /* collisions don't matter here */
        zappy_step(&a, t);
        zappy_step(&b, t);
    }
    for (int p = 0; p < ZAPPY_NUM_PYLONS; p++)
        assert(a.pylons[p].gap_top == b.pylons[p].gap_top);
}

static void test_exit_needs_pause_after_flap(void) {
    /* combo exit only counts after ZAPPY_EXIT_IDLE_MS without a flap */
    assert(!zappy_exit_allowed(1000, 1000));
    assert(!zappy_exit_allowed(1000, 1001)); /* flap stamped after now */
    assert(!zappy_exit_allowed(1000 + ZAPPY_EXIT_IDLE_MS - 1, 1000));
    assert(zappy_exit_allowed(1000 + ZAPPY_EXIT_IDLE_MS, 1000));
    /* uptime wrap */
    assert(zappy_exit_allowed(ZAPPY_EXIT_IDLE_MS - 10, 0xFFFFFFF0u));
}

static void test_steps_follow_real_time(void) {
    /* 40 ms timer callbacks must still average one step per ZAPPY_TICK_MS */
    uint32_t acc = 0, steps = 0;
    for (int i = 0; i < 1000; i++)
        steps += zappy_steps_due(&acc, 40);
    assert(steps == 40000 / ZAPPY_TICK_MS);
    /* a long stall is capped so the game doesn't jump ahead */
    acc = 0;
    assert(zappy_steps_due(&acc, 5000) == ZAPPY_MAX_CATCHUP_STEPS);
    assert(acc < ZAPPY_TICK_MS);
}

static void test_idle_ignores_input_newer_than_now(void) {
    /* a flap stamped just after the timer read the clock is not idle */
    assert(!zappy_idle_expired(1000, 1001));
    assert(!zappy_idle_expired(1000, 1000 + 50));
    assert(!zappy_idle_expired(1000 + ZAPPY_IDLE_EXIT_MS, 1000));
    assert(zappy_idle_expired(1000 + ZAPPY_IDLE_EXIT_MS + 1, 1000));
    /* uptime wrap */
    assert(zappy_idle_expired(ZAPPY_IDLE_EXIT_MS, 0xFFFFFFF0u));
}

#define USAGE(id) ((0x07u << 16) | (id))
#define KEY_A 0x04
#define KEY_H 0x0B
#define KEY_Q 0x14
#define KEY_W 0x1A

static struct zappy_pool pool_of(int n, const uint8_t *pos, const uint32_t *usage) {
    struct zappy_pool p = {0};
    p.h_pos = ZAPPY_NO_POS;
    for (int i = 0; i < n; i++)
        zappy_pool_add(&p, pos[i], usage[i]);
    return p;
}

static struct zappy_pool qwh_pool(void) {
    static const uint8_t pos[] = {1, 2, 19};
    static const uint32_t use[] = {USAGE(KEY_Q), USAGE(KEY_W), USAGE(KEY_H)};
    return pool_of(3, pos, use);
}

static void start_hard(uint16_t best, uint16_t best_hard) {
    struct zappy_pool p = qwh_pool();
    zappy_init(&g, best, best_hard, 7, &p);
    now = 0;
    assert(zappy_press(&g, now, p.h_pos));
    assert(g.mode == ZAPPY_HARD);
}

static uint8_t target_pos(void) { return g.pool.pos[g.target]; }

static void test_usage_letter(void) {
    assert(zappy_usage_letter(USAGE(0x04)) == 'A');
    assert(zappy_usage_letter(USAGE(0x1D)) == 'Z');
    assert(zappy_usage_letter((0x02u << 24) | USAGE(KEY_H)) == 'H'); /* mods ignored */
    assert(zappy_usage_letter(USAGE(0x2C)) == 0);                   /* space */
    assert(zappy_usage_letter((0x0Cu << 16) | 0x04) == 0);           /* consumer page */
}

static void test_pool_add(void) {
    static const uint8_t pos[] = {1, 13, 19, 37};
    static const uint32_t use[] = {USAGE(KEY_Q), USAGE(KEY_A), USAGE(KEY_H), USAGE(0x2C)};
    struct zappy_pool p = pool_of(4, pos, use);
    assert(p.n == 3);
    assert(p.letter[2] == 'H');
    assert(p.h_pos == 19);
}

static void test_h_starts_hard(void) {
    struct zappy_pool p = qwh_pool();
    zappy_init(&g, 0, 0, 1, &p);
    assert(zappy_press(&g, 0, 19));
    assert(g.mode == ZAPPY_HARD);
    assert(g.phase == ZAPPY_PLAYING);
    assert(zappy_target_letter(&g) != 0);
}

static void test_other_key_starts_normal(void) {
    struct zappy_pool p = qwh_pool();
    zappy_init(&g, 0, 0, 1, &p);
    assert(zappy_press(&g, 0, 1));
    assert(g.mode == ZAPPY_NORMAL);
    assert(zappy_target_letter(&g) == 0);
}

/* H pressed a moment after another key (a roll) still picks hard mode. */
static void test_h_just_after_other_key_starts_hard(void) {
    struct zappy_pool p = qwh_pool();
    zappy_init(&g, 0, 0, 1, &p);
    assert(zappy_press(&g, 1000, 1));
    assert(g.mode == ZAPPY_NORMAL);
    assert(zappy_press(&g, 1000 + ZAPPY_HARD_GRACE_MS - 1, 19));
    assert(g.mode == ZAPPY_HARD);
    assert(g.phase == ZAPPY_PLAYING);
    assert(g.score == 0);
    assert(zappy_target_letter(&g) != 0);
}

/* After the grace period, H is just a flap in a normal run. */
static void test_h_late_stays_normal(void) {
    struct zappy_pool p = qwh_pool();
    zappy_init(&g, 0, 0, 1, &p);
    assert(zappy_press(&g, 1000, 1));
    assert(zappy_press(&g, 1000 + ZAPPY_HARD_GRACE_MS, 19));
    assert(g.mode == ZAPPY_NORMAL);
}

static void test_hard_needs_two_letters(void) {
    static const uint8_t pos[] = {19};
    static const uint32_t use[] = {USAGE(KEY_H)};
    struct zappy_pool p = pool_of(1, pos, use);
    zappy_init(&g, 0, 0, 1, &p);
    assert(zappy_press(&g, 0, 19));
    assert(g.mode == ZAPPY_NORMAL);
}

static void test_wrong_key_ignored(void) {
    start_hard(0, 0);
    for (int i = 0; i < 10; i++)
        step();
    int32_t vel = g.vel_fp;
    uint8_t t = g.target;
    uint8_t wrong = g.pool.pos[(g.target + 1) % g.pool.n];
    assert(!zappy_press(&g, now, wrong));
    assert(!zappy_press(&g, now, 40)); /* not in the pool at all */
    assert(g.vel_fp == vel);
    assert(g.target == t);
}

static void test_right_key_flaps_and_moves_target(void) {
    start_hard(0, 0);
    bool seen[ZAPPY_MAX_POOL] = {0};
    for (int i = 0; i < 200; i++) {
        g.bird_y_fp = FP(ZAPPY_H / 2);
        g.vel_fp = 0;
        uint8_t before = g.target;
        assert(zappy_press(&g, now, target_pos()));
        assert(g.vel_fp < 0);
        assert(g.target != before);
        assert(g.target < g.pool.n);
        seen[g.target] = true;
    }
    for (int i = 0; i < g.pool.n; i++)
        assert(seen[i]);
}

static void test_hard_restart_stays_hard(void) {
    start_hard(0, 0);
    crash_on_floor();
    uint32_t c = g.crash_ms;
    assert(!zappy_press(&g, c + ZAPPY_CRASH_LOCKOUT_MS - 1, target_pos()));
    assert(zappy_press(&g, c + ZAPPY_CRASH_LOCKOUT_MS, 40));
    assert(g.mode == ZAPPY_HARD);
    assert(g.phase == ZAPPY_PLAYING);
}

static void test_hard_best_separate(void) {
    start_hard(5, 2);
    g.score = 3;
    crash_on_floor();
    assert(g.best_hard == 3);
    assert(g.best == 5);
    assert(g.new_best);
}

static void test_hard_gap_wider(void) {
    start_hard(0, 0);
    assert(g.gap_h == ZAPPY_GAP_H + 24);
    for (long i = 0; i < 20000; i++) {
        hold_in_gap();
        step();
        assert(g.phase == ZAPPY_PLAYING);
        for (int p = 0; p < ZAPPY_NUM_PYLONS; p++) {
            assert(g.pylons[p].gap_top >= ZAPPY_MARGIN);
            assert(g.pylons[p].gap_top <= ZAPPY_H - ZAPPY_MARGIN - g.gap_h);
        }
    }
}

/* The playfield is the landscape screen LVGL sees (280x240). */
static void test_field_is_screen(void) {
    assert(ZAPPY_W == DISPLAY_W);
    assert(ZAPPY_H == DISPLAY_H);
}

int main(void) {
    int n = 0;
#define RUN(t) (t(), n++)
    RUN(test_init_title);
    RUN(test_press_starts_game);
    RUN(test_gravity_pulls_down);
    RUN(test_flap_moves_up);
    RUN(test_ceiling_clamps);
    RUN(test_floor_crashes);
    RUN(test_pylon_crash);
    RUN(test_score_once_per_pylon);
    RUN(test_crash_lockout);
    RUN(test_new_best);
    RUN(test_no_new_best_when_lower);
    RUN(test_long_run_stays_in_bounds);
    RUN(test_deterministic_seed);
    RUN(test_exit_needs_pause_after_flap);
    RUN(test_steps_follow_real_time);
    RUN(test_idle_ignores_input_newer_than_now);
    RUN(test_usage_letter);
    RUN(test_pool_add);
    RUN(test_h_starts_hard);
    RUN(test_other_key_starts_normal);
    RUN(test_h_just_after_other_key_starts_hard);
    RUN(test_h_late_stays_normal);
    RUN(test_hard_needs_two_letters);
    RUN(test_wrong_key_ignored);
    RUN(test_right_key_flaps_and_moves_target);
    RUN(test_hard_restart_stays_hard);
    RUN(test_hard_best_separate);
    RUN(test_hard_gap_wider);
    RUN(test_field_is_screen);
    printf("ok %d tests\n", n);
    return 0;
}
