/**
 * @file sht31.c
 *
 * @brief SHT31 temperature & humidity sensor driver (I2C, single-shot mode).
 *
 * Measurement command: 0x2400 (high repeatability, no clock stretching).
 * After issuing the command the driver waits 15 ms before reading 6 bytes:
 *   [0..1] raw temperature  [2] CRC  [3..4] raw humidity  [5] CRC
 *
 * Conversion:
 *   T [°C]  = -45 + 175 × rawT / 65535
 *   RH [%]  = 100 × rawRH / 65535
 *
 * CRC: polynomial 0x31, init 0xFF, no final XOR (CRC-8/NRSC-5).
 */

#include "sht31.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "sht31"

#define SHT31_I2C_PORT      I2C_NUM_0
#define SHT31_I2C_ADDR      0x44
#define SHT31_I2C_FREQ_HZ   400000
#define SHT31_TIMEOUT_MS    50

/* Single-shot, high repeatability, clock-stretching disabled */
#define CMD_MEAS_HIGH_NO_CS_MSB  0x24
#define CMD_MEAS_HIGH_NO_CS_LSB  0x00

/* Soft-reset command */
#define CMD_SOFT_RESET_MSB  0x30
#define CMD_SOFT_RESET_LSB  0xA2

static bool _initialised = false;

/* ---- CRC helper ---------------------------------------------------------- */

static uint8_t _crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* ---- Public API ---------------------------------------------------------- */

esp_err_t sht31_init(void)
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = CONFIG_SHT31_I2C_SDA,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_io_num       = CONFIG_SHT31_I2C_SCL,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = SHT31_I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(SHT31_I2C_PORT, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(SHT31_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        /* ESP_ERR_INVALID_STATE means the driver is already installed */
        ESP_LOGE(TAG, "i2c_driver_install: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Issue a soft reset and give the sensor 2 ms to recover */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHT31_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, CMD_SOFT_RESET_MSB, true);
    i2c_master_write_byte(cmd, CMD_SOFT_RESET_LSB, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(SHT31_I2C_PORT, cmd, pdMS_TO_TICKS(SHT31_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "soft reset failed: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(2));

    _initialised = true;
    ESP_LOGI(TAG, "SHT31 ready — SDA=GPIO%d SCL=GPIO%d addr=0x%02X",
             CONFIG_SHT31_I2C_SDA, CONFIG_SHT31_I2C_SCL, SHT31_I2C_ADDR);
    return ESP_OK;
}

esp_err_t sht31_read(sht31_data_t *data)
{
    if (data == NULL)    return ESP_ERR_INVALID_ARG;
    if (!_initialised)   return ESP_ERR_INVALID_STATE;

    /* Send measurement command */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHT31_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, CMD_MEAS_HIGH_NO_CS_MSB, true);
    i2c_master_write_byte(cmd, CMD_MEAS_HIGH_NO_CS_LSB, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(SHT31_I2C_PORT, cmd, pdMS_TO_TICKS(SHT31_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "measurement command failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Wait for measurement to complete (~15 ms for high repeatability) */
    vTaskDelay(pdMS_TO_TICKS(15));

    /* Read 6 bytes: [rawT MSB, rawT LSB, CRC_T, rawRH MSB, rawRH LSB, CRC_RH] */
    uint8_t buf[6] = {0};
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHT31_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buf, sizeof(buf), I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(SHT31_I2C_PORT, cmd, pdMS_TO_TICKS(SHT31_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Validate CRC for temperature word */
    if (_crc8(buf, 2) != buf[2]) {
        ESP_LOGE(TAG, "temperature CRC mismatch (got 0x%02X expected 0x%02X)",
                 buf[2], _crc8(buf, 2));
        return ESP_ERR_INVALID_CRC;
    }

    /* Validate CRC for humidity word */
    if (_crc8(buf + 3, 2) != buf[5]) {
        ESP_LOGE(TAG, "humidity CRC mismatch (got 0x%02X expected 0x%02X)",
                 buf[5], _crc8(buf + 3, 2));
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t raw_t  = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_rh = ((uint16_t)buf[3] << 8) | buf[4];

    data->temperature_c = -45.0f + 175.0f * ((float)raw_t  / 65535.0f);
    data->humidity_pct  = 100.0f          * ((float)raw_rh / 65535.0f);

    ESP_LOGD(TAG, "T=%.2f°C  RH=%.1f%%", data->temperature_c, data->humidity_pct);
    return ESP_OK;
}

void sht31_deinit(void)
{
    if (!_initialised) return;
    i2c_driver_delete(SHT31_I2C_PORT);
    _initialised = false;
    ESP_LOGI(TAG, "SHT31 de-initialised");
}
