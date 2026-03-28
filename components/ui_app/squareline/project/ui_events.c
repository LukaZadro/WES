// ui_events.c
#include "ui.h"
#include "max98357a.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void plava_boja(lv_event_t * e)
{
    if (ui_HomePage == NULL) return;
    lv_obj_set_style_bg_color(ui_HomePage, lv_color_hex(0x88BADC), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void roza_boja(lv_event_t * e)
{
    if (ui_HomePage == NULL) return;
    lv_obj_set_style_bg_color(ui_HomePage, lv_color_hex(0xFFB6C1), LV_PART_MAIN | LV_STATE_DEFAULT);
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
