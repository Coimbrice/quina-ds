#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "game.h"

#define KB_LEFT '\1'
#define KB_RIGHT '\2'
#define KB_ERASE '\b'
#define KB_ENTER '\n'

void keyboard_init(void);
// Draws the keys colored by the hints of g. focused is the highlighted key,
// or -1 for none.
void keyboard_draw(const struct game *g, int focused);

// Returns the key under a point, or -1.
int keyboard_hit(int x, int y);
// Returns the key next to k in a direction. Moving up and down goes to the
// key closest to where the player last moved sideways.
int keyboard_move(int k, int dx, int dy);
// Letter of a key, or one of the KB_ codes above.
char keyboard_char(int k);
// Key with a letter or KB_ code, or -1.
int keyboard_find(char c);

// Shows a key as pressed for a few frames. keyboard_animate advances the
// animation and returns true while the keyboard needs redrawing.
void keyboard_press(int k);
bool keyboard_animate(void);

#endif
