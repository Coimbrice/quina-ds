#include <string.h>

#include "daily.h"
#include "words.h"

#define ANSWERS_PER_DAY 7

// Days since 1970, so every date a console clock can show is a different day,
// even when it's set wrong.
int daily_day(time_t now)
{
    return now / 86400;
}

void daily_answers(int day, int num_boards, char answers[][WORD_LEN])
{
    // Quina takes the day's first answer, Duque the next two and Quadra the
    // last four, so no word repeats across modes on the same day.
    int first = day * ANSWERS_PER_DAY + num_boards - 1;

    for (int b = 0; b < num_boards; b++)
        memcpy(answers[b], word_text(answer_order[(first + b) % answer_count]), WORD_LEN);
}
