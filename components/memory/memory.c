#include "memory.h"
#include <stdlib.h>
#include <string.h>

#define CARD_UNSELECTED 7

void memory_init(memory_game_t *game) {
    uint8_t indices[CARD_COLUMNS * CARD_ROWS];
    for (int i = 0; i < CARD_COLUMNS * CARD_ROWS; i++)
        indices[i] = i;
    for (int i = CARD_COLUMNS * CARD_ROWS - 1; i > 0; i--) {
        int j = rand() % i;
        uint8_t tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }
    for (int i = 0; i < CARD_COLUMNS * CARD_ROWS; i++) {
        int x = indices[i] % CARD_COLUMNS, y = indices[i] / CARD_COLUMNS;
        game->cards[x][y] = i / 2;
    }
    memset(game->revealed, 0, sizeof(game->revealed));
    memory_hide(game);
}

bool memory_is_revealed(memory_game_t *game, int x, int y) {
    if (game->revealed[x][y])
        return true;
    if (game->first_point.x == x && game->first_point.y == y)
        return true;
    return game->second_point.x == x && game->second_point.y == y;
}

bool memory_reveal(memory_game_t *game, card_point_t point) {
    if (game->revealed[point.x][point.y])
        return false;

    if (game->first_point.x == CARD_UNSELECTED)
        game->first_point = point;
    else if (game->second_point.x == CARD_UNSELECTED) {
        if (game->first_point.x == point.x && game->first_point.y == point.y)
            return false;
        if (game->cards[game->first_point.x][game->first_point.y] == game->cards[point.x][point.y]) {
            game->revealed[game->first_point.x][game->first_point.y] = true;
            game->revealed[point.x][point.y] = true;
            game->first_point.x = CARD_UNSELECTED;
            game->second_point.x = CARD_UNSELECTED;
        } else {
            game->second_point = point;
        }
    } else {
        game->first_point = point;
        game->second_point.x = CARD_UNSELECTED;
    }

    return true;
}

void memory_hide(memory_game_t *game) {
    game->first_point.x = CARD_UNSELECTED;
    game->second_point.x = CARD_UNSELECTED;
}