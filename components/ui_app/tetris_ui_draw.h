#ifndef TETRIS_UI_DRAW_H
#define TETRIS_UI_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void setup_tetris_ui(void);

void destroy_tetris_ui(void);

typedef enum {
    tetris_move_left, tetris_move_right,
    tetris_soft_drop, tetris_hard_drop,
    tetris_rotate_left, tetris_rotate_right,
} movement_t;

bool tetris_movement(movement_t movement);

#ifdef __cplusplus
}
#endif

#endif // TETRIS_UI_DRAW_H