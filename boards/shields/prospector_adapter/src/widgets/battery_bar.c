#include "battery_bar.h"

#include <string.h>

#include <zmk/display.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_central_status_changed.h>
#if IS_ENABLED(CONFIG_PROSPECTOR_CHARGING_INDICATOR)
#include <zmk/events/peripheral_charging_changed.h>
#endif
#include <zmk/event_manager.h>
#include <zmk/split/central.h>

#include <fonts.h>
#include <theme.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);


static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/*
 * Per-peripheral state, updated from events at any time (even before the
 * display exists) and redrawn in full on the display work queue.
 *
 * ZMK_DISPLAY_WIDGET_LISTENER keeps a single "latest state" per listener and
 * drops events raised before the display is initialized. With two halves
 * connecting at nearly the same moment (e.g. right after the dongle boots),
 * one half's connect event was overwritten before the redraw ran, so its
 * battery never appeared until it reconnected. Keeping a slot table avoids
 * losing either event.
 */
struct peripheral_state {
    bool connected;
    bool has_level;
    uint8_t level;
    bool charging; // on USB power: show a bolt instead of the (inflated) percentage
};

// What is currently drawn, so redraws only animate real changes
struct drawn_state {
    bool valid;
    bool connected;
    int level;
    bool charging;
};

K_MUTEX_DEFINE(state_mutex);
static struct peripheral_state peripherals[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];
static struct drawn_state drawn[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];

static void draw_level(lv_obj_t *info_container, uint8_t level, bool charging) {
    lv_obj_t *bar = lv_obj_get_child(info_container, 0);
    lv_obj_t *num = lv_obj_get_child(info_container, 1);

    lv_bar_set_value(bar, level, LV_ANIM_ON);
    if (charging) {
        lv_label_set_text(num, ICON_CHARGING);
    } else {
        lv_label_set_text_fmt(num, "%d", level);
    }
    lv_obj_set_style_text_color(
        num, lv_color_hex(charging ? THEME_BATTERY_CHARGING : THEME_BATTERY_TEXT), 0);

    // Bar coloured by charge (see theme.h); the number stays snow
    lv_obj_set_style_bg_color(bar, lv_color_hex(theme_battery_color(level)), LV_PART_INDICATOR);
}

static void draw_connected(lv_obj_t *info_container, bool connected) {
    lv_obj_t *bar = lv_obj_get_child(info_container, 0);
    lv_obj_t *num = lv_obj_get_child(info_container, 1);
    lv_obj_t *nc_bar = lv_obj_get_child(info_container, 2);
    lv_obj_t *nc_num = lv_obj_get_child(info_container, 3);

    if (connected) {
        lv_obj_fade_out(nc_bar, 150, 0);
        lv_obj_fade_out(nc_num, 150, 0);
        lv_obj_fade_in(bar, 150, 250);
        lv_obj_fade_in(num, 150, 250);
    } else {
        lv_obj_fade_out(bar, 150, 0);
        lv_obj_fade_out(num, 150, 0);
        lv_obj_fade_in(nc_bar, 150, 250);
        lv_obj_fade_in(nc_num, 150, 250);
    }
}

static void redraw_work_cb(struct k_work *work) {
    struct peripheral_state snapshot[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];

    k_mutex_lock(&state_mutex, K_FOREVER);
    memcpy(snapshot, peripherals, sizeof(snapshot));
    k_mutex_unlock(&state_mutex);

    struct zmk_widget_battery_bar *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
            lv_obj_t *info_container = lv_obj_get_child(widget->obj, i);
            if (info_container == NULL) {
                continue;
            }

            if (snapshot[i].has_level &&
                (!drawn[i].valid || drawn[i].level != snapshot[i].level ||
                 drawn[i].charging != snapshot[i].charging)) {
                draw_level(info_container, snapshot[i].level, snapshot[i].charging);
                drawn[i].level = snapshot[i].level;
                drawn[i].charging = snapshot[i].charging;
            }

            if (!drawn[i].valid || drawn[i].connected != snapshot[i].connected) {
                LOG_DBG("Peripheral %d %s", i, snapshot[i].connected ? "connected" : "disconnected");
                draw_connected(info_container, snapshot[i].connected);
                drawn[i].connected = snapshot[i].connected;
            }

            drawn[i].valid = true;
        }
    }
}

static K_WORK_DEFINE(redraw_work, redraw_work_cb);

static void schedule_redraw(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &redraw_work);
    }
}

