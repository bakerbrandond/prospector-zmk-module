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

static atomic_t to_save;

static void save_work_cb(struct k_work *work) {
    uint16_t best = (uint16_t)atomic_get(&to_save);
    int err = settings_save_one("zappy/hi", &best, sizeof(best));
    if (err)
        LOG_ERR("zappy: failed to save high score (%d)", err);
}

static K_WORK_DEFINE(save_work, save_work_cb);

void zappy_store_save(uint16_t best) {
    atomic_set(&to_save, best);
    k_work_submit(&save_work);
}

static int zappy_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                              void *cb_arg) {
    const char *next;
    if (settings_name_steq(name, "hi", &next) && !next) {
        uint16_t best;
        if (len != sizeof(best))
            return -EINVAL;
        int rc = read_cb(cb_arg, &best, sizeof(best));
        if (rc < 0)
            return rc;
        zappy_mode_set_best(best);
        return 0;
    }
    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(zappy, "zappy", NULL, zappy_settings_set, NULL, NULL);
