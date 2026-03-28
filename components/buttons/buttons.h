/**
 * @file buttons.h
 *
 * @brief Driver for the 4 user push-buttons on the BLDK peripheral module.
 *
 * Hardware notes (from schematics):
 *  - 4 buttons are active-HIGH with 10 k pull-DOWN resistors.
 *  - Net labels on the ESP32 core module:
 *      BTN1 -> GPIO36  (input-only, no interrupt support)
 *      BTN2 -> GPIO32
 *      BTN3 -> GPIO33
 *      BTN4 -> GPIO25
 *  - The corresponding DIP switches (S2) must be in the ON position.
 *
 * COPYRIGHT NOTICE: (c) 2025 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef __BUTTON_H__
#define __BUTTON_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include <stdbool.h>
#include "esp_err.h"

//---------------------------------- MACROS -----------------------------------
#define BUTTON_GPIO_BTN1    (36)
#define BUTTON_GPIO_BTN2    (32)
#define BUTTON_GPIO_BTN3    (33)
#define BUTTON_GPIO_BTN4    (25)

//-------------------------------- DATA TYPES ---------------------------------

/** @brief Logical button identifiers. */
typedef enum
{
    BUTTON_1 = 0,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_COUNT
} button_id_t;

/**
 * @brief Callback invoked when a button event fires.
 *
 * @param[in] id      Button that triggered the event.
 * @param[in] pressed true = pressed (rising edge), false = released (falling edge).
 */
typedef void (*button_event_cb_t)(button_id_t id, bool pressed);

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initialise all four button GPIOs.
 *
 * BTN2/BTN3/BTN4 use interrupts. BTN1 (GPIO36) is polled by an internal task.
 *
 * @param[in] cb  Event callback. Pass NULL to poll with button_read().
 * @return ESP_OK on success, or an esp_err_t error code.
 */
esp_err_t button_init(button_event_cb_t cb);

/**
 * @brief Read the current state of a single button.
 *
 * @param[in]  id       Button to query.
 * @param[out] pressed  Set to true if the button is currently pressed.
 * @return ESP_OK on success.
 */
esp_err_t button_read(button_id_t id, bool *pressed);

/**
 * @brief De-initialise all button GPIOs and detach ISR handlers.
 */
void button_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_H__ */