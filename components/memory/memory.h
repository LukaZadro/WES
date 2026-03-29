#ifndef MEMORY_ENGINE_H
#define MEMORY_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#define CARD_COLUMNS 4
#define CARD_ROWS 3

typedef struct {
    uint8_t x: 3, y: 3;
} card_point_t;

typedef struct {
    uint8_t cards[CARD_COLUMNS][CARD_ROWS];
    bool revealed[CARD_COLUMNS][CARD_ROWS];
    card_point_t first_point;
    card_point_t second_point;
} memory_game_t;

void memory_init(memory_game_t *game);
bool memory_is_revealed(memory_game_t *game, int x, int y);
bool memory_reveal(memory_game_t *game, card_point_t point);
void memory_hide(memory_game_t *game);

#endif