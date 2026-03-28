#include "gui.h"
#include "nav.h"
#include "max98357a.h"
#include "wifi_station.h"
#include "mqtt_client_bl.h"
#include "buttons.h"
#include "joystick.h"
#include "ui.h"
#include "ui_events.h"
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
        is_blue_mode(NULL);
    } else {
        lv_obj_add_state(ui_ColorSwitch, LV_STATE_CHECKED);
        roza_boja(NULL);
    }
}

static void _button_task(void *arg)
{
    btn_event_t evt;
    while (1)
    {
        if (xQueueReceive(_btn_queue, &evt, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "%s %s", _btn_names[evt.id],
                     evt.pressed ? "PRESSED" : "RELEASED");

            switch (evt.id)
            {
                case BUTTON_1:
                    if (evt.pressed) lv_async_call(_toggle_switch_cb, NULL);
                    break;
                case BUTTON_2:
                    /* Back button — navigate to the previous screen */
                    if (evt.pressed) nav_go_back();
                    break;
                case BUTTON_3:
                    break;
                case BUTTON_4:
                    /* Select / Enter — press or release the focused widget */
                    nav_send_key(LV_KEY_ENTER,
                                 evt.pressed ? LV_INDEV_STATE_PR
                                             : LV_INDEV_STATE_REL);
                    break;
                default:
                    break;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Joystick → LVGL navigation keys                                    */
/*                                                                     */
/*  Joystick range: -100 … +100 on each axis.                         */
/*  Dominant axis beyond ±JOY_THRESHOLD drives NEXT / PREV focus.     */
/* ------------------------------------------------------------------ */
#define JOY_THRESHOLD 30

static void _joystick_event_cb(joystick_pos_t pos)
{
    static uint32_t prev_key = 0;

    int16_t ax = pos.x < 0 ? -pos.x : pos.x;
    int16_t ay = pos.y < 0 ? -pos.y : pos.y;

    uint32_t key = 0;

    if (ax > ay) {
        /* X axis dominates */
        if      (pos.x >  JOY_THRESHOLD) key = LV_KEY_NEXT;
        else if (pos.x < -JOY_THRESHOLD) key = LV_KEY_PREV;
    } else {
        /* Y axis dominates */
        if      (pos.y >  JOY_THRESHOLD) key = LV_KEY_NEXT;
        else if (pos.y < -JOY_THRESHOLD) key = LV_KEY_PREV;
    }

    if (key != prev_key) {
        if (prev_key != 0) nav_send_key(prev_key, LV_INDEV_STATE_REL);
        if (key      != 0) nav_send_key(key,      LV_INDEV_STATE_PR);
        prev_key = key;
    }
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
    vTaskDelete(NULL);
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