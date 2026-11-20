/*
 * Typing pet controller.
 *
 * Threads:
 * - Key presses arrive in a ZMK event listener, which only touches atomics.
 * - The life tick (system work queue) applies them to the pet, drains its
 *   fullness and saves now and then. It runs every second while the pet is
 *   shown and every minute otherwise.
 * - Commands (behavior context) take the same mutex for the pet and the game.
 * - The display work queue swaps the WPM panel and the pet panel, and runs the
 *   panel timer (game steps, mood, rendering) while the pet is shown.
 * - Keymap layers are only changed on the system work queue, like Zappy's.
 */
#include "pet_mode.h"

#include <lvgl.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>

#include "pet_panel.h"
#include "pet_sprites.h"

#if IS_ENABLED(CONFIG_PROSPECTOR_ZAPPY_BIRDS)
#include "../zappy/zappy_mode.h"
#define ZAPPY_ACTIVE() zappy_mode_active()
#else
#define ZAPPY_ACTIVE() false
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define TICK_SHOWN_MS 1000
#define TICK_HIDDEN_MS 60000
#define FRAME_MS 100
#define CELEBRATE_MS 3000

__weak void pet_store_save(const struct pet_state *p) { ARG_UNUSED(p); }
__weak void pet_store_save_shown(bool shown) { ARG_UNUSED(shown); }

static atomic_t shown;
static atomic_t pending_presses;
static atomic_t last_press_ms;
static atomic_t capturing; /* capture layer is on */
static atomic_t capture_layer;

/* guarded by lock */
static K_MUTEX_DEFINE(lock);
static struct pet_state pet = {.fullness_ms = PET_FULL_MS};
static struct pet_game game;
static bool dirty;
static bool boosting;
static uint32_t boost_start;
static bool result_seen;

/* system work queue only */
static uint32_t last_life_ms, last_save_ms;

/* display work queue only */
static lv_obj_t *wpm_obj, *pet_obj;
static lv_timer_t *frame_timer;
static uint32_t celebrate_start;
static bool celebrating;

static void life_work_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(life_work, life_work_cb);
static void release_work_cb(struct k_work *work);
static K_WORK_DEFINE(release_work, release_work_cb);
static void capture_work_cb(struct k_work *work);
static K_WORK_DEFINE(capture_work, capture_work_cb);
static void sync_work_cb(struct k_work *work);
static K_WORK_DEFINE(sync_work, sync_work_cb);

static void submit_sync(void) { k_work_submit_to_queue(zmk_display_work_q(), &sync_work); }

/* ---- key presses ---- */

static int pet_position_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (!ev || !ev->state)
        return ZMK_EV_EVENT_BUBBLE;
    atomic_set(&last_press_ms, (atomic_val_t)k_uptime_get_32());
    if (!ZAPPY_ACTIVE() && !atomic_get(&capturing))
        atomic_inc(&pending_presses);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(typing_pet, pet_position_listener);
ZMK_SUBSCRIPTION(typing_pet, zmk_position_state_changed);

/* ---- life tick (system work queue) ---- */

static void life_work_cb(struct k_work *work) {
    uint32_t now = k_uptime_get_32();
    uint32_t n = (uint32_t)atomic_set(&pending_presses, 0);
    struct pet_state copy;
    bool save;

    k_mutex_lock(&lock, K_FOREVER);
    bool was_hungry = pet_hungry(&pet);
    bool stage_up = n && pet_add_presses(&pet, n);
    pet_elapse(&pet, now - last_life_ms);
    /* presses, feeding and turning hungry mark it dirty; the slow drain alone
     * doesn't, so an untouched pet isn't rewritten every 15 minutes */
    dirty |= n > 0 || pet_hungry(&pet) != was_hungry;
    save = stage_up || pet_save_due(now - last_save_ms, dirty);
    if (save) {
        copy = pet;
        dirty = false;
    }
    k_mutex_unlock(&lock);
    last_life_ms = now;

    if (save) {
        last_save_ms = now;
        pet_store_save(&copy);
    }
    k_work_reschedule(&life_work, K_MSEC(atomic_get(&shown) ? TICK_SHOWN_MS : TICK_HIDDEN_MS));
}

/* ---- capture layer (system work queue) ---- */

static void capture_work_cb(struct k_work *work) {
    zmk_keymap_layer_activate((zmk_keymap_layer_id_t)atomic_get(&capture_layer));
}

static void release_work_cb(struct k_work *work) {
    zmk_keymap_layer_deactivate((zmk_keymap_layer_id_t)atomic_get(&capture_layer));
}

/* turn the capture layer off once, whoever notices first */
static void release_capture(void) {
    if (atomic_cas(&capturing, 1, 0))
        k_work_submit(&release_work);
}

void pet_mode_end_game(void) {
    k_mutex_lock(&lock, K_FOREVER);
    game.phase = PET_GAME_OFF;
    k_mutex_unlock(&lock);
    release_capture();
}

/* ---- commands (behavior context) ---- */

