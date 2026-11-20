/*
 * Typing pet controller. pet_mode_command()/pet_mode_guess() run in behavior
 * context; the life tick runs on the system work queue; the swap and the
 * panel timer run on the display work queue.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "pet_core.h"

enum pet_cmd { PET_CMD_TOGGLE = 0, PET_CMD_FEED = 1, PET_CMD_PLAY = 2 };

void pet_mode_command(enum pet_cmd cmd, uint8_t capture_layer);
void pet_mode_guess(uint8_t position); /* &petkey press */
bool pet_mode_shown(void);             /* pet showing instead of the WPM panel */
/* End a running mini-game and release its layer (Zappy and the matrix rain
 * call this when they take over the screen; weak no-op without the pet). */
void pet_mode_end_game(void);

/* Settings, at boot */
void pet_mode_load_state(const struct pet_state *saved);
void pet_mode_load_shown(bool shown);

/* Persistence. Weak no-ops here; pet_store.c provides them. */
void pet_store_save(const struct pet_state *p);
void pet_store_save_shown(bool shown);
