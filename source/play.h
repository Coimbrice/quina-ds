#ifndef PLAY_H
#define PLAY_H

#include <stdbool.h>

// Opens the daily challenge or free play in the mode last played there, and
// draws it. Each mode keeps its game until it's over.
void play_open(bool daily);
// Handles one frame of input and redraws the screens if anything changed.
// Returns true when the player chose to go back to the menu.
bool play_update(void);
// How many of today's daily challenges are over.
int play_daily_done(void);

#endif
