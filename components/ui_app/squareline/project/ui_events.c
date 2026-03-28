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

void tetris_music_on(lv_event_t * e)
{
	// Your code here
}

void is_blue_mode(lv_event_t * e)
{
	// Your code here
}
