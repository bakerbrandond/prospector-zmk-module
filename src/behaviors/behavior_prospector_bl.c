/*
 * &pbl: step the Prospector display backlight to its next level and save it
 * (see boards/shields/prospector_adapter/src/brightness.c). In ambient-light
 * sensor mode there is no fixed level, so the weak no-op below swallows the
 * press instead.
 */
#define DT_DRV_COMPAT zmk_behavior_prospector_backlight

#include <zephyr/device.h>
#include <zephyr/toolchain.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

__weak void prospector_backlight_cycle(void) {}

static int pbl_pressed(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    prospector_backlight_cycle();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int pbl_released(struct zmk_behavior_binding *binding,
                        struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api pbl_api = {
    .binding_pressed = pbl_pressed,
    .binding_released = pbl_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &pbl_api);

#endif
