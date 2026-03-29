#include "core/lv_obj_pos.h"
#include "esp_log.h"
#include "memory_ui_draw.h"
#include "memory.h"
#include "ui.h"

#define TAG "memory_draw"

/* Constants for the UI look */
#define CARD_MARGIN 4
#define CARD_RADIUS 5

memory_game_t *memory_game = NULL;

static void ui_event_draw_memory_board(lv_event_t * e)
{
    memory_game_t *game = memory_game;
    if (game == NULL) {
        ESP_LOGW(TAG, "game is NULL");
        return;
    }

    lv_obj_t * obj = lv_event_get_target(e);
    lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);
    
    lv_coord_t w = lv_obj_get_width(obj);
    lv_coord_t h = lv_obj_get_height(obj);
    lv_coord_t cell_w = w / CARD_COLUMNS;
    lv_coord_t cell_h = h / CARD_ROWS;

    lv_draw_rect_dsc_t card_dsc;
    lv_draw_rect_dsc_init(&card_dsc);
    card_dsc.radius = CARD_RADIUS;
    card_dsc.border_width = 1;
    card_dsc.border_color = lv_palette_main(LV_PALETTE_GREY);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = lv_color_white();
    label_dsc.font = LV_FONT_DEFAULT;

    for(int r = 0; r < CARD_ROWS; r++) {
        for(int c = 0; c < CARD_COLUMNS; c++) {
            lv_area_t card_area;
            card_area.x1 = obj->coords.x1 + (c * cell_w) + CARD_MARGIN;
            card_area.y1 = obj->coords.y1 + (r * cell_h) + CARD_MARGIN;
            card_area.x2 = card_area.x1 + cell_w - (CARD_MARGIN * 2);
            card_area.y2 = card_area.y1 + cell_h - (CARD_MARGIN * 2);

            bool revealed = memory_is_revealed(game, c, r);

            if (revealed) {
                card_dsc.bg_color = lv_palette_main(LV_PALETTE_BLUE);
                lv_draw_rect(draw_ctx, &card_dsc, &card_area);
                
                // Draw the card symbol (assuming it's a character or number)
                char buf[2] = {game->cards[c][r] + 'a', '\0'};
                lv_draw_label(draw_ctx, &label_dsc, &card_area, buf, NULL);
            } else {
                // Hidden state
                card_dsc.bg_color = lv_palette_main(LV_PALETTE_DEEP_ORANGE);
                lv_draw_rect(draw_ctx, &card_dsc, &card_area);
            }
        }
    }
}

static void ui_event_memory_click(lv_event_t * e)
{
    memory_game_t *game = memory_game;
    if (game == NULL) {
        ESP_LOGW(TAG, "game is NULL");
        return;
    }

    lv_obj_t * obj = lv_event_get_target(e);
    lv_indev_t * indev = lv_indev_get_act();
    if(indev == NULL) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    /* Map absolute touch coordinates to relative grid coordinates */
    lv_coord_t local_x = p.x - obj->coords.x1;
    lv_coord_t local_y = p.y - obj->coords.y1;
    
    int c = local_x * CARD_COLUMNS / lv_obj_get_width(obj);
    int r = local_y * CARD_ROWS / lv_obj_get_height(obj);

    /* Validate bounds and flip card */
    if (r >= 0 && r < CARD_ROWS && c >= 0 && c < CARD_COLUMNS) {
        if (memory_reveal(game, (card_point_t) { .x=c, .y=r })) {
            lv_obj_invalidate(obj); // Trigger redraw
        }
    }
}

/*static void memory_tick_timer_cb(lv_timer_t * timer)
{
    lv_obj_t * panel = (lv_obj_t *)timer->user_data;
    if (memory_needs_update()) {
        memory_logic_tick(); // Handle card flip-back delays
        lv_obj_invalidate(panel);
    }
}*/

bool first_initialization = true;

void setup_memory_ui()
{
    memory_game = (memory_game_t *) lv_mem_alloc(sizeof(memory_game_t));
    memory_init(memory_game);

    ESP_LOGI(TAG, "Initialized memory!");

    lv_obj_add_flag(ui_Container1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_Container1, LV_OBJ_FLAG_SCROLLABLE);

    /* Register the custom draw and click events */
    if (first_initialization) {
        first_initialization = false;
        lv_obj_add_event_cb(ui_Container1, ui_event_draw_memory_board, LV_EVENT_DRAW_MAIN, NULL);
    }
    lv_obj_add_event_cb(ui_Container1, ui_event_memory_click, LV_EVENT_CLICKED, NULL);
    
    /* Optional: If your logic has a timer for flipping cards back, 
       create an lv_timer to check state and invalidate the panel */
    //lv_timer_create(memory_tick_timer_cb, 100, ui_Container1);
}

void destroy_memory_ui(void) {
    lv_obj_remove_event_cb(ui_Container1, ui_event_memory_click);

    lv_mem_free(memory_game);
    memory_game = NULL;
}
