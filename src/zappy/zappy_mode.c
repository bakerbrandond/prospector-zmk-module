/*
 * Zappy Birds mode controller.
 *
 * Threads: zappy_mode_toggle()/zappy_mode_flap() run in behavior context and
 * only touch atomics and keymap layers. Everything LVGL happens on the
 * display work queue, in sync_work (screen swap) and the game lv_timer.
 * sync_work always reconciles the screen with the latest `active` value, so
 * a fast enter/exit/enter can't leave the screen and layer out of step.
 */
#include "zappy_mode.h"

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <zmk/behavior.h>
#include <zmk/display.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>

#include "zappy_core.h"
#include "zappy_screen.h"
#include "../matrix/matrix_mode.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static atomic_t active;
static atomic_t flaps_lo, flaps_hi; /* bitmask of pressed positions 0-31, 32-63 */
static atomic_t last_input_ms;
static atomic_t layer_id;
static atomic_t best, best_hard;

/* display work queue only */
static struct zappy_game game;
static lv_obj_t *status_screen;
static lv_timer_t *timer;
static uint16_t saved_best, saved_best_hard;
static struct zappy_pool pool;
static bool pool_built;
static uint32_t last_tick_ms, step_acc;

/* LVGL runs every CONFIG_ZMK_DISPLAY_TICK_PERIOD_MS (10 ms), so a 33 ms
 * timer really fires every 40 ms. Poll each pass and step on a fixed
 * ZAPPY_TICK_MS timestep instead, so game speed follows real time. */
#define POLL_MS 10

/* Overridden by the matrix screensaver when it's built. */
__weak void matrix_mode_hide_now(void) {}

__weak void zappy_store_save(enum zappy_mode m, uint16_t b) {
    ARG_UNUSED(m);
    ARG_UNUSED(b);
}

/* Letter keys of the Default layer. &kp X gives X; other behaviors (hold-taps
 * like &hrm MOD X) give their second parameter, the tap key. */
static void build_pool(void) {
    zmk_keymap_layer_id_t layer = zmk_keymap_layer_default();
    pool = (struct zappy_pool){.h_pos = ZAPPY_NO_POS};
    for (uint32_t i = 0; i < ZMK_KEYMAP_LEN && i < ZAPPY_NO_POS; i++) {
        const struct zmk_behavior_binding *b = zmk_keymap_get_layer_binding_at_idx(layer, i);
        if (!b || !b->behavior_dev)
            continue;
        uint32_t usage = strcmp(b->behavior_dev, "key_press") == 0 ? b->param1 : b->param2;
        zappy_pool_add(&pool, (uint8_t)i, usage);
    }
    pool_built = true;
    LOG_INF("zappy: %u letter keys, H at %u", pool.n, pool.h_pos);
}

/* Try each pressed position in order until one is accepted (one flap per
 * pass; in hard mode the target key counts even among other presses). */
static bool apply_presses(uint32_t now) {
    uint32_t lo = (uint32_t)atomic_set(&flaps_lo, 0), hi = (uint32_t)atomic_set(&flaps_hi, 0);
    bool any = lo || hi;
    for (int i = 0; i < 64 && (lo || hi); i++) {
        uint32_t *w = i < 32 ? &lo : &hi;
        uint32_t bit = BIT(i % 32);
        if (!(*w & bit))
            continue;
        *w &= ~bit;
        if (zappy_press(&game, now, (uint8_t)i))
            break;
    }
    return any;
}

static void deactivate_work_cb(struct k_work *work);
static K_WORK_DEFINE(deactivate_work, deactivate_work_cb);
static void sync_work_cb(struct k_work *work);
static K_WORK_DEFINE(sync_work, sync_work_cb);

static void submit_sync(void) { k_work_submit_to_queue(zmk_display_work_q(), &sync_work); }

