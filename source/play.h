#ifndef PLAY_H
#define PLAY_H

#include <stdbool.h>

// Starts a new game and draws it.
void play_start(int num_boards);
// Handles one frame of input and redraws the screens if anything changed.
// Returns true when the player chose to go back to the menu.
bool play_update(void);

#endif
