#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_wpm_panel {
    lv_obj_t *obj;
};

int zmk_widget_wpm_panel_init(struct zmk_widget_wpm_panel *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_wpm_panel_obj(struct zmk_widget_wpm_panel *widget);
