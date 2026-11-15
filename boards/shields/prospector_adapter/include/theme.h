#pragma once

/*
 * Nord palette (https://www.nordtheme.com/docs/colors-and-palettes) and the
 * semantic colors the status screen uses. Change the mapping here to retheme.
 */

// Polar Night
#define NORD0 0x2E3440
#define NORD1 0x3B4252
#define NORD2 0x434C5E
#define NORD3 0x4C566A
// Snow Storm
#define NORD4 0xD8DEE9
#define NORD5 0xE5E9F0
#define NORD6 0xECEFF4
// Frost
#define NORD7 0x8FBCBB
#define NORD8 0x88C0D0
#define NORD9 0x81A1C1
#define NORD10 0x5E81AC
// Aurora
#define NORD11 0xBF616A
#define NORD12 0xD08770
#define NORD13 0xEBCB8B
#define NORD14 0xA3BE8C
#define NORD15 0xB48EAD

#define THEME_BG NORD0

#define THEME_LAYER_ACTIVE NORD8
#define THEME_LAYER_INACTIVE NORD3

#define THEME_BATTERY_FILL NORD9
#define THEME_BATTERY_TRACK NORD1
#define THEME_BATTERY_TEXT NORD4
#define THEME_BATTERY_LOW NORD12
#define THEME_BATTERY_LOW_TRACK NORD1
#define THEME_DISCONNECTED NORD3

#define THEME_INDICATOR_ON NORD7
#define THEME_INDICATOR_OFF NORD1

/*
 * Fonts: CaskaydiaCove Nerd Font Propo (Cascadia Code), generated with
 * lv_font_conv (ASCII 0x20-0x7E, 4 bpp). See include/fonts.h.
 */
#define THEME_FONT_LAYER_ACTIVE CaskaydiaCove_Regular_48
#define THEME_FONT_LAYER_INACTIVE CaskaydiaCove_ExtraLight_48
#define THEME_FONT_BATTERY CaskaydiaCove_SemiBold_20

// WPM panel
#define THEME_WPM_NUMBER NORD6
#define THEME_WPM_IDLE NORD3
#define THEME_WPM_LABEL NORD4
#define THEME_WPM_LINE NORD8
#define THEME_WPM_FILL NORD1
#define THEME_FONT_WPM CaskaydiaCove_SemiBold_32
#define THEME_FONT_WPM_LABEL CaskaydiaCove_SemiBold_20
