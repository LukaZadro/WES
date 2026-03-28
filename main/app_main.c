#include "gui.h"
#include "max98357a.h"
#include "wifi_station.h"
#include "mqtt_client_bl.h"
#include "buttons.h"
#include "joystick.h"
#include "ui.h"
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

/* ------------------------------------------------------------------ */
/*  Entry point                                                         */
/* ------------------------------------------------------------------ */
static void _sound_task(void *arg)
{
    max98357a_cfg_t cfg = {
        .i2s_num         = 0,
        .gpio_bclk       = 26,
        .gpio_lrc        = 21,  /* GPIO21 — free, not a strapping pin */
        .gpio_dout       = 27,
        .gpio_sd_mode    = -1,
        .sample_rate     = 44100,
        .bits_per_sample = 16,
    };
    max98357a_init(&cfg);
    /* write 500ms of true silence (zeros) between loops */
    static const int16_t silence[512] = {0};
    uint32_t silence_loops = (44100 * 500 / 1000) / (sizeof(silence) / 4);

    while(1)
    {
        play_tetris();
        for(uint32_t i = 0; i < silence_loops; i++)
            max98357a_play_raw(silence, sizeof(silence), 1000);
    }
}

void app_main(void)
{
    /* Audio – init amp and play startup sound on Core 0 */
    xTaskCreatePinnedToCore(_sound_task, "sound", 4096, NULL, 5, NULL, 0);

    /* Buttons */
    _btn_queue = xQueueCreate(10, sizeof(btn_event_t));
    xTaskCreate(_button_task, "btn_task", 2048, NULL, 5, NULL);
    button_init(_button_event_cb);

    joystick_init(_joystick_event_cb);

    /* WiFi + MQTT */
    wifi_station_init();
    mqtt_client_bl_init();

    /* GUI – pins to Core 1 */
    gui_init();
}