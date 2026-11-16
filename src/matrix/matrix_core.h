/*
 * Matrix rain model: a grid of glyphs with brightness levels, pure C so it
 * can be unit-tested on the host (see tests/matrix).
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../display_size.h"

/* 12x20 px cells: 23 columns x 12 rows on the 280x240 screen */
#define MATRIX_CELL_W 12
#define MATRIX_CELL_H 20
#define MATRIX_COLS (DISPLAY_W / MATRIX_CELL_W)
#define MATRIX_ROWS (DISPLAY_H / MATRIX_CELL_H)
#define MATRIX_GLYPHS 56 /* half-width katakana U+FF66..U+FF9D */
#define MATRIX_LEVEL_MAX 15

struct matrix_col {
    int16_t head;   /* row of the head; >= MATRIX_ROWS once it has fallen off */
    uint8_t period; /* advance one row every `period` steps (1-3) */
    uint8_t phase;  /* steps since the last advance */
    uint8_t decay;  /* levels lost per step by trail cells */
    uint8_t wait;   /* steps left before (re)starting at row 0 */
};

struct matrix_state {
    uint8_t glyph[MATRIX_ROWS][MATRIX_COLS];
    uint8_t level[MATRIX_ROWS][MATRIX_COLS]; /* 0 = empty */
    struct matrix_col col[MATRIX_COLS];
    uint16_t dirty[MATRIX_COLS]; /* bit r = row r changed in the last step */
    uint32_t rng;
};

void matrix_init(struct matrix_state *m, uint32_t seed);
void matrix_step(struct matrix_state *m);
bool matrix_is_head(const struct matrix_state *m, int r, int c);