static int battery_bar_event_listener(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *bat_ev =
        as_zmk_peripheral_battery_state_changed(eh);
    const struct zmk_split_central_status_changed *conn_ev =
        as_zmk_split_central_status_changed(eh);

    k_mutex_lock(&state_mutex, K_FOREVER);
    if (bat_ev != NULL && bat_ev->source < ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        LOG_DBG("Battery event: source=%d, level=%d", bat_ev->source, bat_ev->state_of_charge);
        peripherals[bat_ev->source].level = bat_ev->state_of_charge;
        peripherals[bat_ev->source].has_level = true;
    } else if (conn_ev != NULL && conn_ev->slot < ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        LOG_DBG("Connection event: slot=%d, connected=%d", conn_ev->slot, conn_ev->connected);
        peripherals[conn_ev->slot].connected = conn_ev->connected;
    }
#if IS_ENABLED(CONFIG_PROSPECTOR_CHARGING_INDICATOR)
    const struct zmk_peripheral_charging_changed *chg_ev = as_zmk_peripheral_charging_changed(eh);
    if (chg_ev != NULL && chg_ev->source < ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        LOG_DBG("Charging event: source=%d, charging=%d", chg_ev->source, chg_ev->charging);
        peripherals[chg_ev->source].charging = chg_ev->charging;
    }
#endif
    k_mutex_unlock(&state_mutex);

    schedule_redraw();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_battery_bar, battery_bar_event_listener);
ZMK_SUBSCRIPTION(widget_battery_bar, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(widget_battery_bar, zmk_split_central_status_changed);
#if IS_ENABLED(CONFIG_PROSPECTOR_CHARGING_INDICATOR)
ZMK_SUBSCRIPTION(widget_battery_bar, zmk_peripheral_charging_changed);
#endif

// Fill in battery levels ZMK already knows about but we have no event for
static void seed_battery_levels(void) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
    k_mutex_lock(&state_mutex, K_FOREVER);
    for (uint8_t i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        uint8_t level;
        if (!peripherals[i].has_level &&
            zmk_split_central_get_peripheral_battery_level(i, &level) == 0 && level > 0) {
            peripherals[i].level = level;
            peripherals[i].has_level = true;
        }
    }
    k_mutex_unlock(&state_mutex);
#endif
}

// The battery number's font, falling back to the icon font for the charging bolt
static lv_font_t battery_font;

int zmk_widget_battery_bar_init(struct zmk_widget_battery_bar *widget, lv_obj_t *parent) {
    battery_font = THEME_FONT_BATTERY;
    battery_font.fallback = &CaskaydiaCove_Icons_20;

    widget->obj = lv_obj_create(parent);
    lv_obj_set_width(widget->obj, lv_pct(100));
    lv_obj_set_flex_flow(widget->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->obj,
                          LV_FLEX_ALIGN_SPACE_EVENLY, // Main axis (horizontal)
                          LV_FLEX_ALIGN_CENTER,       // Cross axis (vertical)
                          LV_FLEX_ALIGN_CENTER        // Track alignment
    );
    lv_obj_set_style_pad_column(widget->obj, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(widget->obj, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(widget->obj, 16, LV_PART_MAIN);

    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        lv_obj_t *info_container = lv_obj_create(widget->obj);
        lv_obj_center(info_container);
        lv_obj_set_height(info_container, lv_pct(100));
        lv_obj_set_flex_grow(info_container, 1);

        lv_obj_t *bar = lv_bar_create(info_container);
        lv_obj_set_size(bar, lv_pct(100), 4);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(THEME_BATTERY_TRACK), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, lv_color_hex(theme_battery_color(0)), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 1, LV_PART_INDICATOR);
        lv_obj_set_style_anim_time(bar, 250, 0);

        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_opa(bar, 0, 0); // hidden until connected

        lv_obj_t *num = lv_label_create(info_container);
        lv_obj_set_style_text_font(num, &battery_font, 0);
        lv_obj_set_style_text_color(num, lv_color_hex(THEME_BATTERY_TEXT), 0);
        lv_obj_align(num, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(num, "N/A");
        lv_obj_set_style_opa(num, 0, 0); // hidden until connected

        lv_obj_t *nc_bar = lv_obj_create(info_container);
        lv_obj_set_size(nc_bar, lv_pct(100), 4);
        lv_obj_align(nc_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(nc_bar, lv_color_hex(THEME_DISCONNECTED), LV_PART_MAIN);
        lv_obj_set_style_radius(nc_bar, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(nc_bar, 255, 0);

        lv_obj_t *nc_num = lv_label_create(info_container);
        lv_obj_set_style_text_font(nc_num, &CaskaydiaCove_Icons_20, 0);
        lv_obj_set_style_text_color(nc_num, lv_color_hex(THEME_DISCONNECTED), 0);
        lv_obj_align(nc_num, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(nc_num, ICON_CLOSE);
        lv_obj_set_style_opa(nc_num, 255, 0);
    }

    sys_slist_append(&widgets, &widget->node);

    // Draw whatever arrived before the screen existed. This runs on the
    // display work queue, so call the redraw directly.
    seed_battery_levels();
    redraw_work_cb(NULL);

    return 0;
}

lv_obj_t *zmk_widget_battery_bar_obj(struct zmk_widget_battery_bar *widget) { return widget->obj; }
