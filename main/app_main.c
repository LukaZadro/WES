#include "gui.h"
#include "buttons.h"
#include "joystick.h"
#include "ui.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TAG "main"

typedef struct {
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

static void _toggle_switch_cb(void *arg)
{
    if (ui_ColorSwitch == NULL) return;

    if (lv_obj_has_state(ui_ColorSwitch, LV_STATE_CHECKED)) {
        lv_obj_clear_state(ui_ColorSwitch, LV_STATE_CHECKED);
        plava_boja(NULL);
    } else {
        lv_obj_add_state(ui_ColorSwitch, LV_STATE_CHECKED);
        roza_boja(NULL);
    }
}

static void _go_poruke_cb(void *arg)
{
    _ui_screen_change(&ui_Poruke1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Poruke1_screen_init);
}

static void _button_task(void *arg)
{
    btn_event_t evt;
    while (1)
    {
        if (xQueueReceive(_btn_queue, &evt, portMAX_DELAY))
        {
            if (!evt.pressed) continue;

            ESP_LOGI(TAG, "%s PRESSED", _btn_names[evt.id]);

            switch (evt.id)
            {
                case BUTTON_1:
                    lv_async_call(_toggle_switch_cb, NULL);
                    break;
                case BUTTON_2:
                    lv_async_call(_go_poruke_cb, NULL);
                    break;
                case BUTTON_3:
                    break;
                case BUTTON_4:
                    break;
                default:
                    break;
            }
        }
    }
}

static void _joystick_event_cb(joystick_pos_t pos)
{
    ESP_LOGI(TAG, "JOY  X: %4d  Y: %4d", pos.x, pos.y);
}

void app_main(void)
{
    _btn_queue = xQueueCreate(10, sizeof(btn_event_t));
    xTaskCreate(_button_task, "btn_task", 2048, NULL, 5, NULL);
    button_init(_button_event_cb);

    joystick_init(_joystick_event_cb);

    gui_init();
}