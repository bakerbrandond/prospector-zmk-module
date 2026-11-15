/*
 * Zappy Birds behaviors:
 *   &zappy LAYER  toggle game mode (activates/deactivates LAYER)
 *   &zflap        flap; sends nothing to the host
 */
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zephyr/toolchain.h>

#include "../zappy/zappy_mode.h"

static __maybe_unused int zappy_released(struct zmk_behavior_binding *binding,
                          struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

#define DT_DRV_COMPAT zmk_behavior_zappy_toggle
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int zappy_toggle_pressed(struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event) {
    zappy_mode_toggle((uint8_t)binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api zappy_toggle_api = {
    .binding_pressed = zappy_toggle_pressed,
    .binding_released = zappy_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &zappy_toggle_api);

#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT zmk_behavior_zappy_flap
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int zappy_flap_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    zappy_mode_flap();
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api zappy_flap_api = {
    .binding_pressed = zappy_flap_pressed,
    .binding_released = zappy_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &zappy_flap_api);

#endif
#undef DT_DRV_COMPAT
