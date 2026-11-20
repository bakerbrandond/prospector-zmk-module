/*
 * Typing pet panel: the LVGL widget that takes the WPM panel's place on the
 * status screen (same size and position). Display work queue only.
 */
#pragma once

#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>

#include "pet_core.h"
#include "pet_sprites.h"

#define PET_PANEL_W 204
#define PET_PANEL_H 64

/* Everything the panel shows, snapshotted by pet_mode each frame */
struct pet_view {
    enum pet_stage stage;
    enum pet_pose pose;
    enum pet_mood mood;   /* hearts = mood level 0..4 */
    uint8_t fullness_pips; /* 0..4 */
    uint32_t keys;
    int wpm;
    uint8_t progress;      /* 0..100 towards the next stage */
    uint32_t next;         /* next stage's growth value, 0 at Adult */
    enum pet_game_phase game;
    uint8_t score, round;
    enum pet_side side;    /* REVEAL: where it was hiding */
    bool hit;              /* REVEAL: guessed right */
};

/* Opaque THEME_BG box of PET_PANEL_W x PET_PANEL_H, created hidden */
lv_obj_t *pet_panel_create(lv_obj_t *parent);
void pet_panel_render(const struct pet_view *v);

/* From custom_status_screen.c: the two widgets pet_mode swaps */
void pet_mode_attach(lv_obj_t *wpm_panel, lv_obj_t *pet_panel);
