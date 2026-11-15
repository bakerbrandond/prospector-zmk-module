#include "layer_roller.h"

#include <ctype.h>
#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>

#include <fonts.h>
#include <theme.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static char layer_names_buffer[256] = {0}; // Buffer for concatenated layer names

// Trailing layers left out of the roller (e.g. the Zappy Birds game layer)
#define VISIBLE_LAYERS (ZMK_KEYMAP_LAYERS_LEN - CONFIG_PROSPECTOR_ROLLER_HIDDEN_TRAILING_LAYERS)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_roller_state {
    uint8_t index;
};

static void layer_roller_set_sel(lv_obj_t *roller, struct layer_roller_state state) {
    lv_roller_set_selected(roller, state.index, LV_ANIM_ON);
}

static void layer_roller_update_cb(struct layer_roller_state state) {
    struct zmk_widget_layer_roller *widget;
    if (state.index >= VISIBLE_LAYERS) {
        return; // hidden layer: keep showing the current selection
    }
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        layer_roller_set_sel(widget->obj, state);
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
    char *ptr = layer_names_buffer;

    for (int i = 0; i < VISIBLE_LAYERS; i++) {
        const char *layer_name = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(i));
        if (layer_name) {
            if (i > 0) {
                strcat(ptr, "\n");
                ptr += strlen(ptr);
            }

            if (layer_name && *layer_name) {
#if IS_ENABLED(CONFIG_LAYER_ROLLER_ALL_CAPS)
                while (*layer_name) {
                    *ptr = toupper((unsigned char)*layer_name);
                    ptr++;
                    layer_name++;
                }
                *ptr = '\0';
#else
                strcat(ptr, layer_name);
                ptr += strlen(layer_name);
#endif
            } else {
                // Just use the number for unnamed layers
                char index_str[12];
                snprintf(index_str, sizeof(index_str), "%d", i);
                strcat(ptr, index_str);
                ptr += strlen(index_str);
            }
        }
    }

    lv_roller_set_options(widget->obj, layer_names_buffer, LV_ROLLER_MODE_INFINITE);

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