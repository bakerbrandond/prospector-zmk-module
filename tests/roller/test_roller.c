/* Host tests for the layer roller option builder. Run: make -C tests/roller test */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "roller_names.h"

static char buf[256];

static void test_all_visible(void) {
    const char *names[] = {"Default", "Lower"};
    assert(roller_build(names, 2, 1, NULL, false, buf, sizeof(buf)) == 1);
    assert(strcmp(buf, "Default\nLower") == 0);
}

static void test_hidden_skipped(void) {
    const char *names[] = {"A", "Zappy", "B"};
    assert(roller_build(names, 3, 2, "Zappy", false, buf, sizeof(buf)) == 1);
    assert(strcmp(buf, "A\nB") == 0);
}

static void test_active_hidden(void) {
    const char *names[] = {"A", "Zappy", "B"};
    assert(roller_build(names, 3, 1, "Zappy", false, buf, sizeof(buf)) == -1);
    assert(roller_build(names, 3, 7, "Zappy", false, buf, sizeof(buf)) == -1);
}

static void test_unnamed_uses_index(void) {
    const char *names[] = {NULL, "X", ""};
    assert(roller_build(names, 3, 0, "", false, buf, sizeof(buf)) == 0);
    assert(strcmp(buf, "0\nX\n2") == 0);
}

static void test_caps(void) {
    const char *names[] = {"Default"};
    roller_build(names, 1, 0, NULL, true, buf, sizeof(buf));
    assert(strcmp(buf, "DEFAULT") == 0);
}

static void test_truncates_safely(void) {
    const char *names[] = {"Default", "Lower", "Raise"};
    char small[12];
    memset(small, 'x', sizeof(small));
    roller_build(names, 3, 0, NULL, false, small, 10);
    assert(strlen(small) <= 9);
    assert(small[10] == 'x' && small[11] == 'x'); /* nothing written past len */
}

int main(void) {
    int n = 0;
#define RUN(t) (t(), n++)
    RUN(test_all_visible);
    RUN(test_hidden_skipped);
    RUN(test_active_hidden);
    RUN(test_unnamed_uses_index);
    RUN(test_caps);
    RUN(test_truncates_safely);
    printf("ok %d tests\n", n);
    return 0;
}
