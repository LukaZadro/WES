/**
 * @file accelerometer.h
 *
 * @brief LIS3DH 3-axis accelerometer driver over SPI.
 *
 * Shares the VSPI bus (SPI3_HOST) with the ILI9341 display and XPT2046 touch.
 *
 * Wiring:
 *   MISO  GPIO19  (shared)
 *   MOSI  GPIO23  (shared)
 *   CLK   GPIO18  (shared)
 *   CS    GPIO13
 *   IRQ1  GPIO39  (input-only, activity interrupt)
 */

#ifndef __ACCELEROMETER_H__
#define __ACCELEROMETER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdint.h>

#define ACC_SPI_HOST   SPI3_HOST
#define ACC_GPIO_MISO  (19)
#define ACC_GPIO_MOSI  (23)
#define ACC_GPIO_CLK   (18)
#define ACC_GPIO_CS    (13)
#define ACC_GPIO_IRQ1  (39)

/** Acceleration on each axis in milli-g (mg). */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} acc_data_t;

/** Called from ISR when motion above threshold is detected on INT1. */
typedef void (*acc_activity_cb_t)(void);

/**
 * @brief Initialise the LIS3DH driver and start the data-logging task.
 *
 * @param activity_cb  ISR callback on activity detection. NULL to disable.
 * @return ESP_OK on success.
 */
esp_err_t acc_init(acc_activity_cb_t activity_cb);

/**
 * @brief Read the latest X/Y/Z acceleration in mg.
 */
esp_err_t acc_read(acc_data_t *data);

/**
 * @brief Clear the latched INT1 interrupt before entering light sleep.
 *
 * Reads INT1_SRC (de-asserts INT1) twice with a 150 ms gap to flush any
 * pipeline sample, ensuring INT1 is LOW before ext1 wakeup is armed.
 */
esp_err_t acc_prepare_sleep(void);

/**
 * @brief Remove the SPI device and release the IRQ1 GPIO.
 */
void acc_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __ACCELEROMETER_H__ */
