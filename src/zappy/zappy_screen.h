#pragma once

#include <lvgl.h>

#include "zappy_core.h"

/* Both must be called on the display work queue. */
lv_obj_t *zappy_screen_create(void);                  /* once, lazily; returns the screen */
void zappy_screen_render(const struct zappy_game *g); /* every tick */