void pet_mode_command(enum pet_cmd cmd, uint8_t layer) {
    switch (cmd) {
    case PET_CMD_TOGGLE: {
        bool now_shown = !atomic_get(&shown);
        atomic_set(&shown, now_shown);
        if (!now_shown)
            pet_mode_end_game();
        pet_store_save_shown(now_shown);
        k_work_reschedule(&life_work, K_NO_WAIT);
        submit_sync();
        break;
    }
    case PET_CMD_FEED:
        if (!atomic_get(&shown))
            break;
        k_mutex_lock(&lock, K_FOREVER);
        pet_feed(&pet);
        dirty = true;
        k_mutex_unlock(&lock);
        break;
    case PET_CMD_PLAY:
        if (!atomic_get(&shown) || ZAPPY_ACTIVE())
            break;
        k_mutex_lock(&lock, K_FOREVER);
        bool start = game.phase == PET_GAME_OFF;
        if (start) {
            pet_game_start(&game, k_cycle_get_32(), k_uptime_get_32());
            result_seen = false;
        }
        k_mutex_unlock(&lock);
        if (start && atomic_cas(&capturing, 0, 1)) {
            atomic_set(&capture_layer, layer);
            k_work_submit(&capture_work);
        }
        break;
    }
}

void pet_mode_guess(uint8_t position) {
    if (!atomic_get(&capturing))
        return;
    k_mutex_lock(&lock, K_FOREVER);
    pet_game_guess(&game, pet_side_of_position(position), k_uptime_get_32());
    k_mutex_unlock(&lock);
}

bool pet_mode_shown(void) { return atomic_get(&shown); }

/* ---- display work queue ---- */

static enum pet_pose pick_pose(const struct pet_view *v, bool hungry, uint32_t now) {
    if (celebrating || (v->game == PET_GAME_RESULT && v->score >= PET_GAME_WIN))
        return PET_POSE_CHEER;
    if (v->game == PET_GAME_REVEAL)
        return v->hit ? PET_POSE_CHEER : PET_POSE_IDLE;
    if (v->mood == PET_ASLEEP)
        return PET_POSE_SLEEP;
    if (hungry)
        return PET_POSE_HUNGRY;
    /* hop in time with the mood: faster when excited */
    if (v->mood == PET_EXCITED)
        return (now / 250) % 2 ? PET_POSE_BOUNCE : PET_POSE_IDLE;
    if (v->mood == PET_HAPPY)
        return (now / 600) % 2 ? PET_POSE_BOUNCE : PET_POSE_IDLE;
    return PET_POSE_IDLE;
}

static void frame_cb(lv_timer_t *t) {
    uint32_t now = k_uptime_get_32();
    struct pet_view v = {0};
    bool hungry, game_capturing;
    enum pet_stage stage;

    k_mutex_lock(&lock, K_FOREVER);
    pet_game_step(&game, now);
    if (game.phase == PET_GAME_RESULT && !result_seen) {
        result_seen = true;
        if (pet_game_won(&game)) {
            boosting = true;
            boost_start = now;
        }
    }
    game_capturing = pet_game_capturing(&game);
    stage = pet_stage_of(pet.growth_half);
    /* a stage reached since it was last on screen: celebrate, then remember */
    if (!celebrating && pet.shown_stage < stage) {
        celebrating = true;
        celebrate_start = now;
    }
    if (celebrating && !pet_within(celebrate_start, now, CELEBRATE_MS)) {
        celebrating = false;
        pet.shown_stage = stage;
        dirty = true;
    }
    v.stage = stage;
    v.keys = pet.keys;
    v.fullness_pips = (uint8_t)((pet.fullness_ms + PET_FULL_MS / 4 - 1) / (PET_FULL_MS / 4));
    hungry = pet_hungry(&pet);
    v.next = pet_next_threshold(pet.growth_half);
    v.progress = pet_progress_pct(pet.growth_half);
    v.game = game.phase;
    v.score = game.score;
    v.round = game.round;
    v.side = game.hide;
    v.hit = game.last_hit;
    if (boosting && !pet_within(boost_start, now, PET_BOOST_MS))
        boosting = false;
    bool boost = boosting;
    k_mutex_unlock(&lock);

    if (!game_capturing)
        release_capture();

    int wpm = zmk_wpm_get_state();
    if (wpm < 0 || wpm > 250)
        wpm = 0; /* ZMK reports 255 until the first calculation */
    v.wpm = wpm;
    v.mood = pet_mood_of(wpm, now - (uint32_t)atomic_get(&last_press_ms), boost);
    v.pose = pick_pose(&v, hungry, now);
    pet_panel_render(&v);
}

static void sync_work_cb(struct k_work *work) {
    if (!wpm_obj || !pet_obj)
        return;
    bool want = atomic_get(&shown);
    if (want) {
        lv_obj_add_flag(wpm_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(pet_obj, LV_OBJ_FLAG_HIDDEN);
        if (!frame_timer) {
            frame_timer = lv_timer_create(frame_cb, FRAME_MS, NULL);
            frame_cb(NULL);
        }
    } else {
        if (frame_timer) {
            lv_timer_del(frame_timer);
            frame_timer = NULL;
        }
        lv_obj_add_flag(pet_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(wpm_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void pet_mode_attach(lv_obj_t *wpm_panel, lv_obj_t *pet_panel) {
    wpm_obj = wpm_panel;
    pet_obj = pet_panel;
    sync_work_cb(NULL); /* already on the display work queue */
}

/* ---- settings (at boot) ---- */

void pet_mode_load_state(const struct pet_state *saved) {
    k_mutex_lock(&lock, K_FOREVER);
    pet = *saved;
    dirty = false;
    k_mutex_unlock(&lock);
}

void pet_mode_load_shown(bool s) {
    atomic_set(&shown, s);
    submit_sync();
}

static int pet_mode_init(void) {
    uint32_t now = k_uptime_get_32();
    last_life_ms = last_save_ms = now;
    atomic_set(&last_press_ms, (atomic_val_t)now);
    k_work_reschedule(&life_work, K_MSEC(TICK_HIDDEN_MS));
    return 0;
}

SYS_INIT(pet_mode_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
