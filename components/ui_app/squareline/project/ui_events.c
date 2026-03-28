// ui_events.c
#include "ui.h"
#include "max98357a.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void plava_boja(lv_event_t * e)
{
	(void)e;
	lv_obj_set_style_bg_color(ui_HomePage, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_HomePage, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void roza_boja(lv_event_t * e)
{
	(void)e;
	lv_obj_set_style_bg_color(ui_HomePage, lv_color_hex(0xFF69B4), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_HomePage, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void _tetris_task(void *arg)
{
    static const int16_t silence[512] = {0};
    uint32_t silence_loops = (44100 * 500 / 1000) / (sizeof(silence) / 4);

    while(1)
    {
        play_tetris();
        for(uint32_t i = 0; i < silence_loops; i++)
            max98357a_play_raw(silence, sizeof(silence), 1000);
    }
}

static void _sos_task(void *arg)
{
    play_sos();
    vTaskDelete(NULL);
}

void sos_button_pressed(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        xTaskCreate(_sos_task, "sos", 4096, NULL, 5, NULL);
    }
}

void tetris_music_on(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        xTaskCreate(_tetris_task, "tetris", 4096, NULL, 5, NULL);
    }
}

void is_blue_mode(lv_event_t * e)
{
	// Your code here
}
