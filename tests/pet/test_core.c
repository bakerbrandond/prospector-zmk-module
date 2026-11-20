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
    printf("ok %d tests\n", n);
    return 0;
}
