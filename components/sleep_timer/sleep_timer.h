/**
 * @file sleep_timer.h
 *
 * @brief Inactivity-based light sleep timer.
 *
 * After SLEEP_TIMEOUT_S seconds with no input (button, joystick, touch) the
 * device enters ESP32 light sleep. All FreeRTOS tasks are suspended and the
 * display holds its last frame. On wakeup every task resumes where it left off.
 *
 * Wakeup sources:
 *   GPIO32 / BTN2   HIGH level  (gpio_wakeup)
 *   GPIO33 / BTN3   HIGH level  (gpio_wakeup)
 *   GPIO25 / BTN4   HIGH level  (gpio_wakeup)
 *   GPIO15 / Touch  LOW  level  (gpio_wakeup, XPT2046 IRQ active-low)
 *   GPIO39 / ACC    HIGH level  (ext1 — RTC GPIO, threshold ~1.5 g)
 */

#ifndef __SLEEP_TIMER_H__
#define __SLEEP_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "lvgl.h"

#define SLEEP_TIMEOUT_S  (15U)

typedef void (*sleep_cb_t)(void);

/**
 * @brief Initialise the inactivity timer and sleep task.
 *
 * @param on_sleep  Called just before entering light sleep. NULL = no-op.
 * @param on_wake   Called immediately after wakeup.        NULL = no-op.
 */
esp_err_t sleep_timer_init(sleep_cb_t on_sleep, sleep_cb_t on_wake);

/**
 * @brief Restart the inactivity countdown. Call from task context only.
 */
void sleep_timer_reset(void);

/**
 * @brief LVGL indev feedback callback — assign to indev_drv.feedback_cb.
 */
void sleep_timer_lv_feedback(lv_indev_drv_t *drv, uint8_t code);

#ifdef __cplusplus
}
#endif

#endif /* __SLEEP_TIMER_H__ */
