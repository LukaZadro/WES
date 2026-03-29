/**
 * @file accelerometer.c
 *
 * @brief LIS3DH accelerometer driver over SPI (CS=GPIO13, IRQ1=GPIO39).
 *
 * Shares the VSPI bus with the ILI9341 display and XPT2046 touch controller.
 * Adds the LIS3DH as an extra SPI device on the existing host. If the bus is
 * not yet initialised this driver initialises it first.
 *
 * Configuration:
 *   CTRL_REG1 = 0x2F   10 Hz, all axes enabled
 *   CTRL_REG2 = 0x01   HP normal mode (running baseline), filter on INT1
 *   CTRL_REG3 = 0x40   IA1 interrupt routed to INT1 pin
 *   CTRL_REG4 = 0x08   High-resolution mode, ±2 g full scale
 *   CTRL_REG5 = 0x08   Latch INT1 (cleared by reading INT1_SRC)
 *   INT1_CFG  = 0x2A   OR logic — XHIE | YHIE | ZHIE
 *   INT1_THS  = 0x60   96 × 16 mg = 1536 mg ≈ 1.5 g (hard shake only)
 *   INT1_DUR  = 0x00   Immediate
 */

#include "accelerometer.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "acc"

/* ---- LIS3DH register addresses ------------------------------------------ */
#define REG_WHO_AM_I  0x0F
#define REG_CTRL1     0x20
#define REG_CTRL2     0x21
#define REG_CTRL3     0x22
#define REG_CTRL4     0x23
#define REG_CTRL5     0x24
#define REG_OUT_X_L   0x28
#define REG_INT1_CFG  0x30
#define REG_INT1_SRC  0x31
#define REG_INT1_THS  0x32
#define REG_INT1_DUR  0x33

#define SPI_RD        0x80
#define SPI_MS        0x40
#define LIS3DH_ID     0x33

/* ---- Module state -------------------------------------------------------- */
static spi_device_handle_t _dev         = NULL;
static acc_activity_cb_t   _activity_cb = NULL;

/* ---- SPI helpers --------------------------------------------------------- */

static esp_err_t _write_reg(uint8_t reg, uint8_t val)
{
    spi_transaction_t t = {
        .flags   = SPI_TRANS_USE_TXDATA,
        .length  = 16,
        .tx_data = { reg & 0x3F, val },
    };
    return spi_device_polling_transmit(_dev, &t);
}

static esp_err_t _read_reg(uint8_t reg, uint8_t *out)
{
    spi_transaction_t t = {
        .flags   = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA,
        .length  = 16,
        .tx_data = { SPI_RD | (reg & 0x3F), 0x00 },
    };
    esp_err_t err = spi_device_polling_transmit(_dev, &t);
    if (err == ESP_OK) *out = t.rx_data[1];
    return err;
}

static esp_err_t _read_burst(uint8_t reg, uint8_t *buf, size_t len)
{
    uint8_t tx[7] = { SPI_RD | SPI_MS | (reg & 0x3F) };
    uint8_t rx[7] = { 0 };
    spi_transaction_t t = {
        .length    = (len + 1) * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    esp_err_t err = spi_device_polling_transmit(_dev, &t);
    if (err == ESP_OK) {
        for (size_t i = 0; i < len; i++) buf[i] = rx[i + 1];
    }
    return err;
}

/* ---- Sleep helper -------------------------------------------------------- */

esp_err_t acc_prepare_sleep(void)
{
    if (_dev == NULL) return ESP_ERR_INVALID_STATE;
    uint8_t dummy;
    _read_reg(REG_INT1_SRC, &dummy);
    vTaskDelay(pdMS_TO_TICKS(150));
    return _read_reg(REG_INT1_SRC, &dummy);
}

/* ---- IRQ1 ISR ------------------------------------------------------------ */

static void IRAM_ATTR _irq1_isr(void *arg)
{
    if (_activity_cb != NULL) _activity_cb();
}

/* ---- Public API ---------------------------------------------------------- */

esp_err_t acc_init(acc_activity_cb_t activity_cb)
{
    _activity_cb = activity_cb;

    spi_bus_config_t bus = {
        .mosi_io_num   = ACC_GPIO_MOSI,
        .miso_io_num   = ACC_GPIO_MISO,
        .sclk_io_num   = ACC_GPIO_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    esp_err_t ret = spi_bus_initialize(ACC_SPI_HOST, &bus, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 5 * 1000 * 1000,
        .mode           = 3,
        .spics_io_num   = ACC_GPIO_CS,
        .queue_size     = 4,
    };
    ret = spi_bus_add_device(ACC_SPI_HOST, &dev_cfg, &_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device: %s", esp_err_to_name(ret));
        return ret;
    }

    uint8_t who = 0;
    ret = _read_reg(REG_WHO_AM_I, &who);
    if (ret != ESP_OK || who != LIS3DH_ID) {
        ESP_LOGE(TAG, "WHO_AM_I: got 0x%02X expected 0x%02X", who, LIS3DH_ID);
        spi_bus_remove_device(_dev); _dev = NULL;
        return (ret == ESP_OK) ? ESP_ERR_NOT_FOUND : ret;
    }

    ESP_ERROR_CHECK(_write_reg(REG_CTRL1,    0x2F));
    ESP_ERROR_CHECK(_write_reg(REG_CTRL2,    0x01));
    ESP_ERROR_CHECK(_write_reg(REG_CTRL3,    0x40));
    ESP_ERROR_CHECK(_write_reg(REG_CTRL4,    0x08));
    ESP_ERROR_CHECK(_write_reg(REG_CTRL5,    0x08));
    ESP_ERROR_CHECK(_write_reg(REG_INT1_CFG, 0x2A));
    ESP_ERROR_CHECK(_write_reg(REG_INT1_THS, 0x60)); /* 1536 mg ≈ 1.5 g */
    ESP_ERROR_CHECK(_write_reg(REG_INT1_DUR, 0x00));

    if (activity_cb != NULL) {
        gpio_config_t io = {
            .pin_bit_mask = 1ULL << ACC_GPIO_IRQ1,
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_POSEDGE,
        };
        ESP_ERROR_CHECK(gpio_config(&io));
        ESP_ERROR_CHECK(gpio_isr_handler_add(ACC_GPIO_IRQ1, _irq1_isr, NULL));
    }

    ESP_LOGI(TAG, "LIS3DH ready — CS=GPIO%d IRQ1=GPIO%d threshold=1536mg",
             ACC_GPIO_CS, ACC_GPIO_IRQ1);
    return ESP_OK;
}

esp_err_t acc_read(acc_data_t *data)
{
    if (data == NULL)  return ESP_ERR_INVALID_ARG;
    if (_dev  == NULL) return ESP_ERR_INVALID_STATE;
    uint8_t raw[6];
    esp_err_t err = _read_burst(REG_OUT_X_L, raw, 6);
    if (err != ESP_OK) return err;
    data->x = (int16_t)((raw[1] << 8) | raw[0]) >> 4;
    data->y = (int16_t)((raw[3] << 8) | raw[2]) >> 4;
    data->z = (int16_t)((raw[5] << 8) | raw[4]) >> 4;
    return ESP_OK;
}

void acc_deinit(void)
{
    if (_activity_cb != NULL) {
        gpio_isr_handler_remove(ACC_GPIO_IRQ1);
        gpio_reset_pin(ACC_GPIO_IRQ1);
        _activity_cb = NULL;
    }
    if (_dev != NULL) { spi_bus_remove_device(_dev); _dev = NULL; }
    ESP_LOGI(TAG, "LIS3DH de-initialised");
}
