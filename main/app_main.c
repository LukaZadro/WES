#include "core/lv_group.h"
#include "gui.h"
#include "nav.h"
#include "max98357a.h"
#include "ui_app/tetris_ui_draw.h"
#include "ui_app/memory_ui_draw.h"
#include "wifi_station.h"
#include "mqtt_client_bl.h"
#include "buttons.h"
#include "joystick.h"
#include "ui.h"
#include "ui_events.h"
#include "accelerometer.h"
#include "sleep_timer.h"
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

static QueueHandle_t _btn_queue;

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
            sleep_timer_reset();

            switch (evt.id)
            {
                case BUTTON_1:
                    if (evt.pressed)
                        if (!tetris_movement(tetris_rotate_right))
                            lv_async_call(_toggle_switch_cb, NULL);
                    break;
                case BUTTON_2:
                    /* Back button — navigate to the previous screen */
                    if (evt.pressed) nav_go_back();
                    break;
                case BUTTON_3:
                    break;
                case BUTTON_4:
                    if (evt.pressed) {
                        if (lv_scr_act() == ui_MemoryScreen)
                            memory_ui_select();
                        else
                            nav_send_key(LV_KEY_ENTER, LV_INDEV_STATE_PR);
                    } else {
                        if (lv_scr_act() != ui_MemoryScreen)
                            nav_send_key(LV_KEY_ENTER, LV_INDEV_STATE_REL);
                    }
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
    static uint32_t prev_dir = 0;

    int16_t ax = pos.x < 0 ? -pos.x : pos.x;
    int16_t ay = pos.y < 0 ? -pos.y : pos.y;

    uint32_t dir = 0;

    if (ax > ay) {
        /* X axis dominates — hardware is mounted with X inverted, so swap */
        if      (pos.x >  JOY_THRESHOLD) dir = LV_KEY_LEFT;
        else if (pos.x < -JOY_THRESHOLD) dir = LV_KEY_RIGHT;
    } else {
        /* Y axis dominates */
        if      (pos.y >  JOY_THRESHOLD) dir = LV_KEY_DOWN;
        else if (pos.y < -JOY_THRESHOLD) dir = LV_KEY_UP;
    }

    if (dir != 0) sleep_timer_reset();

    bool moved = false;

    if (dir != 0 && lv_scr_act() == ui_MemoryScreen) {
        /* Memory screen: move cursor, no auto-repeat */
        if (dir != prev_dir)
            memory_ui_joystick(dir);
        prev_dir = dir;
        return;
    }

    switch (dir) {
        case 0:
            break;
        case LV_KEY_LEFT:
            moved = tetris_movement(tetris_move_left);
            break;
        case LV_KEY_RIGHT:
            moved = tetris_movement(tetris_move_right);
            break;
        case LV_KEY_DOWN:
            moved = tetris_movement(tetris_soft_drop);
            break;
        case LV_KEY_UP:
            moved = tetris_movement(tetris_hard_drop);
            break;
    }

    /* Fire spatial navigation once per new direction (no auto-repeat spam) */
    if (!moved && dir != 0 && dir != prev_dir)
        nav_move_dir(dir);

    prev_dir = dir;
}

/* ------------------------------------------------------------------ */
/*  Sleep callbacks                                                     */
/* ------------------------------------------------------------------ */
static void _on_sleep_cb(void)
{
    acc_prepare_sleep();   /* Clear latched INT1 so ext1 doesn't fire instantly */
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

    /*
     * Wait for the GUI task to initialise the VSPI bus (lvgl_driver_init)
     * before adding the accelerometer as another device on that bus.
     */
    vTaskDelay(pdMS_TO_TICKS(200));

    acc_init(NULL);
    sleep_timer_init(_on_sleep_cb, NULL);
}