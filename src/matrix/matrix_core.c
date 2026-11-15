/*
 * Matrix rain model. Each column drops a bright head down the grid at its
 * own speed; cells it leaves behind fade by the column's decay each step.
 * After falling off the bottom a column waits a random while, then drops
 * again with new speed and trail length. Dirty bits are computed by diffing
 * each column before/after the step, so they are exact by construction.
 */
#include "matrix_core.h"

#include <string.h>

#define RESPAWN_WAIT_MAX 30
#define INITIAL_WAIT_MAX 40
#define TRAIL_MIN 6
#define TRAIL_MAX 12
#define FLICKER_ONE_IN 8

static uint32_t next_rand(struct matrix_state *m) {
    uint32_t x = m->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m->rng = x;
    return x;
}

static uint32_t rand_below(struct matrix_state *m, uint32_t n) { return next_rand(m) % n; }

bool matrix_is_head(const struct matrix_state *m, int r, int c) {
    const struct matrix_col *col = &m->col[c];
    return col->head == r && col->head < MATRIX_ROWS;
}

static void light(struct matrix_state *m, int r, int c) {
    m->glyph[r][c] = (uint8_t)rand_below(m, MATRIX_GLYPHS);
    m->level[r][c] = MATRIX_LEVEL_MAX;
}

static void spawn(struct matrix_state *m, int c) {
    struct matrix_col *col = &m->col[c];
    uint32_t trail = TRAIL_MIN + rand_below(m, TRAIL_MAX - TRAIL_MIN + 1);
    col->head = 0;
    col->phase = 0;
    col->period = (uint8_t)(1 + rand_below(m, 3));
    col->decay = (uint8_t)((MATRIX_LEVEL_MAX + trail - 1) / trail);
    light(m, 0, c);
}

void matrix_init(struct matrix_state *m, uint32_t seed) {
    memset(m, 0, sizeof(*m));
    m->rng = seed ? seed : 0x6d617472u;
    for (int c = 0; c < MATRIX_COLS; c++) {
        struct matrix_col *col = &m->col[c];
        col->head = MATRIX_ROWS; /* off-grid: waiting */
        col->period = 1;
        col->decay = 2;
        col->wait = (uint8_t)rand_below(m, INITIAL_WAIT_MAX + 1);
        for (int r = 0; r < MATRIX_ROWS; r++)
            m->glyph[r][c] = (uint8_t)rand_below(m, MATRIX_GLYPHS);
    }
}

static void step_column(struct matrix_state *m, int c) {
    struct matrix_col *col = &m->col[c];

    /* fade the trail (the head cell stays at full brightness) */
    for (int r = 0; r < MATRIX_ROWS; r++) {
        if (m->level[r][c] && !matrix_is_head(m, r, c))
            m->level[r][c] = m->level[r][c] > col->decay ? m->level[r][c] - col->decay : 0;
    }

    if (col->head < MATRIX_ROWS) {
        if (++col->phase >= col->period) {
            col->phase = 0;
            col->head++;
            if (col->head < MATRIX_ROWS)
                light(m, col->head, c);
            else
                col->wait = (uint8_t)rand_below(m, RESPAWN_WAIT_MAX + 1);
        }
    } else if (col->wait > 0) {
        col->wait--;
    } else {
        spawn(m, c);
    }

    if (rand_below(m, FLICKER_ONE_IN) == 0) {
        int r = (int)rand_below(m, MATRIX_ROWS);
        if (m->level[r][c])
            m->glyph[r][c] = (uint8_t)rand_below(m, MATRIX_GLYPHS);
    }
}

void matrix_step(struct matrix_state *m) {
    for (int c = 0; c < MATRIX_COLS; c++) {
        uint8_t glyph[MATRIX_ROWS], level[MATRIX_ROWS];
        int16_t head = m->col[c].head;
        for (int r = 0; r < MATRIX_ROWS; r++) {
            glyph[r] = m->glyph[r][c];
            level[r] = m->level[r][c];
        }

        step_column(m, c);

        uint16_t dirty = 0;
        for (int r = 0; r < MATRIX_ROWS; r++) {
            bool was_head = head == r;
            if (glyph[r] != m->glyph[r][c] || level[r] != m->level[r][c] ||
                was_head != matrix_is_head(m, r, c))
                dirty |= (uint16_t)(1u << r);
        }
        m->dirty[c] = dirty;
    }
}
