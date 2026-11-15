#include "layer_roller.h"
#include "roller_names.h"

#include <ctype.h>
#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>

#include <fonts.h>
#include <theme.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static char layer_names_buffer[256] = {0}; // Concatenated names of the visible layers

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_roller_state {
    uint8_t index;
};

/*
 * Rebuilds the roller's options from the live keymap (ZMK Studio can rename,
 * add and reorder layers at runtime, and raises no event for it), skipping the
 * layer named CONFIG_PROSPECTOR_ROLLER_HIDDEN_LAYER_NAME (the Zappy Birds game
 * layer). Options are only re-set when the text changed. Returns the visible
 * row of layer index `active`, or -1 if that layer is hidden.
 */
static int layer_roller_refresh(lv_obj_t *roller, int active) {
    static char next[sizeof(layer_names_buffer)];
    const char *names[ZMK_KEYMAP_LAYERS_LEN];
    int n = 0;
    for (; n < ZMK_KEYMAP_LAYERS_LEN; n++) {
        zmk_keymap_layer_id_t id = zmk_keymap_layer_index_to_id(n);
        if (id == ZMK_KEYMAP_LAYER_ID_INVAL) {
            break; // unused (Studio-reserved) layers come last
        }
        names[n] = zmk_keymap_layer_name(id);
    }
    int sel = roller_build(names, n, active, CONFIG_PROSPECTOR_ROLLER_HIDDEN_LAYER_NAME,
                           IS_ENABLED(CONFIG_PROSPECTOR_LAYER_ROLLER_ALL_CAPS), next, sizeof(next));
    if (strcmp(next, layer_names_buffer) != 0) {
        strcpy(layer_names_buffer, next);
        lv_roller_set_options(roller, layer_names_buffer, LV_ROLLER_MODE_INFINITE);
    }
    return sel;
}

static void layer_roller_update_cb(struct layer_roller_state state) {
    struct zmk_widget_layer_roller *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        int sel = layer_roller_refresh(widget->obj, state.index);
        if (sel >= 0) { // hidden layer: keep showing the current selection
            lv_roller_set_selected(widget->obj, sel, LV_ANIM_ON);
        }
    }
}

static struct layer_roller_state layer_roller_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    LOG_INF("Roller set to: %d", index);
    return (struct layer_roller_state){
        .index = index,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_roller, struct layer_roller_state, layer_roller_update_cb,
                            layer_roller_get_state)
ZMK_SUBSCRIPTION(widget_layer_roller, zmk_layer_state_changed);

int zmk_widget_layer_roller_init(struct zmk_widget_layer_roller *widget, lv_obj_t *parent) {
    widget->obj = lv_roller_create(parent);

    layer_names_buffer[0] = '\0';
    layer_roller_refresh(widget->obj, zmk_keymap_highest_layer_active());

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(THEME_BG));
    lv_style_set_text_color(&style, lv_color_hex(THEME_LAYER_ACTIVE));
    // lv_style_set_text_letter_space(&style, 2);
    lv_style_set_border_width(&style, 0);
    lv_style_set_pad_all(&style, 0);
    // lv_obj_add_style(lv_scr_act(), &style, 0);

    lv_obj_add_style(widget->obj, &style, 0);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_SELECTED);
    lv_obj_set_style_text_font(widget->obj, &THEME_FONT_LAYER_ACTIVE, LV_PART_SELECTED);
    lv_obj_set_style_text_color(widget->obj, lv_color_hex(THEME_LAYER_ACTIVE), LV_PART_SELECTED);
    // lv_obj_set_style_text_line_space(widget->obj, 20, LV_PART_SELECTED);
    // lv_obj_set_style_text_line_space(widget->obj, 20, LV_PART_MAIN);
    lv_obj_set_style_text_font(widget->obj, &THEME_FONT_LAYER_INACTIVE, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->obj, lv_color_hex(THEME_LAYER_INACTIVE), LV_PART_MAIN);
    // Only the active layer is shown; the other rows stay but are invisible,
    // so a layer change still slides the new name in
    lv_obj_set_style_text_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);
    // text_opa is inherited: without its own value the selected row would
    // fall back to the main part's and vanish too
    lv_obj_set_style_text_opa(widget->obj, LV_OPA_COVER, LV_PART_SELECTED);
    // lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_CENTER, 0);


    // static lv_style_t style_roller;
    // lv_style_init(&style_roller);
    // lv_style_set_text_letter_space(&style_roller, -0.5);
    // lv_style_set_text_line_space(&style_roller, 20);
    // lv_style_set_text_color(&style_roller, lv_color_hex(0x6b6b6b));
    // lv_style_set_bg_color(&style_roller, lv_color_hex(0x050505));
    // lv_style_set_bg_opa(&style_roller, 255);

    // static lv_style_t style_roller_sel;
    // lv_style_init(&style_roller_sel);
    // lv_style_set_text_letter_space(&style_roller_sel, -0.5);
    // lv_style_set_text_line_space(&style_roller_sel, 16);
    // lv_style_set_text_color(&style_roller_sel, lv_color_hex(0xffffff));
    // lv_style_set_bg_color(&style_roller_sel, lv_color_hex(0x1c1c1c));
    // lv_style_set_bg_opa(&style_roller_sel, 255);

    // lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_CENTER, 0);
    // lv_obj_add_style(widget->obj, &style_roller, LV_PART_MAIN);
    // lv_obj_add_style(widget->obj, &style_roller_sel, LV_PART_SELECTED);
    // lv_obj_set_style_radius(widget->obj, 20, LV_PART_MAIN);

    lv_obj_set_style_anim_time(widget->obj, 100, 0);

    sys_slist_append(&widgets, &widget->node);

    widget_layer_roller_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_roller_obj(struct zmk_widget_layer_roller *widget) {
    return widget->obj;
}