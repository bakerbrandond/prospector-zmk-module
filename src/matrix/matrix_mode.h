#pragma once

/* Display work queue only: stop the rain now and restore the screen under
 * it. Zappy Birds calls this before taking over the screen. */
void matrix_mode_hide_now(void);
