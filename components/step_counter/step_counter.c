/**
 * @file step_counter.c
 *
 * @brief Pedometer using the LIS3DH accelerometer.
 *
 * Polls the accelerometer at 10 Hz (matching the sensor ODR).
 * Computes vector magnitude, removes gravity with a slow exponential
 * moving average, and counts steps when the filtered signal crosses
 * a positive threshold — with a debounce to avoid double-counting.
 *
 * Tuning parameters:
 *   STEP_THRESHOLD_MG   — minimum peak above baseline to count a step (~0.2 g)
 *   STEP_DEBOUNCE_MS    — minimum time between two steps (~400 ms)
 *   BASELINE_ALPHA      — EMA coefficient for gravity removal (closer to 1 = slower)
 */

#include "step_counter.h"
#include "accelerometer.h"

#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "steps"

#define STEP_THRESHOLD_MG   200      /* mg above baseline to count a step  */
#define STEP_DEBOUNCE_MS    400      /* min ms between consecutive steps   */
#define BASELINE_ALPHA      0.95f    /* gravity EMA: ~2 s settling at 10Hz */
#define POLL_INTERVAL_MS    100      /* 10 Hz — matches sensor ODR         */

static volatile uint32_t _steps        = 0;
static bool              _above        = false;   /* currently above threshold */
static float             _baseline     = 1000.0f; /* initial guess: 1 g       */

static void _step_task(void *arg)
{
    TickType_t last_step_tick = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));

        acc_data_t d;
        if (acc_read(&d) != ESP_OK) continue;

        /* Magnitude of acceleration vector (mg) */
        float mag = sqrtf((float)d.x * d.x +
                          (float)d.y * d.y +
                          (float)d.z * d.z);

        /* Update slow baseline — tracks gravity, ignores fast shocks */
        _baseline = BASELINE_ALPHA * _baseline + (1.0f - BASELINE_ALPHA) * mag;

        float filtered = mag - _baseline;

        TickType_t now = xTaskGetTickCount();

        if (!_above && filtered > STEP_THRESHOLD_MG) {
            /* Rising edge — check debounce */
            _above = true;
            if ((now - last_step_tick) >= pdMS_TO_TICKS(STEP_DEBOUNCE_MS)) {
                _steps++;
                last_step_tick = now;
                ESP_LOGI(TAG, "Step detected! Total: %lu", (unsigned long)_steps);
            }
        } else if (_above && filtered < (STEP_THRESHOLD_MG / 2)) {
            /* Falling edge — reset so the next peak can be detected */
            _above = false;
        }
    }
}

esp_err_t step_counter_init(void)
{
    BaseType_t ok = xTaskCreate(_step_task, "steps", 2048, NULL, 3, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create step task");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "Step counter started");
    return ESP_OK;
}

uint32_t step_counter_get(void)
{
    return _steps;
}

void step_counter_reset(void)
{
    _steps = 0;
}
