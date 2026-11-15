/*
 * Zappy Birds high score, kept in Zephyr settings under "zappy/hi".
 * Saved only when beaten, from the system work queue (never the display
 * thread), so flash writes stay rare and off the render path.
 */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>

#include "zappy_mode.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* latest value per mode; -1 = nothing to save */
static atomic_t to_save[2] = {ATOMIC_INIT(-1), ATOMIC_INIT(-1)};
static const char *const keys[2] = {"zappy/hi", "zappy/hh"};

static void save_work_cb(struct k_work *work) {
    for (int m = 0; m < 2; m++) {
        atomic_val_t v = atomic_set(&to_save[m], -1);
        if (v < 0)
            continue;
        uint16_t best = (uint16_t)v;
        int err = settings_save_one(keys[m], &best, sizeof(best));
        if (err)
            LOG_ERR("zappy: failed to save %s (%d)", keys[m], err);
    }
}

static K_WORK_DEFINE(save_work, save_work_cb);

void zappy_store_save(enum zappy_mode m, uint16_t best) {
    atomic_set(&to_save[m == ZAPPY_HARD], best);
    k_work_submit(&save_work);
}

static int zappy_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                              void *cb_arg) {
    const char *next;
    bool hi = settings_name_steq(name, "hi", &next) && !next;
    bool hh = !hi && settings_name_steq(name, "hh", &next) && !next;
    if (hi || hh) {
        uint16_t best;
        if (len != sizeof(best))
            return -EINVAL;
        int rc = read_cb(cb_arg, &best, sizeof(best));
        if (rc < 0)
            return rc;
        zappy_mode_set_best(hh ? ZAPPY_HARD : ZAPPY_NORMAL, best);
        return 0;
    }
    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(zappy, "zappy", NULL, zappy_settings_set, NULL, NULL);
