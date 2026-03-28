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
