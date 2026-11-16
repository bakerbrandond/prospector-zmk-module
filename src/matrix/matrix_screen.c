/*
 * Matrix rain renderer: one full-screen object whose draw handler paints the
 * grid cell by cell, so no frame buffer is needed. Only changed rows of each
 * column are invalidated, as one span per column: LVGL keeps at most 32
 * dirty areas before falling back to a full-screen redraw.
 */
#include "matrix_screen.h"

#include <fonts.h>
#include <theme.h>

#define CELL_W MATRIX_CELL_W
#define CELL_H MATRIX_CELL_H

static lv_obj_t *screen, *grid;
static const struct matrix_state *state;
static lv_color_t palette[MATRIX_LEVEL_MAX + 1];
static char glyph_utf8[MATRIX_GLYPHS][4];

static void cell_area(int r, int c, lv_area_t *a) {
    a->x1 = grid->coords.x1 + c * CELL_W;
    a->y1 = grid->coords.y1 + r * CELL_H;
    a->x2 = a->x1 + CELL_W - 1;
    a->y2 = a->y1 + CELL_H - 1;
}

static void draw_cb(lv_event_t *e) {
    if (!state)
        return;
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
    const lv_area_t *clip = draw_ctx->clip_area;

    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.font = &THEME_FONT_MATRIX;
    dsc.align = LV_TEXT_ALIGN_CENTER;

    int c0 = LV_MAX(0, (clip->x1 - grid->coords.x1) / CELL_W);
    int c1 = LV_MIN(MATRIX_COLS - 1, (clip->x2 - grid->coords.x1) / CELL_W);
    int r0 = LV_MAX(0, (clip->y1 - grid->coords.y1) / CELL_H);
    int r1 = LV_MIN(MATRIX_ROWS - 1, (clip->y2 - grid->coords.y1) / CELL_H);

    for (int c = c0; c <= c1; c++) {
        for (int r = r0; r <= r1; r++) {
            uint8_t level = state->level[r][c];
            if (!level)
                continue;
            lv_area_t a;
            cell_area(r, c, &a);
            dsc.color = matrix_is_head(state, r, c) ? lv_color_hex(THEME_MATRIX_HEAD) : palette[level];
            lv_draw_label(draw_ctx, &dsc, &a, glyph_utf8[state->glyph[r][c]], NULL);
        }
    }
}

lv_obj_t *matrix_screen_create(void) {
    if (screen)
        return screen;

    for (int l = 0; l <= MATRIX_LEVEL_MAX; l++)
        palette[l] = lv_color_mix(lv_color_hex(THEME_MATRIX_TRAIL), lv_color_hex(THEME_BG),
                                  (uint8_t)(l * 255 / MATRIX_LEVEL_MAX));
    for (int i = 0; i < MATRIX_GLYPHS; i++) {
        uint32_t cp = 0xFF66 + i; /* half-width katakana */
        glyph_utf8[i][0] = (char)(0xE0 | (cp >> 12));
        glyph_utf8[i][1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        glyph_utf8[i][2] = (char)(0x80 | (cp & 0x3F));
        glyph_utf8[i][3] = 0;
    }

    screen = lv_obj_create(NULL);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    grid = lv_obj_create(screen);
    lv_obj_remove_style_all(grid);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(grid, MATRIX_COLS * CELL_W, MATRIX_ROWS * CELL_H);
    /* centre the grid: the width isn't a whole number of cells */
    lv_obj_set_pos(grid, (DISPLAY_W - MATRIX_COLS * CELL_W) / 2, 0);
    lv_obj_add_event_cb(grid, draw_cb, LV_EVENT_DRAW_MAIN, NULL);

    return screen;
}

void matrix_screen_render(const struct matrix_state *m, const uint16_t *dirty) {
    state = m;
    for (int c = 0; c < MATRIX_COLS; c++) {
        if (!dirty[c])
            continue;
        int lo = __builtin_ctz(dirty[c]);
        int hi = 31 - __builtin_clz((uint32_t)dirty[c]);
        lv_area_t a, b;
        cell_area(lo, c, &a);
        cell_area(hi, c, &b);
        a.y2 = b.y2;
        lv_obj_invalidate_area(grid, &a);
    }
}
