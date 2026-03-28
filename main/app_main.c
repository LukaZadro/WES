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
#include "wifi_station.h"
#include "mqtt_client_bl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

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

   play_tetris();
   ESP_LOGI(TAG, "Done.");

   max98357a_deinit();
   vTaskDelete(NULL);
}

void app_main(void)
{
   wifi_station_init();
   mqtt_client_bl_init();

   xTaskCreatePinnedToCore(_sound_task, "sound", 4096, NULL, 5, NULL, 0);
   gui_init();
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

//---------------------------- INTERRUPT HANDLERS -----------------------------

