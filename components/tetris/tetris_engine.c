#include "tetris_engine.h"
#include <string.h>
#include <stdlib.h>

// Tetromino definitions
const uint16_t shapes[7][4] = {
    {0x0F00, 0x4444, 0x0F00, 0x4444}, // I
    {0x8E00, 0x6440, 0x0E20, 0x44C0}, // J
    {0x2E00, 0x4460, 0x0E80, 0xC440}, // L
    {0xCC00, 0xCC00, 0xCC00, 0xCC00}, // O
    {0x6C00, 0x4620, 0x6C00, 0x4620}, // S
    {0x4E00, 0x4640, 0x0E40, 0x4C40}, // T
    {0xC600, 0x2640, 0xC600, 0x2640}  // Z
};

static bool check_collision(tetris_game_t *game, int x, int y, int rotation) {
    uint16_t shape = shapes[game->current_piece.type][rotation % 4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shape & (1 << (15 - (i * 4 + j)))) {
                int nx = x + j;
                int ny = y + i;
                if (nx < 0 || nx >= BOARD_WIDTH || ny >= BOARD_HEIGHT) return true;
                if (ny >= 0 && game->game_board[ny][nx]) return true;
            }
        }
    }
    return false;
}

static void spawn_piece(tetris_game_t *game) {
    game->current_piece.type = rand() % 7;
    game->current_piece.x = BOARD_WIDTH / 2 - 2;
    game->current_piece.y = 0;
    game->current_piece.rotation = 0;
    if (check_collision(game, game->current_piece.x, game->current_piece.y, game->current_piece.rotation)) {
        game->game_over = true;
    }
}

void tetris_init(tetris_game_t *game) {
    memset(game, 0, sizeof(tetris_game_t));
    spawn_piece(game);
}

static void lock_piece(tetris_game_t *game) {
    uint16_t shape = shapes[game->current_piece.type][game->current_piece.rotation % 4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shape & (1 << (15 - (i * 4 + j)))) {
                int ny = game->current_piece.y + i;
                if (ny >= 0) game->game_board[ny][game->current_piece.x + j] = game->current_piece.type + 1;
            }
        }
    }
    // Clear lines
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        bool full = true;
        for (int j = 0; j < BOARD_WIDTH; j++) if (!game->game_board[i][j]) full = false;
        if (full) {
            game->score += 100;
            for (int k = i; k > 0; k--) memcpy(game->game_board[k], game->game_board[k - 1], BOARD_WIDTH);
            memset(game->game_board[0], 0, BOARD_WIDTH);
            i++;
        }
    }
    spawn_piece(game);
}

bool tetris_tick(tetris_game_t *game) {
    if (game->game_over) return false;
    tetris_drop(game);
    return true;
}

void tetris_move(tetris_game_t *game, int dx) {
    if (!check_collision(game, game->current_piece.x + dx, game->current_piece.y, game->current_piece.rotation)) {
        game->current_piece.x += dx;
    }
}

void tetris_rotate(tetris_game_t *game) {
    int next_rot = (game->current_piece.rotation + 1) % 4;
    if (!check_collision(game, game->current_piece.x, game->current_piece.y, next_rot)) {
        game->current_piece.rotation = next_rot;
    }
}

void tetris_drop(tetris_game_t *game) {
    if (!check_collision(game, game->current_piece.x, game->current_piece.y + 1, game->current_piece.rotation)) {
        game->current_piece.y++;
    } else {
        lock_piece(game);
    }
}