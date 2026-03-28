/**
 * @file joystick.h
 *
 * @brief Driver for the analog joystick on the BLDK peripheral module.
 *
 * Hardware notes:
 *  - Joystick X axis -> GPIO34 (ADC1 channel 6)
 *  - Joystick Y axis -> GPIO35 (ADC1 channel 7)
 *  - Output is normalized to -100..+100 on each axis.
 *  - GPIO34 and GPIO35 are input-only pins (no pull resistors needed).
 *
 * COPYRIGHT NOTICE: (c) 2025 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "esp_err.h"
#include <stdint.h>

//---------------------------------- MACROS -----------------------------------
#define JOYSTICK_GPIO_X     (34)
#define JOYSTICK_GPIO_Y     (35)

/** Poll interval in milliseconds. */
#define JOYSTICK_POLL_MS    (50U)

//-------------------------------- DATA TYPES ---------------------------------

/** @brief Joystick position, normalized to -100..+100 per axis. */
typedef struct
{
    int8_t x;  /**< Horizontal axis: -100 (left) .. +100 (right) */
    int8_t y;  /**< Vertical axis:   -100 (down) .. +100 (up)    */
} joystick_pos_t;

/**
 * @brief Callback invoked from the joystick task whenever a new reading
 *        is available.
 *
 * @param[in] pos  Current joystick position.
 */
typedef void (*joystick_event_cb_t)(joystick_pos_t pos);

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initialise the ADC channels and start the polling task.
 *
 * @param[in] cb  Callback invoked every JOYSTICK_POLL_MS with the latest
 *                position. Pass NULL to use joystick_read() instead.
 * @return ESP_OK on success, or an esp_err_t error code.
 */
esp_err_t joystick_init(joystick_event_cb_t cb);

/**
 * @brief Read the current joystick position on demand.
 *
 * Can be called from any task context. Safe to use without a callback.
 *
 * @param[out] pos  Populated with the current position.
 * @return ESP_OK on success.
 */
esp_err_t joystick_read(joystick_pos_t *pos);

/**
 * @brief Stop the polling task and free ADC resources.
 */
void joystick_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __JOYSTICK_H__ */