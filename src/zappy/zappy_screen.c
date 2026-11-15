/*
 * Zappy Birds renderer. Every object is created once in
 * zappy_screen_create(); zappy_screen_render() only moves, shows/hides and
 * re-texts them, so the LVGL heap use is fixed while playing.
 *
 * Original Nord look: a round yellow bird with an orange lightning-bolt
 * wing, and frost-blue charged pylons with bright caps on a Polar Night sky.
 */
#include "zappy_screen.h"

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <fonts.h>
#include <theme.h>

#define FLASH_MS 150

#define BIRD_DRAW 20 /* body drawn slightly larger than the collision box */
#define CAP_H 6
#define CAP_OVERHANG 3
#define TITLE_BIRD_DROP 80

static lv_obj_t *screen;
static lv_obj_t *bird, *bolt;
static lv_obj_t *pylon_top[ZAPPY_NUM_PYLONS], *pylon_bot[ZAPPY_NUM_PYLONS];
static lv_obj_t *cap_top[ZAPPY_NUM_PYLONS], *cap_bot[ZAPPY_NUM_PYLONS];
static lv_obj_t *score_label, *hard_score_label, *target_label;
static lv_obj_t *title_box, *title_best;
static lv_obj_t *over_box, *over_mode, *over_score, *over_best, *over_new;

static lv_style_t st_plain, st_bar, st_cap;

static char score_buf[8], hard_score_buf[8], target_buf[2], title_best_buf[24], over_score_buf[8],
    over_best_buf[16];

/* bolt wing in two poses, coordinates inside the bird object */
static const lv_point_t bolt_up[] = {{2, 12}, {8, 4}, {9, 9}, {15, 2}};
static const lv_point_t bolt_down[] = {{2, 8}, {8, 15}, {9, 10}, {15, 17}};

static lv_obj_t *plain_obj(lv_obj_t *parent) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_add_style(o, &st_plain, LV_PART_MAIN);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static lv_obj_t *label(lv_obj_t *parent, const lv_font_t *font, uint32_t color) {
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    return l;
}

