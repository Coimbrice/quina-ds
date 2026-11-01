#include <stdio.h>
#include <string.h>

#include "save.h"

static int failures;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond);                        \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static const char *path = "build/test/data/quina/save.dat";

static void test_round_trip(void)
{
    struct save written;
    struct save read;

    save_defaults(&written);
    written.music = 0;
    written.hard_mode = 1;
    written.last_mode[1] = MODE_QUADRA;
    written.stats[1][MODE_DUQUE].wins = 12;
    written.games[0][MODE_QUINA].started = 1;
    memcpy(written.games[0][MODE_QUINA].guesses[0], "carta", WORD_LEN);

    CHECK(save_write(path, &written));
    CHECK(save_read(path, &read));
    CHECK(memcmp(&written, &read, sizeof(read)) == 0);
}

static void test_bad_files(void)
{
    struct save defaults;
    struct save read;
    FILE *f;

    save_defaults(&defaults);

    CHECK(!save_read("build/test/missing.dat", &read));
    CHECK(memcmp(&read, &defaults, sizeof(read)) == 0);

    // Flip one byte of the data.
    save_write(path, &defaults);
    f = fopen(path, "r+b");
    CHECK(f);
    if (f) {
        fseek(f, -1, SEEK_END);
        fputc(0x55, f);
        fclose(f);
    }
    read.music = 0;
    CHECK(!save_read(path, &read));
    CHECK(memcmp(&read, &defaults, sizeof(read)) == 0);

    // Cut the file short.
    f = fopen(path, "wb");
    CHECK(f);
    if (f) {
        fputs("QUIN", f);
        fclose(f);
    }
    CHECK(!save_read(path, &read));
}

static void test_temp_file(void)
{
    char temp[280];
    struct save first;
    struct save second;
    struct save read;
    FILE *f;

    snprintf(temp, sizeof(temp), "%s.tmp", path);
    save_defaults(&first);
    first.last_mode[0] = MODE_DUQUE;
    second = first;
    second.last_mode[0] = MODE_QUADRA;

    // Saving over an existing file replaces it and leaves no temporary behind.
    CHECK(save_write(path, &first));
    CHECK(save_write(path, &second));
    CHECK(save_read(path, &read));
    CHECK(read.last_mode[0] == MODE_QUADRA);
    f = fopen(temp, "rb");
    CHECK(!f);
    if (f)
        fclose(f);

    // Losing power while writing the temporary keeps the last good save.
    f = fopen(temp, "wb");
    CHECK(f);
    if (f) {
        fputs("QUIN", f);
        fclose(f);
    }
    CHECK(save_read(path, &read));
    CHECK(read.last_mode[0] == MODE_QUADRA);

    // Losing power right after removing the old file keeps the new one.
    CHECK(rename(path, temp) == 0);
    CHECK(save_read(path, &read));
    CHECK(read.last_mode[0] == MODE_QUADRA);
    remove(temp);
}

static void test_stats(void)
{
    struct stats st = { 0 };

    stats_record(&st, false, 0, true, 3);
    stats_record(&st, false, 0, true, 4);
    stats_record(&st, false, 0, false, 6);
    stats_record(&st, false, 0, true, 3);
    CHECK(st.played == 4);
    CHECK(st.wins == 3);
    CHECK(st.streak == 1);
    CHECK(st.max_streak == 2);
    CHECK(st.guesses[2] == 2);
    CHECK(st.guesses[3] == 1);

    // Skipping a day breaks the daily streak even after a win.
    memset(&st, 0, sizeof(st));
    stats_record(&st, true, 10, true, 2);
    stats_record(&st, true, 11, true, 2);
    CHECK(st.streak == 2);
    stats_record(&st, true, 13, true, 2);
    CHECK(st.streak == 1);
    CHECK(st.max_streak == 2);
    CHECK(st.last_day == 13);

    memset(&st, 0, sizeof(st));
    for (int i = 0; i < 1200; i++)
        stats_record(&st, false, 0, true, 1);
    CHECK(st.played == STATS_MAX);
    CHECK(st.wins == STATS_MAX);
    CHECK(st.max_streak == STATS_MAX);
    CHECK(st.guesses[0] == STATS_MAX);
}

int main(void)
{
    test_round_trip();
    test_bad_files();
    test_temp_file();
    test_stats();

    if (failures > 0) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("All save tests passed.\n");
    return 0;
}
