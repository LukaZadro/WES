#include "esp_log.h"
#include "misc/lv_mem.h"
#include "misc/lv_timer.h"
#include "ui.h"
#include "tetris_ui_draw.h"
#include "tetris_engine.h"
#include "ui_TetrisScreen.h"

#define TAG "tetris_draw"

tetris_game_t *game = NULL;

/* This function handles the manual drawing of the Tetris board */
void ui_event_draw_board(lv_event_t * e)
{
    tetris_game_t *g = game;

    if (g == NULL) {
        ESP_LOGW(TAG, "game is NULL");
        return;
    }

    ESP_LOGI(TAG, "Drawing tetris...");
    
    lv_obj_t * obj = lv_event_get_target(e);
    lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);
    
    /* Get the dimensions of our panel */
    lv_coord_t w = lv_obj_get_width(obj);
    lv_coord_t h = lv_obj_get_height(obj);
    
    /* Calculate size of each cell */
    lv_coord_t cell_w = w / BOARD_WIDTH;
    lv_coord_t cell_h = h / BOARD_HEIGHT;

    /* Prepare drawing descriptor for the blocks */
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.border_width = 1;
    rect_dsc.border_color = lv_palette_main(LV_PALETTE_GREY);
    rect_dsc.radius = 2;

    /* 1. Draw the static board */
    for(int y = 0; y < BOARD_HEIGHT; y++) {
        for(int x = 0; x < BOARD_WIDTH; x++) {
            if(g->game_board[y][x] > 0) {
                lv_area_t cell_area;
                cell_area.x1 = obj->coords.x1 + (x * cell_w);
                cell_area.y1 = obj->coords.y1 + (y * cell_h);
                cell_area.x2 = cell_area.x1 + cell_w - 1;
                cell_area.y2 = cell_area.y1 + cell_h - 1;

                /* Set color based on piece type stored in board */
                rect_dsc.bg_color = lv_palette_main(g->game_board[y][x] % 8); 
                lv_draw_rect(draw_ctx, &rect_dsc, &cell_area);
            }
        }
    }

    /* 2. Draw the active falling piece */
    if (!g->game_over) {
        uint16_t shape = shapes[g->current_piece.type][g->current_piece.rotation % 4];
        rect_dsc.bg_color = lv_palette_main(LV_PALETTE_AMBER); // Highlight falling piece
        
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (shape & (1 << (15 - (i * 4 + j)))) {
                    lv_area_t cell_area;
                    cell_area.x1 = obj->coords.x1 + ((g->current_piece.x + j) * cell_w);
                    cell_area.y1 = obj->coords.y1 + ((g->current_piece.y + i) * cell_h);
                    cell_area.x2 = cell_area.x1 + cell_w - 1;
                    cell_area.y2 = cell_area.y1 + cell_h - 1;
                    
                    /* Clip drawing if piece is partially above the board */
                    if (cell_area.y1 >= obj->coords.y1) {
                        lv_draw_rect(draw_ctx, &rect_dsc, &cell_area);
                    }
                }
            }
        }
    }
}

bool tetris_movement(movement_t movement)
{
    tetris_game_t *g = game;
    if (g == NULL) return false;

    switch (movement) {
        case tetris_move_left:
            tetris_move(g, -1);
            break;
        case tetris_move_right:
            tetris_move(g, +1);
            break;
        case tetris_soft_drop:
            tetris_drop(g);
            break;
        case tetris_hard_drop:
            // TODO
            break;
        case tetris_rotate_left:
            // TODO
            break;
        case tetris_rotate_right:
            tetris_rotate(g);
            break;
    }

    lv_obj_invalidate(ui_Panel1);

    return true;
}

static void game_step_timer_cb(lv_timer_t * timer)
{
    if (game != NULL && tetris_tick(game)) {
        /* Force the panel to redraw its custom content */
        lv_obj_invalidate(ui_Panel1);
    }
}

lv_timer_t *update_timer;

/* Call this once during your app initialization */
void setup_tetris_ui(void)
{
    game = (tetris_game_t *) lv_mem_alloc(sizeof(tetris_game_t));
    tetris_init(game);

    ESP_LOGI(TAG, "Initialized tetris!");
    
    /* Connect the drawing event to the panel created in SquareLine */
    lv_obj_add_event_cb(ui_Panel1, ui_event_draw_board, LV_EVENT_DRAW_MAIN, NULL);
    
    /* Create a timer to tick the game and force a redraw */
    update_timer = lv_timer_create(game_step_timer_cb, 500, NULL);
}

void destroy_tetris_ui(void)
{
    lv_timer_del(update_timer);

    lv_mem_free(game);
    game = NULL;
}