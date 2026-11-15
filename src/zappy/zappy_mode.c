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
#include <zephyr/toolchain.h>

#include <zmk/display.h>
#include <zmk/keymap.h>

#include "zappy_core.h"
#include "zappy_screen.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define IDLE_EXIT_MS 60000

static atomic_t active;
static atomic_t pending_flaps;
static atomic_t last_input_ms;
static atomic_t layer_id;
static atomic_t best;

/* display work queue only */
static struct zappy_game game;
static lv_obj_t *status_screen;
static lv_timer_t *timer;
static uint16_t saved_best;

__weak void zappy_store_save(uint16_t b) { ARG_UNUSED(b); }

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

    if (atomic_set(&pending_flaps, 0) > 0)
        zappy_press(&game, now); /* several presses in one tick = one flap */
    zappy_step(&game, now);
    zappy_screen_render(&game);

    if (game.best > saved_best) {
        saved_best = game.best;
        atomic_set(&best, game.best);
        zappy_store_save(game.best);
    }

    if (now - (uint32_t)atomic_get(&last_input_ms) > IDLE_EXIT_MS)
        k_work_submit(&deactivate_work);
}

static void show_game(void) {
    lv_obj_t *cur = lv_scr_act();
    if (!zmk_display_is_initialized() || !cur) {
        LOG_WRN("zappy: display not ready, leaving game mode");
        k_work_submit(&deactivate_work);
        return;
    }
    lv_obj_t *scr = zappy_screen_create();
    if (cur != scr)
        status_screen = cur;
    saved_best = (uint16_t)atomic_get(&best);
    zappy_init(&game, saved_best, k_cycle_get_32() | 1);
    atomic_set(&pending_flaps, 0);
    zappy_screen_render(&game);
    lv_scr_load(scr);
    timer = lv_timer_create(tick_cb, ZAPPY_TICK_MS, NULL);
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

void zappy_mode_flap(void) {
    if (!atomic_get(&active))
        return;
    atomic_set(&last_input_ms, (atomic_val_t)k_uptime_get_32());
    atomic_inc(&pending_flaps);
}

bool zappy_mode_active(void) { return atomic_get(&active); }

uint16_t zappy_mode_best(void) { return (uint16_t)atomic_get(&best); }

void zappy_mode_set_best(uint16_t b) { atomic_set(&best, b); }
