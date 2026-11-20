/*
 * Typing pet persistence, in Zephyr settings under "pet/":
 *   pet/state  the pet_pack() blob (keys, growth, fullness, last stage shown)
 *   pet/shown  1 byte: whether the pet or the WPM panel is showing
 * Written from the system work queue (never the display thread). pet_mode
 * limits state saves to one every 15 minutes, or a stage-up.
 */
#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>

#include "pet_mode.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct k_spinlock blob_lock;
static uint8_t blob[PET_BLOB_LEN];
static atomic_t state_pending;
static atomic_t shown_pending = ATOMIC_INIT(-1); /* -1 = nothing to save */

static void save_work_cb(struct k_work *work) {
    if (atomic_cas(&state_pending, 1, 0)) {
        uint8_t copy[PET_BLOB_LEN];
        k_spinlock_key_t key = k_spin_lock(&blob_lock);
        memcpy(copy, blob, sizeof(copy));
        k_spin_unlock(&blob_lock, key);
        int err = settings_save_one("pet/state", copy, sizeof(copy));
        if (err)
            LOG_ERR("pet: failed to save state (%d)", err);
    }
    atomic_val_t s = atomic_set(&shown_pending, -1);
    if (s >= 0) {
        uint8_t v = (uint8_t)s;
        int err = settings_save_one("pet/shown", &v, sizeof(v));
        if (err)
            LOG_ERR("pet: failed to save shown (%d)", err);
    }
}

static K_WORK_DEFINE(save_work, save_work_cb);

void pet_store_save(const struct pet_state *p) {
    k_spinlock_key_t key = k_spin_lock(&blob_lock);
    pet_pack(p, blob);
    k_spin_unlock(&blob_lock, key);
    atomic_set(&state_pending, 1);
    k_work_submit(&save_work);
}

void pet_store_save_shown(bool shown) {
    atomic_set(&shown_pending, shown ? 1 : 0);
    k_work_submit(&save_work);
}

static int pet_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    if (settings_name_steq(name, "state", &next) && !next) {
        uint8_t buf[PET_BLOB_LEN];
        struct pet_state p;
        if (len != sizeof(buf))
            return -EINVAL;
        int rc = read_cb(cb_arg, buf, sizeof(buf));
        if (rc < 0)
            return rc;
        if (!pet_unpack(buf, (size_t)rc, &p)) {
            LOG_WRN("pet: ignoring saved state (unknown format)");
            return 0;
        }
        pet_mode_load_state(&p);
        return 0;
    }
    if (settings_name_steq(name, "shown", &next) && !next) {
        uint8_t v;
        if (len != sizeof(v))
            return -EINVAL;
        int rc = read_cb(cb_arg, &v, sizeof(v));
        if (rc < 0)
            return rc;
        pet_mode_load_shown(v != 0);
        return 0;
    }
    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(pet, "pet", NULL, pet_settings_set, NULL, NULL);
