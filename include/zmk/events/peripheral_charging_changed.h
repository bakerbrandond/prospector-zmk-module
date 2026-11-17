#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

// A split peripheral (keyboard half) started or stopped running on USB power
struct zmk_peripheral_charging_changed {
    uint8_t source; // peripheral slot, as in zmk_peripheral_battery_state_changed
    bool charging;
};

ZMK_EVENT_DECLARE(zmk_peripheral_charging_changed);
