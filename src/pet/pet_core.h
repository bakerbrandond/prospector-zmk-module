/*
 * Typing pet (Clackling) rules. Pure C (no Zephyr/LVGL) so it can be
 * unit-tested on the host: see tests/pet.
 *
 * Two counters: keys is the true lifetime press count; growth_half drives the
 * stages, gaining 2 per press (1 while hungry), i.e. growth in half-points.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PET_FULL_MS 86400000u /* full to empty in 24 h of uptime */

enum pet_stage { PET_EGG, PET_BABY, PET_CHILD, PET_TEEN, PET_ADULT, PET_STAGES };

struct pet_state {
    uint32_t keys;        /* true lifetime presses */
    uint32_t growth_half; /* growth in half-points */
    uint32_t fullness_ms; /* 0..PET_FULL_MS */
    uint8_t shown_stage;  /* last stage celebrated on screen */
};

void pet_init(struct pet_state *p); /* fresh egg, full */
enum pet_stage pet_stage_of(uint32_t growth_half);
/* growth value (whole points) of the next stage, 0 if Adult */
uint32_t pet_next_threshold(uint32_t growth_half);
bool pet_hungry(const struct pet_state *p);
/* Returns true if the stage went up. */
bool pet_add_presses(struct pet_state *p, uint32_t n);
/* elapsed_ms is a difference of unsigned uptimes (now - then), so it wraps safely */
void pet_elapse(struct pet_state *p, uint32_t elapsed_ms);
void pet_feed(struct pet_state *p);