/* system work queue: leave game mode (idle timeout or display not ready) */
static void deactivate_work_cb(struct k_work *work) {
    if (atomic_cas(&active, 1, 0)) {
        zmk_keymap_layer_deactivate((zmk_keymap_layer_id_t)atomic_get(&layer_id));
        submit_sync();
    }
}

static void tick_cb(lv_timer_t *t) {
    uint32_t now = k_uptime_get_32();

    bool pressed = apply_presses(now);
    uint8_t steps = zappy_steps_due(&step_acc, now - last_tick_ms);
    last_tick_ms = now;
    for (uint8_t i = 0; i < steps; i++)
        zappy_step(&game, now);
    if (steps || pressed)
        zappy_screen_render(&game);

    if (game.best > saved_best) {
        saved_best = game.best;
        atomic_set(&best, game.best);
        zappy_store_save(ZAPPY_NORMAL, game.best);
    }
    if (game.best_hard > saved_best_hard) {
        saved_best_hard = game.best_hard;
        atomic_set(&best_hard, game.best_hard);
        zappy_store_save(ZAPPY_HARD, game.best_hard);
    }

    if (zappy_idle_expired(now, (uint32_t)atomic_get(&last_input_ms)))
        k_work_submit(&deactivate_work);
}

static void show_game(void) {
    matrix_mode_hide_now(); /* never save the screensaver as the status screen */
    lv_obj_t *cur = lv_scr_act();
    if (!zmk_display_is_initialized() || !cur) {
        LOG_WRN("zappy: display not ready, leaving game mode");
        k_work_submit(&deactivate_work);
        return;
    }
    lv_obj_t *scr = zappy_screen_create();
    if (cur != scr)
        status_screen = cur;
    if (!pool_built)
        build_pool();
    saved_best = (uint16_t)atomic_get(&best);
    saved_best_hard = (uint16_t)atomic_get(&best_hard);
    zappy_init(&game, saved_best, saved_best_hard, k_cycle_get_32() | 1, &pool);
    atomic_set(&flaps_lo, 0);
    atomic_set(&flaps_hi, 0);
    zappy_screen_render(&game);
    lv_scr_load(scr);
    last_tick_ms = k_uptime_get_32();
    step_acc = 0;
    timer = lv_timer_create(tick_cb, POLL_MS, NULL);
}

static void hide_game(void) {
    lv_timer_del(timer);
    timer = NULL;
    if (status_screen)
        lv_scr_load(status_screen);
}

static void sync_work_cb(struct k_work *work) {
    bool want = atomic_get(&active);
    bool shown = timer != NULL;
    if (want && !shown)
        show_game();
    else if (!want && shown)
        hide_game();
}

void zappy_mode_toggle(uint8_t layer) {
    if (atomic_cas(&active, 0, 1)) {
        atomic_set(&layer_id, layer);
        atomic_set(&last_input_ms, (atomic_val_t)k_uptime_get_32());
        zmk_keymap_layer_activate(layer);
    } else if (zappy_exit_allowed(k_uptime_get_32(), (uint32_t)atomic_get(&last_input_ms)) &&
               atomic_cas(&active, 1, 0)) {
        /* ZMK's combo require-prior-idle-ms can't see &zflap presses (they
         * emit no keycode), so the pause-before-exit guard lives here. */
        zmk_keymap_layer_deactivate((zmk_keymap_layer_id_t)atomic_get(&layer_id));
    }
    submit_sync();
}

void zappy_mode_flap(uint8_t position) {
    if (!atomic_get(&active) || position >= 64)
        return;
    atomic_set(&last_input_ms, (atomic_val_t)k_uptime_get_32());
    atomic_or(position < 32 ? &flaps_lo : &flaps_hi, (atomic_val_t)BIT(position % 32));
}

bool zappy_mode_active(void) { return atomic_get(&active); }

void zappy_mode_set_best(enum zappy_mode m, uint16_t b) {
    atomic_set(m == ZAPPY_HARD ? &best_hard : &best, b);
}
