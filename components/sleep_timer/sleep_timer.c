/**
 * @file sleep_timer.c
 *
 * @brief Inactivity-based light sleep timer.
 *
 * A one-shot esp_timer counts SLEEP_TIMEOUT_S seconds. Any input event calls
 * sleep_timer_reset() to restart it. When it fires, a dedicated sleep task
 * calls on_sleep(), configures GPIO/ext1 wakeup sources, and enters light
 * sleep. On wakeup it calls on_wake() and restarts the timer.
 *
 * sleep_timer_reset() must be called from task context (not ISR) because
 * esp_timer_restart is not ISR-safe.
 */

#include "sleep_timer.h"

#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "sleep"

#define WAKE_BTN2   GPIO_NUM_32
#define WAKE_BTN3   GPIO_NUM_33
#define WAKE_BTN4   GPIO_NUM_25
#define WAKE_TOUCH  GPIO_NUM_15
#define WAKE_ACC    GPIO_NUM_39

static esp_timer_handle_t _timer      = NULL;
static TaskHandle_t       _sleep_task = NULL;
static sleep_cb_t         _on_sleep   = NULL;
static sleep_cb_t         _on_wake    = NULL;

/* ---- Timer callback ------------------------------------------------------ */

static void _timeout_cb(void *arg)
{
    xTaskNotifyGive(_sleep_task);
}

/* ---- Wakeup sources ------------------------------------------------------ */

static void _configure_wakeup(void)
{
    gpio_wakeup_enable(WAKE_BTN2,  GPIO_INTR_HIGH_LEVEL);
    gpio_wakeup_enable(WAKE_BTN3,  GPIO_INTR_HIGH_LEVEL);
    gpio_wakeup_enable(WAKE_BTN4,  GPIO_INTR_HIGH_LEVEL);
    gpio_wakeup_enable(WAKE_TOUCH, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    /* ACC IRQ1 on GPIO39 needs ext1 (RTC GPIO, above gpio_wakeup limit of 33). */
    esp_sleep_enable_ext1_wakeup(1ULL << WAKE_ACC, ESP_EXT1_WAKEUP_ANY_HIGH);
}

static const char *_cause_str(esp_sleep_wakeup_cause_t c)
{
    switch (c) {
        case ESP_SLEEP_WAKEUP_GPIO:  return "GPIO (button or touch)";
        case ESP_SLEEP_WAKEUP_EXT1:  return "EXT1 (accelerometer hard shake)";
        case ESP_SLEEP_WAKEUP_TIMER: return "timer";
        default:                     return "unknown";
    }
}

/* ---- Sleep task ---------------------------------------------------------- */

static void _sleep_task_fn(void *arg)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (esp_timer_is_active(_timer)) continue;

        ESP_LOGW(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        ESP_LOGW(TAG, "  No input for %us — entering LIGHT SLEEP", SLEEP_TIMEOUT_S);
        ESP_LOGW(TAG, "  Wakeup: press any button, touch screen, or hard shake");
        ESP_LOGW(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

        if (_on_sleep != NULL) {
            _on_sleep();
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        _configure_wakeup();
        esp_light_sleep_start();

        /* ── post-wakeup ── */
        if (_on_wake != NULL) _on_wake();

        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        ESP_LOGI(TAG, "  Woke from light sleep — cause: %s", _cause_str(cause));
        ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

        sleep_timer_reset();
    }
}

/* ---- Public API ---------------------------------------------------------- */

esp_err_t sleep_timer_init(sleep_cb_t on_sleep, sleep_cb_t on_wake)
{
    _on_sleep = on_sleep;
    _on_wake  = on_wake;

    BaseType_t ok = xTaskCreatePinnedToCore(
        _sleep_task_fn, "sleep_tmr", 2048, NULL, 4, &_sleep_task, 0);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sleep task");
        return ESP_ERR_NO_MEM;
    }

    const esp_timer_create_args_t args = { .callback = _timeout_cb, .name = "inactivity" };
    esp_err_t ret = esp_timer_create(&args, &_timer);
    if (ret != ESP_OK) return ret;

    ret = esp_timer_start_once(_timer, (uint64_t)SLEEP_TIMEOUT_S * 1000000ULL);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "Inactivity timer started — sleep after %us of no input", SLEEP_TIMEOUT_S);
    return ESP_OK;
}

void sleep_timer_reset(void)
{
    if (_timer == NULL) return;
    esp_err_t ret = esp_timer_restart(_timer, (uint64_t)SLEEP_TIMEOUT_S * 1000000ULL);
    if (ret == ESP_ERR_INVALID_STATE) {
        esp_timer_start_once(_timer, (uint64_t)SLEEP_TIMEOUT_S * 1000000ULL);
    }
}

void sleep_timer_lv_feedback(lv_indev_drv_t *drv, uint8_t code)
{
    (void)drv;
    (void)code;
    sleep_timer_reset();
}
