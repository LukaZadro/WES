#ifndef MEMORY_UI_DRAW_H
#define MEMORY_UI_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "lvgl.h"

void setup_memory_ui(void);
void destroy_memory_ui(void);

/* Joystick navigation — call with LV_KEY_LEFT/RIGHT/UP/DOWN.
 * Returns true if consumed, false if the caller should pass it to nav_move_dir(). */
bool memory_ui_joystick(uint32_t lv_key);

/* Select the currently highlighted card (no-op when grid not active). */
void memory_ui_select(void);

#ifdef __cplusplus
}
#endif

#endif // TETRIS_UI_DRAW_H