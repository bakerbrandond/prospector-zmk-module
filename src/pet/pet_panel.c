/*
 * Typing pet panel renderer. Every object is created once in
 * pet_panel_create(); pet_panel_render() only moves, re-texts and shows/hides
 * them, so LVGL heap use is fixed.
 *
 * Layout (204x64): the pet on the left, a 24x24 canvas drawn at 2x (48 px);
 * on the right, the stage name with mood hearts and fullness pips, then WPM and
 * lifetime keys, then a bar towards the next stage. During the mini-game the
 * stats hide, prompts sit in the middle 108 px, and the pet pops out at the
 * side it was hiding on.
 */
#include "pet_panel.h"

#include <stdio.h>
#include <string.h>

#include <fonts.h>
#include <theme.h>

#define PET_ZOOM 512 /* 2x; LV_IMG_ZOOM_NONE is 256 */
#define PET_DRAW (PET_SPRITE_W * 2)
#define STATS_X 60
#define STATS_W (PET_PANEL_W - STATS_X)
#define HEART 8
#define PIP_W 5
#define PIP_H 10

static const char *const stage_names[PET_STAGES] = {"Egg", "Baby", "Child", "Teen", "Adult"};

static lv_obj_t *panel, *canvas;
static lv_obj_t *stage_label, *hearts[4], *pips[4], *wpm_label, *keys_label, *bar, *next_label;
static lv_obj_t *prompt, *score_label;
static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(PET_SPRITE_W, PET_SPRITE_H)];
static const char *const *drawn; /* sprite currently on the canvas */

static lv_color_t ink(char c) {
    switch (c) {
    case '#': return lv_color_hex(NORD3);
    case 'o': return lv_color_hex(NORD8);
    case '*': return lv_color_hex(NORD13);
    case 'w': return lv_color_hex(NORD6);
    default: return lv_color_hex(THEME_BG);
    }
}

static void draw_sprite(const char *const *rows) {
    if (rows == drawn)
        return;
    drawn = rows;
    for (int y = 0; y < PET_SPRITE_H; y++)
        for (int x = 0; x < PET_SPRITE_W; x++)
            lv_canvas_set_px_color(canvas, x, y, ink(rows[y][x]));
}

/* place the zoomed pet so its 48x48 drawing has its top-left at (x, y) */
static void place_pet(int x, int y) {
    /* zoom pivots on the object's centre */
    lv_obj_set_pos(canvas, x + (PET_DRAW - PET_SPRITE_W) / 2, y + (PET_DRAW - PET_SPRITE_H) / 2);
}

static lv_obj_t *box(lv_obj_t *parent, int w, int h, int radius) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, radius, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN);
    return o;
}

static lv_obj_t *label(lv_obj_t *parent, const lv_font_t *font, uint32_t color) {
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
    lv_label_set_text(l, "");
    return l;
}

