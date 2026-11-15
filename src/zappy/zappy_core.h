/*
 * Zappy Birds game rules. Pure C (no Zephyr/LVGL) so it can be unit-tested
 * on the host: see tests/zappy.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ZAPPY_W 240
#define ZAPPY_H 280
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
#define ZAPPY_EXIT_IDLE_MS 300 /* exit combo needs this long without a flap */

enum zappy_phase { ZAPPY_TITLE, ZAPPY_PLAYING, ZAPPY_CRASHED };

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
};

void zappy_init(struct zappy_game *g, uint16_t best, uint32_t seed);
/* Returns true if the press did something (start, flap, restart). */
bool zappy_press(struct zappy_game *g, uint32_t now_ms);
void zappy_step(struct zappy_game *g, uint32_t now_ms);
int16_t zappy_bird_y(const struct zappy_game *g);
/* Fixed timestep: add elapsed ms to *acc, return how many zappy_step()s
 * are due (one per ZAPPY_TICK_MS), capped at ZAPPY_MAX_CATCHUP_STEPS. */
uint8_t zappy_steps_due(uint32_t *acc, uint32_t elapsed_ms);
/* Exit only after a pause in flapping, so two-handed mashing can't quit. */
bool zappy_exit_allowed(uint32_t now_ms, uint32_t last_flap_ms);
