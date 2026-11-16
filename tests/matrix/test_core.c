/* Host tests for the matrix rain model. Run: make -C tests/matrix test */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "matrix_core.h"

static struct matrix_state m;

static void test_init_empty(void) {
    matrix_init(&m, 1);
    for (int r = 0; r < MATRIX_ROWS; r++)
        for (int c = 0; c < MATRIX_COLS; c++)
            assert(m.level[r][c] == 0);
}

static void test_head_advances_by_period(void) {
    matrix_init(&m, 1);
    m.col[0] = (struct matrix_col){.head = 0, .period = 2, .phase = 0, .decay = 2, .wait = 0};
    matrix_step(&m);
    matrix_step(&m);
    assert(m.col[0].head == 1);
    matrix_step(&m);
    matrix_step(&m);
    assert(m.col[0].head == 2);
}

static void test_trail_dims(void) {
    matrix_init(&m, 1);
    /* column 0 parked far away so only our cell matters */
    m.col[0] = (struct matrix_col){.head = MATRIX_ROWS + 1, .period = 1, .decay = 3, .wait = 200};
    m.level[5][0] = MATRIX_LEVEL_MAX;
    matrix_step(&m);
    assert(m.level[5][0] == 12);
    for (int i = 0; i < 4; i++)
        matrix_step(&m);
    assert(m.level[5][0] == 0);
}

static void test_dirty_exact(void) {
    matrix_init(&m, 12345);
    for (int s = 0; s < 2000; s++) {
        struct matrix_state before = m;
        matrix_step(&m);
        for (int r = 0; r < MATRIX_ROWS; r++)
            for (int c = 0; c < MATRIX_COLS; c++) {
                bool changed = before.glyph[r][c] != m.glyph[r][c] ||
                               before.level[r][c] != m.level[r][c] ||
                               matrix_is_head(&before, r, c) != matrix_is_head(&m, r, c);
                bool flagged = (m.dirty[c] >> r) & 1;
                assert(changed == flagged);
            }
    }
}

static void test_columns_respawn(void) {
    matrix_init(&m, 7);
    int starts[MATRIX_COLS] = {0};
    int16_t prev[MATRIX_COLS];
    for (int c = 0; c < MATRIX_COLS; c++)
        prev[c] = m.col[c].head;
    for (int s = 0; s < 3000; s++) {
        matrix_step(&m);
        for (int c = 0; c < MATRIX_COLS; c++) {
            if (m.col[c].head == 0 && prev[c] != 0)
                starts[c]++;
            prev[c] = m.col[c].head;
        }
    }
    for (int c = 0; c < MATRIX_COLS; c++)
        assert(starts[c] >= 2);
}

static void test_long_run_bounds(void) {
    matrix_init(&m, 99);
    for (long s = 0; s < 200000; s++) {
        matrix_step(&m);
        for (int c = 0; c < MATRIX_COLS; c++) {
            assert(m.col[c].head >= 0 && m.col[c].head <= MATRIX_ROWS + 1);
            assert(m.col[c].period >= 1 && m.col[c].period <= 3);
            for (int r = 0; r < MATRIX_ROWS; r++) {
                assert(m.glyph[r][c] < MATRIX_GLYPHS);
                assert(m.level[r][c] <= MATRIX_LEVEL_MAX);
            }
        }
    }
}

static void test_deterministic(void) {
    struct matrix_state a, b;
    matrix_init(&a, 42);
    matrix_init(&b, 42);
    for (int s = 0; s < 500; s++) {
        matrix_step(&a);
        matrix_step(&b);
    }
    assert(memcmp(&a, &b, sizeof(a)) == 0);
}

/* The rain covers the landscape screen LVGL sees (280x240 after the 270-degree
 * rotation): every row visible, and less than one cell of width left over. */
static void test_grid_fits_screen(void) {
    assert(MATRIX_ROWS * MATRIX_CELL_H == DISPLAY_H);
    assert(MATRIX_COLS * MATRIX_CELL_W <= DISPLAY_W);
    assert(DISPLAY_W - MATRIX_COLS * MATRIX_CELL_W < MATRIX_CELL_W);
    assert(MATRIX_ROWS <= 16); /* dirty is a uint16_t row mask */
}

int main(void) {
    int n = 0;
#define RUN(t) (t(), n++)
    RUN(test_init_empty);
    RUN(test_head_advances_by_period);
    RUN(test_trail_dims);
    RUN(test_dirty_exact);
    RUN(test_columns_respawn);
    RUN(test_long_run_bounds);
    RUN(test_deterministic);
    RUN(test_grid_fits_screen);
    printf("ok %d tests\n", n);
    return 0;
}
