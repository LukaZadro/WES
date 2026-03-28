#ifndef TETRIS_ENGINE_H
#define TETRIS_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

// Tetromino definitions (simplified)
extern const uint16_t shapes[7][4];

typedef struct {
    int x, y;
    int type;
    int rotation;
} piece_t;

typedef struct {
    uint8_t game_board[BOARD_HEIGHT][BOARD_WIDTH];
    piece_t current_piece;
    int score;
    bool game_over;
} tetris_game_t;

void tetris_init(tetris_game_t *game);
bool tetris_tick(tetris_game_t *game); // Returns true if board changed
void tetris_move(tetris_game_t *game, int dx);
void tetris_rotate(tetris_game_t *game);
void tetris_drop(tetris_game_t *game);

#endif