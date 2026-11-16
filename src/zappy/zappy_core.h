/*
 * Zappy Birds game rules. Pure C (no Zephyr/LVGL) so it can be unit-tested
 * on the host: see tests/zappy.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../display_size.h"

#define ZAPPY_W DISPLAY_W
#define ZAPPY_H DISPLAY_H
#define ZAPPY_TICK_MS 33
#define ZAPPY_CRASH_LOCKOUT_MS 600
#define ZAPPY_NUM_PYLONS 3
#define ZAPPY_BIRD_X 60
#define ZAPPY_BIRD_SIZE 16 /* collision box, square */
#define ZAPPY_PYLON_W 36
#define ZAPPY_GAP_H 92
#define ZAPPY_MARGIN 24 /* min distance of gap edge from top/bottom */
#define ZAPPY_SCORE_MAX 9999
#define ZAPPY_MAX_CATCHUP_STEPS 3 /* after a stall, don't fast-forward further */
#define ZAPPY_IDLE_EXIT_MS 60000 /* leave game mode after this long untouched */
#define ZAPPY_EXIT_IDLE_MS 300 /* exit combo needs this long without a flap */

enum zappy_phase { ZAPPY_TITLE, ZAPPY_PLAYING, ZAPPY_CRASHED };
enum zappy_mode { ZAPPY_NORMAL, ZAPPY_HARD };

#define ZAPPY_MAX_POOL 42
#define ZAPPY_NO_POS 0xFF
#define ZAPPY_HARD_GAP_H (ZAPPY_GAP_H + 24)
/* H this soon after the press that started a normal run switches it to hard */
#define ZAPPY_HARD_GRACE_MS 150

/* Letter keys usable as hard-mode targets: key position + its letter. */
struct zappy_pool {
    uint8_t n;
    uint8_t pos[ZAPPY_MAX_POOL];
    char letter[ZAPPY_MAX_POOL];
    uint8_t h_pos; /* position of 'H', or ZAPPY_NO_POS */
};

struct zappy_pylon {
    int16_t x;
    int16_t gap_top;
    bool scored;
};

struct zappy_game {
    enum zappy_phase phase;
    int32_t bird_y_fp; /* top of bird, 24.8 fixed point */
    int32_t vel_fp;    /* +down, 24.8 */
    struct zappy_pylon pylons[ZAPPY_NUM_PYLONS];
    uint16_t score, best;
    bool new_best;
    uint32_t crash_ms; /* now_ms when crashed */
    uint32_t rng;      /* xorshift32 state, never 0 */
    uint8_t flap_frames; /* >0 while showing flap pose */
    uint16_t ticks;      /* frame counter (title bob, animation) */
    enum zappy_mode mode;
    uint16_t best_hard;
    int16_t gap_h;          /* gap height for the current mode */
    struct zappy_pool pool;
    uint32_t run_start_ms; /* now_ms of the press that started the run */
    uint8_t target;         /* hard mode: index into pool */
};

/* 'A'..'Z' for keyboard-page usages A..Z (mod bits ignored), else 0 */
char zappy_usage_letter(uint32_t usage);
/* Adds position if usage is a letter; records h_pos; ignores when full. */
void zappy_pool_add(struct zappy_pool *p, uint8_t position, uint32_t usage);

/* pool may be NULL (no hard mode). */
void zappy_init(struct zappy_game *g, uint16_t best, uint16_t best_hard, uint32_t seed,
                const struct zappy_pool *pool);
/* Key at `position` pressed. Returns true if it did something (start, flap,
 * restart). On the title, H starts hard mode (pool of 2+ letters); in hard
 * play only the target key flaps. */
bool zappy_press(struct zappy_game *g, uint32_t now_ms, uint8_t position);
/* Current target letter in hard mode, else 0. */
char zappy_target_letter(const struct zappy_game *g);
void zappy_step(struct zappy_game *g, uint32_t now_ms);
int16_t zappy_bird_y(const struct zappy_game *g);
/* Fixed timestep: add elapsed ms to *acc, return how many zappy_step()s
 * are due (one per ZAPPY_TICK_MS), capped at ZAPPY_MAX_CATCHUP_STEPS. */
uint8_t zappy_steps_due(uint32_t *acc, uint32_t elapsed_ms);
/* Exit only after a pause in flapping, so two-handed mashing can't quit. */
bool zappy_exit_allowed(uint32_t now_ms, uint32_t last_flap_ms);
/* True once ZAPPY_IDLE_EXIT_MS has passed with no input. Input stamped after
 * now_ms (another thread, mid-tick) counts as fresh, not as a huge gap. */
bool zappy_idle_expired(uint32_t now_ms, uint32_t last_input_ms);
