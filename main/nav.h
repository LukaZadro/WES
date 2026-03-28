#pragma once

#include "lvgl.h"

/**
 * @brief Initialise the keypad input device and per-screen focus groups.
 *        Must be called from the LVGL GUI task after ui_init() has run.
 */
void nav_init(void);

/**
 * @brief Update the current key state.
 *        Thread-safe: may be called from any FreeRTOS task.
 *
 * @param key   One of LV_KEY_* constants.
 * @param state LV_INDEV_STATE_PR or LV_INDEV_STATE_REL.
 */
void nav_send_key(uint32_t key, lv_indev_state_t state);

/**
 * @brief Navigate back to the previous screen (schedules lv_async_call).
 *        Thread-safe: may be called from any FreeRTOS task.
 */
void nav_go_back(void);

/**
 * @brief Move focus to the nearest widget in the given direction using
 *        absolute screen coordinates (spatial navigation).
 *        Pass LV_KEY_RIGHT, LV_KEY_LEFT, LV_KEY_UP, or LV_KEY_DOWN.
 *        Thread-safe: may be called from any FreeRTOS task.
 */
void nav_move_dir(uint32_t dir);
