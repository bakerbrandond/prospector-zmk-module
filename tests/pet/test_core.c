/* Host tests for the typing pet rules. Run: make -C tests/pet test */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pet_core.h"

static struct pet_state p;

static void fresh(void) { pet_init(&p); }

static void test_init_is_full_egg(void) {
    fresh();
    assert(p.keys == 0);
    assert(p.growth_half == 0);
    assert(p.fullness_ms == PET_FULL_MS);
    assert(pet_stage_of(0) == PET_EGG);
}

static void test_stage_thresholds(void) {
    assert(pet_stage_of(2 * 499) == PET_EGG);
    assert(pet_stage_of(2 * 500) == PET_BABY);
    assert(pet_stage_of(2 * 10000) == PET_CHILD);
    assert(pet_stage_of(2 * 100000) == PET_TEEN);
    assert(pet_stage_of(2 * 1000000) == PET_ADULT);
    assert(pet_stage_of(UINT32_MAX) == PET_ADULT);
}

static void test_next_threshold(void) {
    assert(pet_next_threshold(0) == 500);
    assert(pet_next_threshold(2 * 500) == 10000);
    assert(pet_next_threshold(2 * 1000000) == 0);
}

static void test_fed_press_counts_full(void) {
    fresh();
    pet_add_presses(&p, 3);
    assert(p.keys == 3);
    assert(p.growth_half == 6);
}

static void test_hungry_press_counts_half(void) {
    fresh();
    p.fullness_ms = 0;
    pet_add_presses(&p, 3);
    assert(p.keys == 3);
    assert(p.growth_half == 3);
}

static void test_add_presses_reports_stage_up(void) {
    fresh();
    p.growth_half = 2 * 499;
    assert(pet_add_presses(&p, 1));
    assert(!pet_add_presses(&p, 1));
}

static void test_growth_saturates(void) {
    fresh();
    p.growth_half = UINT32_MAX - 1;
    pet_add_presses(&p, 10);
    assert(p.growth_half == UINT32_MAX);
    assert(p.keys == 10);
}

static void test_drain_24h(void) {
    fresh();
    pet_elapse(&p, PET_FULL_MS / 2);
    assert(p.fullness_ms == PET_FULL_MS / 2);
    assert(!pet_hungry(&p));
    pet_elapse(&p, PET_FULL_MS);
    assert(p.fullness_ms == 0);
    assert(pet_hungry(&p));
}

static void test_feed_refills(void) {
    fresh();
    pet_elapse(&p, PET_FULL_MS);
    pet_feed(&p);
    assert(p.fullness_ms == PET_FULL_MS);
    assert(!pet_hungry(&p));
}

static void test_elapse_wrap_delta(void) {
    fresh();
    uint32_t then = UINT32_MAX - 999, now = 1000;
    pet_elapse(&p, now - then);
    assert(p.fullness_ms == PET_FULL_MS - 2000);
}

static void test_mood_bands(void) {
    assert(pet_mood_of(80, 0, false) == PET_EXCITED);
    assert(pet_mood_of(30, 0, false) == PET_HAPPY);
    assert(pet_mood_of(5, 0, false) == PET_CALM);
    assert(pet_mood_of(0, 1000, false) == PET_SLEEPY);
    assert(pet_mood_of(0, PET_SLEEP_IDLE_MS, false) == PET_ASLEEP);
}

static void test_boost_raises_one(void) {
    assert(pet_mood_of(5, 0, true) == PET_HAPPY);
    assert(pet_mood_of(80, 0, true) == PET_EXCITED);
    assert(pet_mood_of(0, PET_SLEEP_IDLE_MS, true) == PET_ASLEEP);
}

static void test_save_due(void) {
    assert(!pet_save_due(PET_SAVE_EVERY_MS, false));
    assert(!pet_save_due(PET_SAVE_EVERY_MS - 1, true));
    assert(pet_save_due(PET_SAVE_EVERY_MS, true));
}

static void test_pack_roundtrip(void) {
    struct pet_state a = {123456, 2 * 10001, 4321, PET_CHILD}, b;
    uint8_t buf[PET_BLOB_LEN];
    pet_pack(&a, buf);
    assert(buf[0] == PET_BLOB_VERSION);
    assert(pet_unpack(buf, sizeof(buf), &b));
    assert(b.keys == a.keys && b.growth_half == a.growth_half);
    assert(b.fullness_ms == a.fullness_ms && b.shown_stage == a.shown_stage);
}

static void test_unpack_rejects(void) {
    struct pet_state a = {1, 2, 3, PET_BABY}, out;
    uint8_t buf[PET_BLOB_LEN], zeros[PET_BLOB_LEN] = {0};
    pet_pack(&a, buf);
    memset(&out, 0x5A, sizeof(out));
    struct pet_state sentinel = out;
    assert(!pet_unpack(buf, PET_BLOB_LEN - 1, &out));
    buf[0] = 2;
    assert(!pet_unpack(buf, sizeof(buf), &out));
    assert(!pet_unpack(zeros, sizeof(zeros), &out));
    assert(memcmp(&out, &sentinel, sizeof(out)) == 0);
}

