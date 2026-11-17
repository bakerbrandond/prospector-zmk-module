/*******************************************************************************
 * Size: 20 px
 * Bpp: 4
 * Opts: --font CaskaydiaCoveNerdFontPropo-Regular.ttf -r 0xF0156,0xF140B --size 20 --bpp 4 --format lvgl --no-compress --lv-include lvgl.h -o CaskaydiaCove_Icons_20.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef CASKAYDIACOVE_ICONS_20
#define CASKAYDIACOVE_ICONS_20 1
#endif

#if CASKAYDIACOVE_ICONS_20

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+F0156 "󰅖" */
    0x26, 0x0, 0x0, 0x0, 0x0, 0x70, 0xbf, 0x60,
    0x0, 0x0, 0xb, 0xf6, 0x1d, 0xf6, 0x0, 0x0,
    0xbf, 0x80, 0x1, 0xdf, 0x60, 0xb, 0xf8, 0x0,
    0x0, 0x1d, 0xf7, 0xbf, 0x80, 0x0, 0x0, 0x1,
    0xdf, 0xf8, 0x0, 0x0, 0x0, 0x0, 0xbf, 0xf7,
    0x0, 0x0, 0x0, 0xb, 0xf9, 0xdf, 0x60, 0x0,
    0x0, 0xbf, 0x80, 0x1d, 0xf6, 0x0, 0xb, 0xf8,
    0x0, 0x1, 0xdf, 0x60, 0xbf, 0x80, 0x0, 0x0,
    0x1d, 0xf5, 0x48, 0x0, 0x0, 0x0, 0x1, 0xa1,

    /* U+F140B "󱐋" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4,
    0x0, 0x0, 0x0, 0x0, 0xc, 0x0, 0x0, 0x0,
    0x0, 0x7d, 0x0, 0x0, 0x0, 0x0, 0xed, 0x0,
    0x0, 0x0, 0x7, 0xfd, 0x0, 0x0, 0x0, 0xe,
    0xfd, 0x0, 0x0, 0x0, 0x7f, 0xfe, 0x77, 0x76,
    0x0, 0xef, 0xff, 0xff, 0xf8, 0x7, 0xff, 0xff,
    0xff, 0xf1, 0xe, 0xff, 0xff, 0xff, 0x80, 0x7f,
    0xff, 0xff, 0xff, 0x10, 0x79, 0x99, 0xef, 0xf8,
    0x0, 0x0, 0x0, 0xdf, 0xf1, 0x0, 0x0, 0x0,
    0xdf, 0x80, 0x0, 0x0, 0x0, 0xdf, 0x10, 0x0,
    0x0, 0x0, 0xd8, 0x0, 0x0, 0x0, 0x0, 0xc1,
    0x0, 0x0, 0x0, 0x0, 0x50, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 186, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 72, .adv_w = 160, .box_w = 10, .box_h = 20, .ofs_x = 0, .ofs_y = -3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x12b5
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 983382, .range_length = 4790, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t CaskaydiaCove_Icons_20 = {
#else
lv_font_t CaskaydiaCove_Icons_20 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if CASKAYDIACOVE_ICONS_20*/

