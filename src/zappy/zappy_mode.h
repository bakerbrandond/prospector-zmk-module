#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Behavior context (keymap / system work queue). layer is a keymap layer id,
 * passed the same way &tog / &to take theirs. */
void zappy_mode_toggle(uint8_t layer);
void zappy_mode_flap(void);
bool zappy_mode_active(void);

uint16_t zappy_mode_best(void);
void zappy_mode_set_best(uint16_t best); /* settings load at boot */

/* Persist a new high score. Weak no-op here; zappy_store.c provides it. */
void zappy_store_save(uint16_t best);
