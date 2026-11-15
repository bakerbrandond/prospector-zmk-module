/*
 * Words-per-minute panel: the current WPM as a large number, plus a filled
 * history graph of the last HISTORY_POINTS seconds.
 *
 * ZMK computes WPM on the central (CONFIG_ZMK_WPM) as the average over the
 * current burst of typing, updated every second and reset to 0 after 5 s
 * idle. An LVGL timer samples zmk_wpm_get_state() once per second, so the
 * graph advances steadily even while the value doesn't change (ZMK only
 * raises an event on change). The timer runs in the display thread.
 *
 * The filled area is a bar chart with no gaps; a line chart with the same
 * data and range is stacked on top for the edge.
 */

#include "wpm_panel.h"

#include <zmk/wpm.h>

#include <fonts.h>
#include <theme.h>

#define HISTORY_POINTS 60
#define SAMPLE_MS 1000
#define MIN_RANGE 60   // never scale below 0-60 WPM, so slow typing looks slow
#define RANGE_STEP 20  // round the top of the scale up to a multiple of this

// 2 px per sample: LVGL spreads bar x positions over (width - bar width), so
// an exact multiple packs the bars edge to edge with no stripes.
#define GRAPH_WIDTH (2 * HISTORY_POINTS)
#define GRAPH_HEIGHT 52

static lv_obj_t *number;
static lv_obj_t *fill_chart;
static lv_obj_t *line_chart;
static lv_chart_series_t *fill_series;
static lv_chart_series_t *line_series;
static uint8_t history[HISTORY_POINTS];
static int history_head;
static int last_wpm = -1;

static lv_obj_t *make_chart(lv_obj_t *parent, lv_chart_type_t type) {
    lv_obj_t *chart = lv_chart_create(parent);
    lv_obj_remove_style_all(chart); // drop theme border/background/padding
    lv_chart_set_type(chart, type);
    lv_chart_set_point_count(chart, HISTORY_POINTS);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(chart, 0, 0);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, MIN_RANGE);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR); // no point markers
    return chart;
}

static int scale_top(void) {
    int top = MIN_RANGE;
    for (int i = 0; i < HISTORY_POINTS; i++) {
        if (history[i] > top) {
            top = history[i];
        }
    }
    return ((top + RANGE_STEP - 1) / RANGE_STEP) * RANGE_STEP;
}

static void sample(lv_timer_t *timer) {
    int wpm = zmk_wpm_get_state();
    if (wpm < 0 || wpm > 250) {
        wpm = 0; // ZMK reports 255 until the first calculation
    }

    history[history_head] = wpm;
    history_head = (history_head + 1) % HISTORY_POINTS;

    int top = scale_top();
    lv_chart_set_range(fill_chart, LV_CHART_AXIS_PRIMARY_Y, 0, top);
    lv_chart_set_range(line_chart, LV_CHART_AXIS_PRIMARY_Y, 0, top);
    lv_chart_set_next_value(fill_chart, fill_series, wpm);
    lv_chart_set_next_value(line_chart, line_series, wpm);

    if (wpm != last_wpm) {
        lv_label_set_text_fmt(number, "%d", wpm);
        lv_obj_set_style_text_color(number, lv_color_hex(wpm > 0 ? THEME_WPM_NUMBER : THEME_WPM_IDLE),
                                    LV_PART_MAIN);
        last_wpm = wpm;
    }
}

int zmk_widget_wpm_panel_init(struct zmk_widget_wpm_panel *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    // Opaque so it covers the layer roller's next-layer row underneath
    lv_obj_set_style_bg_color(widget->obj, lv_color_hex(THEME_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_side(widget->obj, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(widget->obj, lv_color_hex(THEME_WPM_DIVIDER), LV_PART_MAIN);
    lv_obj_set_style_pad_top(widget->obj, 6, LV_PART_MAIN);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);

    number = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(number, &THEME_FONT_WPM, LV_PART_MAIN);
    lv_obj_set_style_text_color(number, lv_color_hex(THEME_WPM_IDLE), LV_PART_MAIN);
    lv_label_set_text(number, "0");
    lv_obj_align(number, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *label = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(label, &THEME_FONT_WPM_LABEL, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_WPM_LABEL), LV_PART_MAIN);
    lv_label_set_text(label, "wpm");
    lv_obj_align_to(label, number, LV_ALIGN_OUT_BOTTOM_LEFT, 2, -4);

    // Graph on the right; the number gets the remaining width (room for 3 digits)
    fill_chart = make_chart(widget->obj, LV_CHART_TYPE_BAR);
    lv_obj_set_style_pad_column(fill_chart, 0, LV_PART_MAIN);  // no gap between bars
    lv_obj_set_style_pad_column(fill_chart, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(fill_chart, 0, LV_PART_ITEMS);
    fill_series = lv_chart_add_series(fill_chart, lv_color_hex(THEME_WPM_FILL),
                                      LV_CHART_AXIS_PRIMARY_Y);

    line_chart = make_chart(widget->obj, LV_CHART_TYPE_LINE);
    lv_obj_set_style_line_width(line_chart, 2, LV_PART_ITEMS);
    line_series = lv_chart_add_series(line_chart, lv_color_hex(THEME_WPM_LINE),
                                      LV_CHART_AXIS_PRIMARY_Y);

    lv_obj_t *charts[] = {fill_chart, line_chart};
    for (int i = 0; i < 2; i++) {
        lv_obj_set_size(charts[i], GRAPH_WIDTH, GRAPH_HEIGHT);
        lv_obj_align(charts[i], LV_ALIGN_TOP_RIGHT, 0, 0);
    }

    lv_chart_set_all_value(fill_chart, fill_series, 0);
    lv_chart_set_all_value(line_chart, line_series, 0);

    lv_timer_create(sample, SAMPLE_MS, NULL);

    return 0;
}

lv_obj_t *zmk_widget_wpm_panel_obj(struct zmk_widget_wpm_panel *widget) { return widget->obj; }
