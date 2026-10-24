#ifndef DAILY_H
#define DAILY_H

#include <time.h>

#include "game.h"

// Day number of the daily challenge for a time read from the console clock.
int daily_day(time_t now);
// Answers of a day's challenge for a mode with 1, 2 or 4 boards.
void daily_answers(int day, int num_boards, char answers[][WORD_LEN]);

#endif
