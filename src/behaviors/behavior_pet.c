/*
 * Typing pet behaviors:
 *   &pet CMD LAYER  0 swap the WPM panel and the pet, 1 feed, 2 mini-game
 *                   (LAYER: the mini-game's capture layer, used by 2)
 *   &petkey         a mini-game guess (left or right half); sends nothing
 *
 * Without CONFIG_PROSPECTOR_TYPING_PET both are no-ops that still swallow
 * the key press.
 */
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zephyr/toolchain.h>

#include "../pet/pet_mode.h"

static __maybe_unused int pet_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

#define DT_DRV_COMPAT zmk_behavior_pet
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int pet_pressed(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    if (IS_ENABLED(CONFIG_PROSPECTOR_TYPING_PET) && binding->param1 <= PET_CMD_PLAY)
        pet_mode_command((enum pet_cmd)binding->param1, (uint8_t)binding->param2);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api pet_api = {
    .binding_pressed = pet_pressed,
    .binding_released = pet_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &pet_api);

#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT zmk_behavior_pet_key
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int pet_key_pressed(struct zmk_behavior_binding *binding,
                           struct zmk_behavior_binding_event event) {
    if (IS_ENABLED(CONFIG_PROSPECTOR_TYPING_PET))
        pet_mode_guess((uint8_t)event.position);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api pet_key_api = {
    .binding_pressed = pet_key_pressed,
    .binding_released = pet_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &pet_key_api);

#endif
#undef DT_DRV_COMPAT
