#ifndef STATS_H
#define STATS_H

#include "save.h"

// Draws the saved stats of a mode on the top screen. last is the number of
// guesses of the game just won, 0 if it was lost or -1 for none, to
// highlight its bar.
void stats_draw(bool daily, int mode, int last);

#endif
