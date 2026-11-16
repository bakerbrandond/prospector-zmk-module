/*
 * Screen size as LVGL sees it. The ST7789V panel is 240x280 portrait, but
 * display_rotate_init.c turns it 90 or 270 degrees, so LVGL lays out a
 * 280x240 landscape screen. Pure C: shared by the host-tested cores.
 */
#pragma once

#define DISPLAY_W 280
#define DISPLAY_H 240