lv_obj_t *pet_panel_create(lv_obj_t *parent) {
    panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, PET_PANEL_W, PET_PANEL_H);
    lv_obj_set_style_bg_color(panel, lv_color_hex(THEME_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);

    canvas = lv_canvas_create(panel);
    lv_canvas_set_buffer(canvas, cbuf, PET_SPRITE_W, PET_SPRITE_H, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_hex(THEME_BG), LV_OPA_COVER);
    lv_img_set_antialias(canvas, false);
    lv_img_set_zoom(canvas, PET_ZOOM);
    place_pet(0, (PET_PANEL_H - PET_DRAW) / 2);
    drawn = NULL;

    stage_label = label(panel, &CaskaydiaCove_SemiBold_20, NORD6);
    lv_obj_set_pos(stage_label, STATS_X, 0);
    for (int i = 0; i < 4; i++) {
        hearts[i] = box(panel, HEART, HEART, 2);
        lv_obj_set_pos(hearts[i], STATS_X + 66 + i * (HEART + 2), 7);
        pips[i] = box(panel, PIP_W, PIP_H, 1);
        lv_obj_set_pos(pips[i], STATS_X + 112 + i * (PIP_W + 2), 6);
    }
    wpm_label = label(panel, &CaskaydiaCove_Regular_14, NORD4);
    lv_obj_set_pos(wpm_label, STATS_X, 26);
    keys_label = label(panel, &CaskaydiaCove_Regular_14, NORD4);
    lv_obj_align(keys_label, LV_ALIGN_TOP_RIGHT, 0, 26);

    bar = lv_bar_create(panel);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, STATS_W - 44, 6);
    lv_obj_set_pos(bar, STATS_X, 50);
    lv_obj_set_style_bg_color(bar, lv_color_hex(NORD1), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(NORD15), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, 100);
    next_label = label(panel, &CaskaydiaCove_Regular_14, NORD3);
    lv_obj_align(next_label, LV_ALIGN_TOP_RIGHT, 0, 44);

    prompt = label(panel, &CaskaydiaCove_Regular_14, NORD6);
    lv_obj_align(prompt, LV_ALIGN_TOP_MID, 0, 6);
    score_label = label(panel, &CaskaydiaCove_Regular_14, NORD4);
    lv_obj_align(score_label, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_add_flag(prompt, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(score_label, LV_OBJ_FLAG_HIDDEN);
    return panel;
}

static void fmt_count(char *buf, size_t len, uint32_t n) {
    if (n < 10000)
        snprintf(buf, len, "%u", (unsigned)n);
    else if (n < 1000000)
        snprintf(buf, len, "%u.%uk", (unsigned)(n / 1000), (unsigned)(n / 100 % 10));
    else
        snprintf(buf, len, "%u.%uM", (unsigned)(n / 1000000), (unsigned)(n / 100000 % 10));
}

static void set_text(lv_obj_t *l, const char *text) {
    if (strcmp(lv_label_get_text(l), text) != 0)
        lv_label_set_text(l, text);
}

static void show(lv_obj_t *o, bool on) {
    if (on)
        lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static void show_stats(bool on) {
    show(stage_label, on);
    show(wpm_label, on);
    show(keys_label, on);
    show(bar, on);
    show(next_label, on);
    for (int i = 0; i < 4; i++) {
        show(hearts[i], on);
        show(pips[i], on);
    }
    show(prompt, !on);
    show(score_label, !on);
}

void pet_panel_render(const struct pet_view *v) {
    if (!panel)
        return;
    char buf[32];
    draw_sprite(pet_sprite(v->stage, v->pose));

    if (v->game == PET_GAME_OFF) {
        show_stats(true);
        show(canvas, true);
        place_pet(0, (PET_PANEL_H - PET_DRAW) / 2);
        set_text(stage_label, stage_names[v->stage]);
        for (int i = 0; i < 4; i++) {
            lv_obj_set_style_bg_color(hearts[i], lv_color_hex(i < (int)v->mood ? NORD11 : NORD2),
                                      LV_PART_MAIN);
            lv_obj_set_style_bg_color(pips[i], lv_color_hex(i < v->fullness_pips ? NORD13 : NORD2),
                                      LV_PART_MAIN);
        }
        snprintf(buf, sizeof(buf), "%d wpm", v->wpm);
        set_text(wpm_label, buf);
        char n[12];
        fmt_count(n, sizeof(n), v->keys);
        snprintf(buf, sizeof(buf), "%s keys", n);
        set_text(keys_label, buf);
        lv_obj_align(keys_label, LV_ALIGN_TOP_RIGHT, 0, 26);
        lv_bar_set_value(bar, v->progress, LV_ANIM_OFF);
        if (v->next) {
            fmt_count(n, sizeof(n), v->next);
            set_text(next_label, n);
        } else {
            set_text(next_label, "max");
        }
        lv_obj_align(next_label, LV_ALIGN_TOP_RIGHT, 0, 44);
        return;
    }

    /* mini-game */
    show_stats(false);
    switch (v->game) {
    case PET_GAME_ROUND:
        show(canvas, false); /* it's hiding */
        set_text(prompt, "left/right?");
        break;
    case PET_GAME_REVEAL:
        show(canvas, true);
        place_pet(v->side == PET_LEFT ? 0 : PET_PANEL_W - PET_DRAW, (PET_PANEL_H - PET_DRAW) / 2);
        set_text(prompt, v->hit ? "found!" : "missed");
        break;
    default: /* RESULT */
        show(canvas, true);
        place_pet(0, (PET_PANEL_H - PET_DRAW) / 2);
        set_text(prompt, v->score >= PET_GAME_WIN ? "well done" : "nice try");
        break;
    }
    lv_obj_align(prompt, LV_ALIGN_TOP_MID, 0, 6);
    snprintf(buf, sizeof(buf), "%u / %u", v->score, PET_GAME_ROUNDS);
    set_text(score_label, buf);
    lv_obj_align(score_label, LV_ALIGN_BOTTOM_MID, 0, -4);
}
