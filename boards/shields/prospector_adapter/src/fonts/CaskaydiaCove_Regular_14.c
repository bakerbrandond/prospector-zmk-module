/*******************************************************************************
 * Size: 14 px
 * Bpp: 4
 * Opts: --font CaskaydiaCoveNerdFontPropo-Regular.ttf --symbols  wpm --size 14 --bpp 4 --format lvgl --no-compress --lv-include lvgl.h -o CaskaydiaCove_Regular_14.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef CASKAYDIACOVE_REGULAR_14
#define CASKAYDIACOVE_REGULAR_14 1
#endif

#if CASKAYDIACOVE_REGULAR_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+006D "m" */
    0x3f, 0x9f, 0x7e, 0xe2, 0x3f, 0x6a, 0xd3, 0xf6,
    0x3f, 0x38, 0xb0, 0xf7, 0x3f, 0x38, 0xb0, 0xf7,
    0x3f, 0x38, 0xb0, 0xf7, 0x3f, 0x38, 0xb0, 0xf7,
    0x3f, 0x38, 0xb0, 0xf7, 0x3f, 0x38, 0xb0, 0xf7,

    /* U+0070 "p" */
    0xd8, 0xbf, 0xd4, 0xd, 0xe7, 0x4b, 0xf0, 0xdc,
    0x0, 0x1f, 0x5d, 0x90, 0x0, 0xf7, 0xd9, 0x0,
    0xf, 0x7d, 0xc0, 0x2, 0xf6, 0xde, 0x96, 0xcf,
    0x1d, 0x9a, 0xfd, 0x40, 0xd9, 0x0, 0x0, 0xd,
    0x90, 0x0, 0x0, 0xd9, 0x0, 0x0, 0xd, 0x90,
    0x0, 0x0,

    /* U+0077 "w" */
    0x7e, 0xb, 0xc0, 0xca, 0x5f, 0xc, 0xd0, 0xd8,
    0x4f, 0xd, 0xc0, 0xe6, 0x2f, 0x3b, 0xa2, 0xf4,
    0xf, 0x69, 0x86, 0xf2, 0xf, 0x97, 0x69, 0xf0,
    0xd, 0xd5, 0x4d, 0xe0, 0xc, 0xf3, 0x2f, 0xc0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 131, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 131, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 131, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 74, .adv_w = 131, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x4d, 0x50, 0x57
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 88, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 4, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
const lv_font_t CaskaydiaCove_Regular_14 = {
#else
lv_font_t CaskaydiaCove_Regular_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 12,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if CASKAYDIACOVE_REGULAR_14*/

