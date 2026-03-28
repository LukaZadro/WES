<<<<<<< HEAD
/**
* @file main.c

* @brief 
* 
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/
//--------------------------------- INCLUDES ----------------------------------
#include "gui.h"
#include "max98357a.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

//---------------------------------- MACROS -----------------------------------
=======
#include "gui.h"
#include "buttons.h"
#include "joystick.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
>>>>>>> 9b63525 (Dodani joystick i gumbovi)

#define TAG "main"

<<<<<<< HEAD
//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static const char *TAG = "app_main";

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------
static void _sound_task(void *arg)
{
   max98357a_cfg_t cfg = {
      .i2s_num         = 0,
      .gpio_bclk       = 26,   /* change to your wired GPIO */
      .gpio_lrc        = 25,   /* change to your wired GPIO */
      .gpio_dout       = 27,   /* change to your wired GPIO */
      .gpio_sd_mode    = -1,   /* -1 if SHDN pin is tied HIGH */
      .sample_rate     = 44100,
      .bits_per_sample = 16,
   };
   ESP_LOGI(TAG, "Initialising amplifier...");
   max98357a_init(&cfg);

   max98357a_play_tone(523, 320, 80);  /* C5 */
   max98357a_play_tone(659, 320, 80);  /* E5 */
   max98357a_play_tone(784, 500, 80);  /* G5 */
   ESP_LOGI(TAG, "Done.");

   max98357a_deinit();
   vTaskDelete(NULL);
}

void app_main(void)
{
   xTaskCreatePinnedToCore(_sound_task, "sound", 4096, NULL, 5, NULL, 0);
   gui_init();
=======
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
>>>>>>> 9b63525 (Dodani joystick i gumbovi)
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