static lv_obj_t *box(void) {
    lv_obj_t *b = plain_obj(screen);
    lv_obj_set_size(b, ZAPPY_W, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(b, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(b, 6, LV_PART_MAIN);
    return b;
}

/* Only touch flags/styles on change: every LVGL setter invalidates its object
 * even when the value is unchanged, and the screen is the whole display. */
static void set_hidden(lv_obj_t *o, bool hidden) {
    if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN) == hidden)
        return;
    if (hidden)
        lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static int last_flash = -1, last_flap_pose = -1;

lv_obj_t *zappy_screen_create(void) {
    if (screen)
        return screen;

    lv_style_init(&st_plain);
    lv_style_set_bg_opa(&st_plain, LV_OPA_TRANSP);
    lv_style_set_border_width(&st_plain, 0);
    lv_style_set_pad_all(&st_plain, 0);

    lv_style_init(&st_bar);
    lv_style_set_bg_color(&st_bar, lv_color_hex(NORD10));
    lv_style_set_bg_opa(&st_bar, LV_OPA_COVER);
    lv_style_set_border_color(&st_bar, lv_color_hex(NORD9));
    lv_style_set_border_width(&st_bar, 2);
    lv_style_set_border_side(&st_bar, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT);

    lv_style_init(&st_cap);
    lv_style_set_bg_color(&st_cap, lv_color_hex(NORD8));
    lv_style_set_bg_opa(&st_cap, LV_OPA_COVER);
    lv_style_set_radius(&st_cap, 2);

    screen = lv_obj_create(NULL);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    for (int i = 0; i < ZAPPY_NUM_PYLONS; i++) {
        pylon_top[i] = plain_obj(screen);
        pylon_bot[i] = plain_obj(screen);
        lv_obj_add_style(pylon_top[i], &st_bar, LV_PART_MAIN);
        lv_obj_add_style(pylon_bot[i], &st_bar, LV_PART_MAIN);
        cap_top[i] = plain_obj(screen);
        cap_bot[i] = plain_obj(screen);
        lv_obj_add_style(cap_top[i], &st_cap, LV_PART_MAIN);
        lv_obj_add_style(cap_bot[i], &st_cap, LV_PART_MAIN);
        lv_obj_set_size(cap_top[i], ZAPPY_PYLON_W + 2 * CAP_OVERHANG, CAP_H);
        lv_obj_set_size(cap_bot[i], ZAPPY_PYLON_W + 2 * CAP_OVERHANG, CAP_H);
    }

    bird = plain_obj(screen);
    lv_obj_set_size(bird, BIRD_DRAW, BIRD_DRAW);
    lv_obj_set_style_radius(bird, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bird, lv_color_hex(NORD13), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bird, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_t *eye = plain_obj(bird);
    lv_obj_set_size(eye, 4, 4);
    lv_obj_set_pos(eye, 13, 5);
    lv_obj_set_style_radius(eye, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(eye, lv_color_hex(NORD0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, LV_PART_MAIN);
    bolt = lv_line_create(bird);
    lv_obj_set_style_line_color(bolt, lv_color_hex(NORD12), LV_PART_MAIN);
    lv_obj_set_style_line_width(bolt, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(bolt, true, LV_PART_MAIN);
    lv_line_set_points(bolt, bolt_up, 4);

    score_label = label(screen, &CaskaydiaCove_SemiBold_32, NORD6);
    lv_label_set_text_static(score_label, score_buf);
    lv_obj_align(score_label, LV_ALIGN_TOP_MID, 0, 10);

    /* hard mode: big target letter on top, score in the corner */
    target_label = label(screen, &CaskaydiaCove_Regular_48, NORD13);
    lv_label_set_text_static(target_label, target_buf);
    lv_obj_align(target_label, LV_ALIGN_TOP_MID, 0, 14);
    hard_score_label = label(screen, &CaskaydiaCove_SemiBold_20, NORD6);
    lv_label_set_text_static(hard_score_label, hard_score_buf);
    lv_obj_align(hard_score_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    title_box = box();
    lv_label_set_text_static(label(title_box, &CaskaydiaCove_Bold_32, NORD13), "ZAPPY\nBIRDS");
    lv_label_set_text_static(label(title_box, &CaskaydiaCove_SemiBold_20, NORD4), "any key: play\nH: hard");
    title_best = label(title_box, &CaskaydiaCove_SemiBold_20, NORD8);
    lv_label_set_text_static(title_best, title_best_buf);
    lv_obj_align(title_box, LV_ALIGN_TOP_MID, 0, 28);

    over_box = box();
    over_mode = label(over_box, &CaskaydiaCove_SemiBold_20, NORD12);
    lv_label_set_text_static(over_mode, "HARD");
    over_score = label(over_box, &CaskaydiaCove_Regular_48, NORD6);
    lv_label_set_text_static(over_score, over_score_buf);
    over_best = label(over_box, &CaskaydiaCove_SemiBold_20, NORD8);
    lv_label_set_text_static(over_best, over_best_buf);
    over_new = label(over_box, &CaskaydiaCove_SemiBold_20, NORD13);
    lv_label_set_text_static(over_new, "NEW BEST");
    lv_obj_align(over_box, LV_ALIGN_CENTER, 0, 0);

    return screen;
}

static void set_text_num(lv_obj_t *l, char *buf, size_t len, const char *fmt, unsigned v) {
    char tmp[16];
    snprintf(tmp, sizeof(tmp), fmt, v);
    if (strncmp(tmp, buf, len) != 0) {
        snprintf(buf, len, "%s", tmp);
        lv_label_set_text_static(l, buf); /* re-layout with the new text */
    }
}

static void set_text_str(lv_obj_t *l, char *buf, size_t len, const char *text) {
    if (strncmp(text, buf, len) != 0) {
        snprintf(buf, len, "%s", text);
        lv_label_set_text_static(l, buf);
    }
}

static void render_pylon(int i, const struct zappy_pylon *p, int16_t gap_h) {
    int16_t bot_y = p->gap_top + gap_h;
    lv_obj_set_pos(pylon_top[i], p->x, 0);
    lv_obj_set_size(pylon_top[i], ZAPPY_PYLON_W, p->gap_top);
    lv_obj_set_pos(pylon_bot[i], p->x, bot_y);
    lv_obj_set_size(pylon_bot[i], ZAPPY_PYLON_W, ZAPPY_H - bot_y);
    lv_obj_set_pos(cap_top[i], p->x - CAP_OVERHANG, p->gap_top - CAP_H);
    lv_obj_set_pos(cap_bot[i], p->x - CAP_OVERHANG, bot_y);
}

void zappy_screen_render(const struct zappy_game *g) {
    bool playing = g->phase == ZAPPY_PLAYING;
    bool title = g->phase == ZAPPY_TITLE;
    bool crashed = g->phase == ZAPPY_CRASHED;

    /* bird: draw box centered on the collision box */
    int16_t off = (BIRD_DRAW - ZAPPY_BIRD_SIZE) / 2;
    /* on the title the bird bobs below the text instead of behind it */
    int16_t title_drop = g->phase == ZAPPY_TITLE ? TITLE_BIRD_DROP : 0;
    lv_obj_set_pos(bird, ZAPPY_BIRD_X - off, zappy_bird_y(g) - off + title_drop);
    int pose = g->flap_frames ? 1 : 0;
    if (pose != last_flap_pose) {
        last_flap_pose = pose;
        lv_line_set_points(bolt, pose ? bolt_down : bolt_up, 4);
    }

    for (int i = 0; i < ZAPPY_NUM_PYLONS; i++) {
        bool show = !title;
        set_hidden(pylon_top[i], !show);
        set_hidden(pylon_bot[i], !show);
        set_hidden(cap_top[i], !show);
        set_hidden(cap_bot[i], !show);
        if (show)
            render_pylon(i, &g->pylons[i], g->gap_h);
    }

    bool hard = g->mode == ZAPPY_HARD;
    set_hidden(score_label, !(playing && !hard));
    set_hidden(hard_score_label, !(playing && hard));
    set_hidden(target_label, !(playing && hard));
    if (playing && !hard)
        set_text_num(score_label, score_buf, sizeof(score_buf), "%u", g->score);
    if (playing && hard) {
        set_text_num(hard_score_label, hard_score_buf, sizeof(hard_score_buf), "%u", g->score);
        char t[2] = {zappy_target_letter(g), 0};
        set_text_str(target_label, target_buf, sizeof(target_buf), t);
    }

    set_hidden(title_box, !title);
    if (title) {
        char t[24];
        snprintf(t, sizeof(t), "BEST %u  HARD %u", g->best, g->best_hard);
        set_text_str(title_best, title_best_buf, sizeof(title_best_buf), t);
    }

    set_hidden(over_box, !crashed);
    if (crashed) {
        set_text_num(over_score, over_score_buf, sizeof(over_score_buf), "%u", g->score);
        set_text_num(over_best, over_best_buf, sizeof(over_best_buf), "BEST %u",
                     hard ? g->best_hard : g->best);
        set_hidden(over_mode, !hard);
        set_hidden(over_new, !g->new_best);
    }

    /* crash flash: a brief red sky, then back to night */
    int flash = crashed && (uint32_t)(k_uptime_get_32() - g->crash_ms) < FLASH_MS;
    if (flash != last_flash) {
        last_flash = flash;
        lv_obj_set_style_bg_color(screen, lv_color_hex(flash ? NORD11 : THEME_BG), LV_PART_MAIN);
    }
}
