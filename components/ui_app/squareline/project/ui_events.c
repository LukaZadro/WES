// ui_events.c
#include "ui.h"

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
