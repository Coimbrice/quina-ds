#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "game.h"

#define KB_ERASE '\b'
#define KB_ENTER '\n'

void keyboard_init(void);
// Draws the keys colored by the hints of g. focused is the highlighted key,
// or -1 for none.
void keyboard_draw(const struct game *g, int focused);

// Returns the key under a point, or -1.
int keyboard_hit(int x, int y);
// Returns the key next to k in a direction. Moving up and down keeps the
// column the player was on, even when passing through the wide ENTER key.
int keyboard_move(int k, int dx, int dy);
// Letter of a key, or KB_ERASE or KB_ENTER.
char keyboard_char(int k);

#endif
