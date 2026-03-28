/**
 * @file buttons.c
 *
 * @brief Driver for the 4 user push-buttons on the BLDK peripheral module.
 *
 * Buttons are wired with 10 k pull-DOWN resistors (active-HIGH).
 *
 * GPIO36 (BTN1) is input-only on ESP32 — no interrupt support.
 * It is handled by a polling task running every 20 ms.
 * GPIO32 (BTN2), GPIO33 (BTN3), GPIO25 (BTN4) use interrupts normally.
 *
 * COPYRIGHT NOTICE: (c) 2025 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "buttons.h"

#include "esp_attr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//---------------------------------- MACROS -----------------------------------
#define TAG     "button"

/** Debounce window in microseconds (5 ms) — used by ISR buttons. */
#define DEBOUNCE_US  (5000ULL)

/** Poll interval for GPIO36 (BTN1) in milliseconds. */
#define POLL_MS      (20U)

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
static void IRAM_ATTR _gpio_isr_handler(void *arg);
static void           _poll_task(void *arg);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static const gpio_num_t _btn_gpio[BUTTON_COUNT] = {
    BUTTON_GPIO_BTN1,   /* GPIO36 - input only, polled */
    BUTTON_GPIO_BTN2,   /* GPIO32 - interrupt          */
    BUTTON_GPIO_BTN3,   /* GPIO33 - interrupt          */
    BUTTON_GPIO_BTN4,   /* GPIO25 - interrupt          */
};

static button_event_cb_t _event_cb                  = NULL;
static int64_t           _last_isr_us[BUTTON_COUNT] = { 0 };
static bool              _last_state[BUTTON_COUNT]  = { false };
static TaskHandle_t      _poll_task_handle           = NULL;

//------------------------------ PUBLIC FUNCTIONS -----------------------------
esp_err_t button_init(button_event_cb_t cb)
{
    _event_cb = cb;

    esp_err_t ret = gpio_install_isr_service(0);
    if(ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(ret));
        return ret;
    }

    for(int i = 0; i < BUTTON_COUNT; i++)
    {
        gpio_num_t pin       = _btn_gpio[i];
        bool       input_only = (pin == 36);

        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << pin),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = input_only ? GPIO_INTR_DISABLE : GPIO_INTR_ANYEDGE,
        };

        ret = gpio_config(&io_conf);
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG, "gpio_config failed for GPIO%d: %s", pin, esp_err_to_name(ret));
            return ret;
        }

        if(!input_only)
        {
            ret = gpio_isr_handler_add(pin, _gpio_isr_handler, (void *)(intptr_t)i);
            if(ret != ESP_OK)
            {
                ESP_LOGE(TAG, "gpio_isr_handler_add failed for GPIO%d: %s", pin, esp_err_to_name(ret));
                return ret;
            }
        }

        /* Capture initial state to avoid false events on startup. */
        _last_state[i] = (gpio_get_level(pin) == 1);
    }

    /* Polling task handles BTN1 (GPIO36). */
    BaseType_t res = xTaskCreate(_poll_task, "btn_poll", 2048, NULL, 5, &_poll_task_handle);
    if(res != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create button poll task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Button driver initialised (BTN1=GPIO%d, BTN2=GPIO%d, BTN3=GPIO%d, BTN4=GPIO%d)",
             BUTTON_GPIO_BTN1, BUTTON_GPIO_BTN2, BUTTON_GPIO_BTN3, BUTTON_GPIO_BTN4);

    return ESP_OK;
}

esp_err_t button_read(button_id_t id, bool *pressed)
{
    if(id >= BUTTON_COUNT || pressed == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *pressed = (gpio_get_level(_btn_gpio[id]) == 1);
    return ESP_OK;
}

void button_deinit(void)
{
    if(_poll_task_handle != NULL)
    {
        vTaskDelete(_poll_task_handle);
        _poll_task_handle = NULL;
    }

    for(int i = 0; i < BUTTON_COUNT; i++)
    {
        gpio_num_t pin = _btn_gpio[i];
        if(pin != 36)
        {
            gpio_isr_handler_remove(pin);
        }
        gpio_reset_pin(pin);
    }
    ESP_LOGI(TAG, "Button driver de-initialised");
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

/**
 * @brief Polling task for BTN1 (GPIO36, input-only).
 *
 * Checks state every POLL_MS ms. Two consecutive matching reads
 * required before firing the callback (debounce).
 */
static void _poll_task(void *arg)
{
    bool stable = _last_state[BUTTON_1];

    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));

        bool current = (gpio_get_level(_btn_gpio[BUTTON_1]) == 1);

        if(current != _last_state[BUTTON_1])
        {
            if(current == stable)
            {
                _last_state[BUTTON_1] = current;
                if(_event_cb != NULL)
                {
                    _event_cb(BUTTON_1, current);
                }
            }
        }
        stable = current;
    }
}

static void IRAM_ATTR _gpio_isr_handler(void *arg)
{
    if(_event_cb == NULL)
    {
        return;
    }

    int     id  = (int)(intptr_t)arg;
    int64_t now = esp_timer_get_time();

    if((now - _last_isr_us[id]) < DEBOUNCE_US)
    {
        return;
    }
    _last_isr_us[id] = now;

    bool pressed = (gpio_get_level(_btn_gpio[id]) == 1);
    _event_cb((button_id_t)id, pressed);
}

//---------------------------- INTERRUPT HANDLERS -----------------------------