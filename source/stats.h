#ifndef STATS_H
#define STATS_H

#include "save.h"

// Draws the stats on the top screen. last is the number of guesses of the
// game just won, 0 if it was lost or -1 for none, to highlight its bar.
void stats_draw(const struct stats *st, const char *title, int max_guesses, int last);

#endif
