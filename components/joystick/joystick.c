/**
 * @file joystick.c
 *
 * @brief Driver for the analog joystick on the BLDK peripheral module.
 *
 * Reads X (GPIO34 / ADC1_CH6) and Y (GPIO35 / ADC1_CH7) axes via the
 * ESP-IDF legacy ADC driver. Raw 12-bit values (0-4095) are normalized
 * to -100..+100. Only fires callback when position changes meaningfully.
 *
 * COPYRIGHT NOTICE: (c) 2025 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "joystick.h"

#include "driver/adc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//---------------------------------- MACROS -----------------------------------
#define TAG         "joystick"

#define ADC_CH_X    ADC1_CHANNEL_6   /* GPIO34 */
#define ADC_CH_Y    ADC1_CHANNEL_7   /* GPIO35 */
#define ADC_WIDTH   ADC_WIDTH_BIT_12
#define ADC_ATTEN   ADC_ATTEN_DB_12  /* 0..3.3 V range */
#define ADC_MAX     4095
#define ADC_MID     2048

/** Only fire callback when position changes by more than this amount. */
#define CHANGE_THRESHOLD  3

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
static void   _joystick_task(void *arg);
static int8_t _normalize(int raw);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static joystick_event_cb_t _event_cb    = NULL;
static TaskHandle_t        _task_handle = NULL;

//------------------------------ PUBLIC FUNCTIONS -----------------------------
esp_err_t joystick_init(joystick_event_cb_t cb)
{
    _event_cb = cb;

    esp_err_t ret = adc1_config_width(ADC_WIDTH);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "adc1_config_width failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = adc1_config_channel_atten(ADC_CH_X, ADC_ATTEN);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "adc1_config_channel_atten X failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = adc1_config_channel_atten(ADC_CH_Y, ADC_ATTEN);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "adc1_config_channel_atten Y failed: %s", esp_err_to_name(ret));
        return ret;
    }

    BaseType_t res = xTaskCreatePinnedToCore(
        _joystick_task, "joystick", 2048, NULL, 5, &_task_handle, 0);

    if(res != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create joystick task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Joystick driver initialised (X=GPIO%d, Y=GPIO%d)",
             JOYSTICK_GPIO_X, JOYSTICK_GPIO_Y);

    return ESP_OK;
}

esp_err_t joystick_read(joystick_pos_t *pos)
{
    if(pos == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    pos->x = _normalize(adc1_get_raw(ADC_CH_X));
    pos->y = _normalize(adc1_get_raw(ADC_CH_Y));

    return ESP_OK;
}

void joystick_deinit(void)
{
    if(_task_handle != NULL)
    {
        vTaskDelete(_task_handle);
        _task_handle = NULL;
    }
    ESP_LOGI(TAG, "Joystick driver de-initialised");
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------
static int8_t _normalize(int raw)
{
    int centred = raw - ADC_MID;

    /* Dead-zone: ~±82 counts (~2% of full scale). */
    if(centred > -82 && centred < 82)
    {
        return 0;
    }

    int normalized;
    if(centred < 0)
    {
        normalized = (centred * 100) / ADC_MID;
    }
    else
    {
        normalized = (centred * 100) / (ADC_MAX - ADC_MID);
    }

    if(normalized < -100) normalized = -100;
    if(normalized >  100) normalized =  100;

    return (int8_t)normalized;
}

static void _joystick_task(void *arg)
{
    joystick_pos_t pos  = { 0, 0 };
    joystick_pos_t last = { 0, 0 };

    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(JOYSTICK_POLL_MS));

        if(joystick_read(&pos) != ESP_OK)
        {
            continue;
        }

        int dx = (int)pos.x - (int)last.x;
        int dy = (int)pos.y - (int)last.y;
        if(dx < 0) dx = -dx;
        if(dy < 0) dy = -dy;

        if(dx > CHANGE_THRESHOLD || dy > CHANGE_THRESHOLD)
        {
            last = pos;
            if(_event_cb != NULL)
            {
                _event_cb(pos);
            }
        }
    }
}

//---------------------------- INTERRUPT HANDLERS -----------------------------