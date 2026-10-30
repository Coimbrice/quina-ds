#ifndef SAVE_H
#define SAVE_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"

#define STATS_MAX 999

struct stats {
    uint16_t played;
    uint16_t wins;
    uint16_t streak;
    uint16_t max_streak;
    // Wins by number of guesses, starting from one guess.
    uint16_t guesses[MAX_GUESSES];
    // Day of the last daily challenge played, to break the streak when the
    // player skips a day.
    int32_t last_day;
};

// A game in progress is kept as its answers and guesses, and played again
// on load.
struct saved_game {
    uint8_t started;
    uint8_t num_guesses;
    int32_t day;
    char answers[MAX_BOARDS][WORD_LEN];
    char guesses[MAX_GUESSES][WORD_LEN];
};

// Arrays are indexed by daily (1) or free play (0), then by mode.
struct save {
    uint8_t sound_effects;
    uint8_t music;
    uint8_t high_contrast;
    uint8_t hard_mode;
    uint8_t last_mode[2];
    struct stats stats[2][MODE_COUNT];
    struct saved_game games[2][MODE_COUNT];
};

extern struct save save;

void save_defaults(struct save *s);
// These return false if the file can't be used. A missing, corrupted or
// outdated file leaves s with the defaults.
bool save_read(const char *path, struct save *s);
bool save_write(const char *path, const struct save *s);

// Loads the global save from path, or uses the defaults if path is NULL
// because there's no storage. save_commit writes it back to the same path.
void save_load(const char *path);
void save_commit(void);

void stats_record(struct stats *st, bool daily, int day, bool won, int guesses);

#endif
