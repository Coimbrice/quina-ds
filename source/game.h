#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

#define WORD_LEN 5
#define MAX_BOARDS 4
#define MAX_GUESSES 9

enum mode {
    MODE_QUINA,
    MODE_DUQUE,
    MODE_QUADRA,
    MODE_COUNT,
};

// Ordered so that the best hint for a key is the highest value.
enum hint {
    HINT_NONE,
    HINT_ABSENT,
    HINT_PRESENT,
    HINT_CORRECT,
};

// Words are WORD_LEN lowercase ASCII letters, not null-terminated.
struct game {
    int num_boards;
    int max_guesses;
    int num_guesses;
    char answers[MAX_BOARDS][WORD_LEN];
    char guesses[MAX_GUESSES][WORD_LEN];
    int8_t solved_at[MAX_BOARDS];
};

extern const int mode_boards[MODE_COUNT];

void evaluate_guess(const char *guess, const char *answer, uint8_t hints[WORD_LEN]);

void game_start(struct game *g, int num_boards, const char answers[][WORD_LEN]);
void game_guess(struct game *g, const char *guess);
bool game_won(const struct game *g);
bool game_over(const struct game *g);

// Number of guess rows shown on a board: rows after it was solved stay empty.
int game_rows(const struct game *g, int board);
void game_hints(const struct game *g, int board, int row, uint8_t hints[WORD_LEN]);
enum hint game_key_hint(const struct game *g, int board, char letter);

#endif
