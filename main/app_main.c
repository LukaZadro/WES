#include "gui.h"
#include "buttons.h"
#include "joystick.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TAG "main"

/* ------------------------------------------------------------------ */
/*  Button queue                                                        */
/* ------------------------------------------------------------------ */
typedef struct
{
    button_id_t id;
    bool        pressed;
} btn_event_t;

static QueueHandle_t      _btn_queue;
static const char * const _btn_names[BUTTON_COUNT] = {
    "BTN1", "BTN2", "BTN3", "BTN4"
};

static void _button_event_cb(button_id_t id, bool pressed)
{
    btn_event_t evt = { .id = id, .pressed = pressed };
    xQueueSendFromISR(_btn_queue, &evt, NULL);
}

static void _button_task(void *arg)
{
    btn_event_t evt;
    while(1)
    {
        if(xQueueReceive(_btn_queue, &evt, portMAX_DELAY))
        {
            if(evt.id < BUTTON_COUNT)
            {
                ESP_LOGI(TAG, "%s %s", _btn_names[evt.id],
                         evt.pressed ? "PRESSED" : "RELEASED");
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Joystick callback                                                   */
/* ------------------------------------------------------------------ */
static void _joystick_event_cb(joystick_pos_t pos)
{
    ESP_LOGI(TAG, "JOY  X: %4d  Y: %4d", pos.x, pos.y);
}

/* ------------------------------------------------------------------ */
/*  Entry point                                                         */
/* ------------------------------------------------------------------ */
void app_main(void)
{
    /* Buttons */
    _btn_queue = xQueueCreate(10, sizeof(btn_event_t));
    xTaskCreate(_button_task, "btn_task", 2048, NULL, 5, NULL);
    button_init(_button_event_cb);

    /* Joystick */
    joystick_init(_joystick_event_cb);

    /* GUI */
    gui_init();
}