static void test_unpack_clamps(void) {
    struct pet_state a = {1, 2, 0xFFFFFFFFu, 9}, out;
    uint8_t buf[PET_BLOB_LEN];
    pet_pack(&a, buf);
    assert(pet_unpack(buf, sizeof(buf), &out));
    assert(out.fullness_ms == PET_FULL_MS);
    assert(out.shown_stage == PET_ADULT);
}

static struct pet_game g;

static void test_side_of_position(void) {
    const uint8_t left[] = {0, 5, 12, 29, 36, 37, 38};
    const uint8_t right[] = {6, 11, 18, 35, 39, 40, 41};
    for (size_t i = 0; i < sizeof(left); i++)
        assert(pet_side_of_position(left[i]) == PET_LEFT);
    for (size_t i = 0; i < sizeof(right); i++)
        assert(pet_side_of_position(right[i]) == PET_RIGHT);
}

static enum pet_side other(enum pet_side s) { return s == PET_LEFT ? PET_RIGHT : PET_LEFT; }

static void test_correct_guess_scores(void) {
    pet_game_start(&g, 7, 1000);
    assert(g.phase == PET_GAME_ROUND && pet_game_capturing(&g));
    pet_game_guess(&g, g.hide, 1100);
    assert(g.phase == PET_GAME_REVEAL && g.last_hit && g.score == 1);
}

static void test_wrong_guess_no_score(void) {
    pet_game_start(&g, 7, 1000);
    pet_game_guess(&g, other(g.hide), 1100);
    assert(g.phase == PET_GAME_REVEAL && !g.last_hit && g.score == 0);
}

static void test_second_guess_ignored(void) {
    pet_game_start(&g, 7, 1000);
    pet_game_guess(&g, g.hide, 1100);
    uint32_t end = g.phase_end_ms;
    pet_game_guess(&g, g.hide, 1200);
    assert(g.score == 1 && g.phase_end_ms == end && g.phase == PET_GAME_REVEAL);
}

static void test_round_timeout_is_miss(void) {
    pet_game_start(&g, 7, 1000);
    pet_game_step(&g, 1000 + PET_ROUND_MS);
    assert(g.phase == PET_GAME_REVEAL && !g.last_hit && g.score == 0);
}

static void test_full_game_ends(void) {
    uint32_t t = 5000;
    pet_game_start(&g, 99, t);
    for (int r = 0; r < PET_GAME_ROUNDS; r++) {
        assert(g.phase == PET_GAME_ROUND);
        t += 100;
        pet_game_guess(&g, g.hide, t);
        t += PET_REVEAL_MS;
        pet_game_step(&g, t);
    }
    assert(g.phase == PET_GAME_RESULT);
    assert(g.score == PET_GAME_ROUNDS);
    assert(pet_game_won(&g) && !pet_game_capturing(&g));
    pet_game_step(&g, t + PET_RESULT_MS);
    assert(g.phase == PET_GAME_OFF);
}

static void test_hard_cap(void) {
    pet_game_start(&g, 3, 2000);
    pet_game_step(&g, 2000 + PET_GAME_CAP_MS);
    assert(g.phase == PET_GAME_OFF && !pet_game_capturing(&g));
}

static void test_guess_when_off_ignored(void) {
    memset(&g, 0, sizeof(g));
    g.phase = PET_GAME_OFF;
    pet_game_guess(&g, PET_LEFT, 10);
    assert(g.phase == PET_GAME_OFF && g.score == 0);
}

static void test_hide_varies(void) {
    int lefts = 0;
    for (uint32_t seed = 0; seed < 64; seed++) {
        pet_game_start(&g, seed, 0);
        lefts += g.hide == PET_LEFT;
    }
    assert(lefts > 8 && lefts < 56);
}

static void test_progress_pct(void) {
    assert(pet_progress_pct(0) == 0);
    assert(pet_progress_pct(2 * 250) == 50);
    assert(pet_progress_pct(2 * 500) == 0);       /* Baby just reached */
    assert(pet_progress_pct(2 * 5250) == 50);     /* halfway 500 -> 10000 */
    assert(pet_progress_pct(2 * 1000000) == 100); /* Adult */
    assert(pet_progress_pct(UINT32_MAX) == 100);
}

int main(void) {
    int n = 0;
#define RUN(t) (t(), n++)
    RUN(test_init_is_full_egg);
    RUN(test_stage_thresholds);
    RUN(test_next_threshold);
    RUN(test_fed_press_counts_full);
    RUN(test_hungry_press_counts_half);
    RUN(test_add_presses_reports_stage_up);
    RUN(test_growth_saturates);
    RUN(test_drain_24h);
    RUN(test_feed_refills);
    RUN(test_elapse_wrap_delta);
    RUN(test_mood_bands);
    RUN(test_boost_raises_one);
    RUN(test_save_due);
    RUN(test_pack_roundtrip);
    RUN(test_unpack_rejects);
    RUN(test_unpack_clamps);
    RUN(test_side_of_position);
    RUN(test_correct_guess_scores);
    RUN(test_wrong_guess_no_score);
    RUN(test_second_guess_ignored);
    RUN(test_round_timeout_is_miss);
    RUN(test_full_game_ends);
    RUN(test_hard_cap);
    RUN(test_guess_when_off_ignored);
    RUN(test_hide_varies);
    RUN(test_progress_pct);
    printf("ok %d tests\n", n);
    return 0;
}
