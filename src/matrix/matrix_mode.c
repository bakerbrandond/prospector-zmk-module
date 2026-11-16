/*
 * Matrix rain screensaver controller.
 *
 * Idle detection rides on ZMK's activity tracking, whose listener runs ahead
 * of the keymap and sees every key press (the jiggler's mouse moves don't
 * count). IDLE (after CONFIG_ZMK_IDLE_TIMEOUT) arms a delayed work item for
 * the rest of the screensaver timeout; ACTIVE cancels it and wakes. The
 * activity event is raised while the waking key press is being dispatched,
 * before its binding runs, so the wake is queued ahead of anything the key
 * itself queues (e.g. Zappy Birds' screen swap).
 *
 * Threads: listener and delayed work touch only atomics and submit work.
 * All LVGL work happens on the display queue in sync_work and the timer;
 * sync_work reconciles the screen with the latest want_rain value.
 */
#include "matrix_mode.h"

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

#include "matrix_core.h"
#include "matrix_screen.h"

#if IS_ENABLED(CONFIG_PROSPECTOR_ZAPPY_BIRDS)
#include "../zappy/zappy_mode.h"
#define GAME_ACTIVE() zappy_mode_active()
#else
#define GAME_ACTIVE() false
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

BUILD_ASSERT(CONFIG_PROSPECTOR_MATRIX_TIMEOUT_S * 1000 >= CONFIG_ZMK_IDLE_TIMEOUT,
             "screensaver timeout must be at least ZMK's idle timeout");

#define REMAINING_MS (CONFIG_PROSPECTOR_MATRIX_TIMEOUT_S * 1000 - CONFIG_ZMK_IDLE_TIMEOUT)
#define GAME_RECHECK_S 10
#define POLL_MS 10  /* LVGL is serviced every 10 ms */
#define STEP_MS 66  /* ~15 fps */
#define MAX_CATCHUP 2

static atomic_t want_rain;

/* display work queue only */
static struct matrix_state model;
static uint16_t pending_dirty[MATRIX_COLS];
static lv_obj_t *rain_screen, *saved_screen;
static lv_timer_t *timer;
static uint32_t last_ms, acc_ms;

static void sync_work_cb(struct k_work *work);
static K_WORK_DEFINE(sync_work, sync_work_cb);
static void idle_work_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(idle_work, idle_work_cb);

static void submit_sync(void) { k_work_submit_to_queue(zmk_display_work_q(), &sync_work); }

static void tick_cb(lv_timer_t *t) {
    uint32_t now = k_uptime_get_32();
    acc_ms += now - last_ms;
    last_ms = now;
    int steps = 0;
    while (acc_ms >= STEP_MS && steps < MAX_CATCHUP) {
        acc_ms -= STEP_MS;
        matrix_step(&model);
        for (int c = 0; c < MATRIX_COLS; c++)
            pending_dirty[c] |= model.dirty[c];
        steps++;
    }
    if (acc_ms >= STEP_MS)
        acc_ms = 0; /* after a stall, don't fast-forward */
    if (steps) {
        matrix_screen_render(&model, pending_dirty);
        for (int c = 0; c < MATRIX_COLS; c++)
            pending_dirty[c] = 0;
    }
}

static void show_rain(void) {
    if (!zmk_display_is_initialized() || GAME_ACTIVE()) {
        atomic_set(&want_rain, 0);
        return;
    }
    lv_obj_t *scr = matrix_screen_create();
    if (!scr) { /* out of LVGL memory: skip this round, the next idle retries */
        atomic_set(&want_rain, 0);
        return;
    }
    lv_obj_t *cur = lv_scr_act();
    if (cur != scr)
        saved_screen = cur;
    rain_screen = scr;
    matrix_init(&model, k_cycle_get_32() | 1);
    matrix_screen_render(&model, pending_dirty); /* bind the model; screen load redraws all */
    lv_scr_load(scr);
    last_ms = k_uptime_get_32();
    acc_ms = 0;
    timer = lv_timer_create(tick_cb, POLL_MS, NULL);
    if (!timer) {
        /* no timer means "not shown" to sync_work, so nothing would ever
         * take the rain down: put the old screen back now */
        LOG_WRN("matrix: no memory for the rain timer");
        atomic_set(&want_rain, 0);
        if (saved_screen)
            lv_scr_load(saved_screen);
    }
}

static void hide_rain(void) {
    lv_timer_del(timer);
    timer = NULL;
    /* only restore if nothing else (the game) has taken the screen since */
    if (saved_screen && lv_scr_act() == rain_screen)
        lv_scr_load(saved_screen);
}

static void sync_work_cb(struct k_work *work) {
    bool want = atomic_get(&want_rain);
    bool shown = timer != NULL;
    if (want && !shown)
        show_rain();
    else if (!want && shown)
        hide_rain();
}

void matrix_mode_hide_now(void) {
    atomic_set(&want_rain, 0);
    if (timer)
        hide_rain();
}

static void idle_work_cb(struct k_work *work) {
    if (GAME_ACTIVE()) {
        k_work_reschedule(&idle_work, K_SECONDS(GAME_RECHECK_S));
        return;
    }
    atomic_set(&want_rain, 1);
    submit_sync();
}

static int matrix_activity_listener(const zmk_event_t *eh) {
    const struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (!ev)
        return ZMK_EV_EVENT_BUBBLE;
    if (ev->state == ZMK_ACTIVITY_IDLE) {
        k_work_reschedule(&idle_work, K_MSEC(REMAINING_MS));
    } else {
        k_work_cancel_delayable(&idle_work);
        if (atomic_cas(&want_rain, 1, 0))
            submit_sync();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(matrix_screensaver, matrix_activity_listener);
ZMK_SUBSCRIPTION(matrix_screensaver, zmk_activity_state_changed);
