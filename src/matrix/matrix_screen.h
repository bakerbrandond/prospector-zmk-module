#pragma once

#include <lvgl.h>

#include "matrix_core.h"

/* Display work queue only. */
lv_obj_t *matrix_screen_create(void); /* once, lazily; returns the screen */
/* Point the renderer at the model and invalidate one span per column
 * covering the rows set in dirty[c]. */
void matrix_screen_render(const struct matrix_state *m, const uint16_t *dirty);
