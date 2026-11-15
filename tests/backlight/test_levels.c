/* Host tests for backlight level stepping. Run: make -C tests/backlight test */
#include <assert.h>
#include <stdio.h>

#include "bl_levels.h"

static void test_cycles_in_order(void) {
    assert(bl_next_level(0) == 5);
    assert(bl_next_level(5) == 15);
    assert(bl_next_level(15) == 30);
    assert(bl_next_level(30) == 60);
    assert(bl_next_level(60) == 100);
    assert(bl_next_level(100) == 0);
}

static void test_between_levels_goes_up(void) {
    assert(bl_next_level(50) == 60);
    assert(bl_next_level(1) == 5);
}

static void test_out_of_range_wraps(void) {
    assert(bl_next_level(101) == 0);
    assert(bl_next_level(255) == 0);
}

int main(void) {
    int n = 0;
#define RUN(t) (t(), n++)
    RUN(test_cycles_in_order);
    RUN(test_between_levels_goes_up);
    RUN(test_out_of_range_wraps);
    printf("ok %d tests\n", n);
    return 0;
}
