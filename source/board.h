#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>

#include "game.h"
#include "gfx.h"

enum color hint_color(enum hint hint);

// Draws one tile. c is a Latin-1 character, or 0 for an empty tile. Tiles
// that are being typed are outlined instead of filled.
void draw_tile(enum screen s, int x, int y, int w, int h, enum font_size size, int c,
               enum color fill, bool outlined, bool cursor);

// Draws the boards on the top screen. typed holds the letters of the
// current guess, 0 where empty.
void board_draw(const struct game *g, const char typed[WORD_LEN], int cursor);

#endif
