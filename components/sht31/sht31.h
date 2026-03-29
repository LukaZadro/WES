/**
 * @file sht31.h
 *
 * @brief SHT31 temperature & humidity sensor driver over I2C.
 *
 * Uses I2C_NUM_0 at 400 kHz. Pins are configured via Kconfig
 * (SHT31_I2C_SDA / SHT31_I2C_SCL, defaults GPIO21 / GPIO22).
 *
 * Wiring (BLDK peripheral module, U6 SHT31-DIS-F2.5KS):
 *   SDA   I2C_SDA_SW  (Kconfig: CONFIG_SHT31_I2C_SDA)
 *   SCL   I2C_SCL_SW  (Kconfig: CONFIG_SHT31_I2C_SCL)
 *   VDD   3.3 V
 *   ADDR  GND  → I2C address 0x44
 */

#ifndef SHT31_H
#define SHT31_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/** Measured temperature (°C) and relative humidity (%RH). */
typedef struct {
    float temperature_c;
    float humidity_pct;
} sht31_data_t;

/**
 * @brief Initialise the I2C master and verify the sensor responds.
 *
 * @return ESP_OK on success.
 */
esp_err_t sht31_init(void);

/**
 * @brief Trigger a single-shot measurement and return the result.
 *
 * Blocks for ~15 ms while the sensor acquires data.
 * Validates CRC on both temperature and humidity words.
 *
 * @param[out] data  Pointer to caller-allocated result struct.
 * @return ESP_OK on success, ESP_ERR_INVALID_CRC on checksum failure.
 */
esp_err_t sht31_read(sht31_data_t *data);

/**
 * @brief Release the I2C driver.
 */
void sht31_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SHT31_H */
