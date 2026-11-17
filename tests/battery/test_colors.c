/* Host tests for the battery bar's colour by charge level. Run: make -C tests/battery test */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "theme.h"

static void test_high_is_green(void) {
    assert(theme_battery_color(100) == NORD14);
    assert(theme_battery_color(70) == NORD14);
}

static void test_middle_is_yellow(void) {
    assert(theme_battery_color(69) == NORD13);
    assert(theme_battery_color(30) == NORD13);
}

static void test_low_is_red(void) {
    assert(theme_battery_color(29) == NORD11);
    assert(theme_battery_color(0) == NORD11);
}

int main(void) {
    test_high_is_green();
    test_middle_is_yellow();
    test_low_is_red();
    printf("battery colours: 3 tests passed\n");
    return 0;
}
