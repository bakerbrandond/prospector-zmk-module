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
/* 0..100 from the current stage's start to the next one; 100 at Adult */
uint8_t pet_progress_pct(uint32_t growth_half);
bool pet_hungry(const struct pet_state *p);
/* Returns true if the stage went up. */
bool pet_add_presses(struct pet_state *p, uint32_t n);
/* elapsed_ms is a difference of unsigned uptimes (now - then), so it wraps safely */
void pet_elapse(struct pet_state *p, uint32_t elapsed_ms);
void pet_feed(struct pet_state *p);

/* Mood, idle and saving */
#define PET_SLEEP_IDLE_MS 120000u /* asleep after 2 min without a key press */
#define PET_SAVE_EVERY_MS 900000u /* save at most every 15 min */
#define PET_BOOST_MS 300000u      /* a won mini-game cheers it up this long */
#define PET_EXCITED_WPM 60
#define PET_HAPPY_WPM 25

/* Hearts shown = mood level (0..4) */
enum pet_mood { PET_ASLEEP, PET_SLEEPY, PET_CALM, PET_HAPPY, PET_EXCITED };

enum pet_mood pet_mood_of(int wpm, uint32_t idle_ms, bool boost);
bool pet_save_due(uint32_t since_save_ms, bool dirty);

/* Saved form: version, keys, growth_half, fullness_ms (little-endian), shown_stage */
#define PET_BLOB_LEN 14
#define PET_BLOB_VERSION 1
void pet_pack(const struct pet_state *p, uint8_t out[PET_BLOB_LEN]);
/* false (and *out untouched) if the length or version is wrong */
bool pet_unpack(const uint8_t *buf, size_t len, struct pet_state *out);

/* Mini-game: the pet hides left or right; guess with a key on that half. */
#define PET_GAME_ROUNDS 5
#define PET_ROUND_MS 2000u
#define PET_REVEAL_MS 600u
#define PET_RESULT_MS 2000u
#define PET_GAME_CAP_MS 16000u /* hard stop from start, whatever the phase */
#define PET_GAME_WIN 3         /* score that earns the boost */

enum pet_side { PET_LEFT, PET_RIGHT };
enum pet_game_phase { PET_GAME_OFF, PET_GAME_ROUND, PET_GAME_REVEAL, PET_GAME_RESULT };

struct pet_game {
    enum pet_game_phase phase;
    uint8_t round, score;
    enum pet_side hide; /* where the pet hides this round */
    bool last_hit;      /* REVEAL: was the guess right */
    uint32_t start_ms, phase_end_ms, rng;
};

/* Corne 42-key positions: columns 0-5 of rows 0-2 and thumbs 36-38 are left */
enum pet_side pet_side_of_position(uint8_t pos);
void pet_game_start(struct pet_game *g, uint32_t seed, uint32_t now_ms);
void pet_game_guess(struct pet_game *g, enum pet_side side, uint32_t now_ms); /* ROUND only */
void pet_game_step(struct pet_game *g, uint32_t now_ms);
/* true while the capture layer must stay on (ROUND or REVEAL) */
bool pet_game_capturing(const struct pet_game *g);
bool pet_game_won(const struct pet_game *g); /* RESULT and score >= PET_GAME_WIN */
