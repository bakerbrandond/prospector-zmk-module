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
    zappy_init(&g, best, 1);
    now = 0;
    assert(zappy_press(&g, now));
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
    g.bird_y_fp = FP(g.pylons[best].gap_top + ZAPPY_GAP_H / 2 - ZAPPY_BIRD_SIZE / 2);
    g.vel_fp = 0;
}

static void test_init_title(void) {
    zappy_init(&g, 7, 1);
    assert(g.phase == ZAPPY_TITLE);
    assert(g.best == 7);
    assert(g.score == 0);
}

static void test_press_starts_game(void) {
    zappy_init(&g, 0, 1);
    assert(zappy_press(&g, 0));
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
    assert(zappy_press(&g, now));
    for (int i = 0; i < 3; i++)
        step();
    assert(zappy_bird_y(&g) < y);
}

static void test_ceiling_clamps(void) {
    start(0);
    for (int i = 0; i < 40; i++) {
        zappy_press(&g, now);
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
    assert(!zappy_press(&g, c + ZAPPY_CRASH_LOCKOUT_MS - 1));
    assert(g.phase == ZAPPY_CRASHED);
    assert(zappy_press(&g, c + ZAPPY_CRASH_LOCKOUT_MS));
    assert(g.phase == ZAPPY_PLAYING);
    assert(g.score == 0);
}

static void test_new_best(void) {
    start(0);
    g.score = 3;
    crash_on_floor();
    assert(g.best == 3);
    assert(g.new_best);
    assert(zappy_press(&g, g.crash_ms + ZAPPY_CRASH_LOCKOUT_MS));
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
    zappy_init(&a, 0, 42);
    zappy_init(&b, 0, 42);
    zappy_press(&a, 0);
    zappy_press(&b, 0);
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
    printf("ok %d tests\n", n);
    return 0;
}
