#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zappy_core.h"

/* Behavior context (keymap / system work queue). layer is a keymap layer id,
 * passed the same way &tog / &to take theirs. */
void zappy_mode_toggle(uint8_t layer);
void zappy_mode_flap(uint8_t position); /* key position of the press */
bool zappy_mode_active(void);

void zappy_mode_set_best(enum zappy_mode m, uint16_t best); /* settings load at boot */

/* Persist a new high score. Weak no-op here; zappy_store.c provides it. */
void zappy_store_save(enum zappy_mode m, uint16_t best);
