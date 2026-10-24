#include <stdio.h>
#include <string.h>

#include "daily.h"
#include "words.h"

static int failures;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond);                        \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static void test_day(void)
{
    CHECK(daily_day(0) == 0);
    CHECK(daily_day(20454L * 86400) == 0);
    CHECK(daily_day(20454L * 86400 + 86399) == 0);
    CHECK(daily_day(20455L * 86400) == 1);
    CHECK(daily_day(20819L * 86400) == 365);
}

// All the answers of a day, in mode order.
static void day_answers(int day, char answers[7][WORD_LEN])
{
    daily_answers(day, 1, &answers[0]);
    daily_answers(day, 2, &answers[1]);
    daily_answers(day, 4, &answers[3]);
}

static void test_answers(void)
{
    char a[7][WORD_LEN];
    char b[7][WORD_LEN];

    // No word repeats across modes on the same day.
    for (int day = 0; day < 1000; day++) {
        day_answers(day, a);
        for (int i = 0; i < 7; i++) {
            for (int j = i + 1; j < 7; j++)
                CHECK(memcmp(a[i], a[j], WORD_LEN) != 0);
        }
    }

    // Consecutive days don't share words while the list lasts.
    day_answers(0, a);
    day_answers(1, b);
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++)
            CHECK(memcmp(a[i], b[j], WORD_LEN) != 0);
    }
}

int main(void)
{
    test_day();
    test_answers();

    if (failures > 0) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("All daily tests passed.\n");
    return 0;
}
