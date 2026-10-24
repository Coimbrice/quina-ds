#include <string.h>

#include "daily.h"
#include "words.h"

// 2026-01-01, in days since 1970-01-01.
#define FIRST_DAY 20454
#define ANSWERS_PER_DAY 7

int daily_day(time_t now)
{
    int day = now / 86400 - FIRST_DAY;
    return day < 0 ? 0 : day;
}

void daily_answers(int day, int num_boards, char answers[][WORD_LEN])
{
    // Quina takes the day's first answer, Duque the next two and Quadra the
    // last four, so no word repeats across modes on the same day.
    int first = day * ANSWERS_PER_DAY + num_boards - 1;

    for (int b = 0; b < num_boards; b++)
        memcpy(answers[b], word_text(answer_order[(first + b) % answer_count]), WORD_LEN);
}
