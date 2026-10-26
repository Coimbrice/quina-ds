#include <string.h>

#include "game.h"

const int mode_boards[MODE_COUNT] = { 1, 2, 4 };
const char *const mode_names[MODE_COUNT] = { "Quina", "Duque", "Quadra" };

void evaluate_guess(const char *guess, const char *answer, uint8_t hints[WORD_LEN])
{
    int left[26] = { 0 };

    for (int i = 0; i < WORD_LEN; i++) {
        if (guess[i] == answer[i]) {
            hints[i] = HINT_CORRECT;
        } else {
            hints[i] = HINT_ABSENT;
            left[answer[i] - 'a']++;
        }
    }

    for (int i = 0; i < WORD_LEN; i++) {
        int c = guess[i] - 'a';
        if (hints[i] != HINT_CORRECT && left[c] > 0) {
            hints[i] = HINT_PRESENT;
            left[c]--;
        }
    }
}

int game_max_guesses(int num_boards)
{
    // 1, 2 and 4 boards get 6, 7 and 9 guesses.
    return num_boards + 5;
}

void game_start(struct game *g, int num_boards, const char answers[][WORD_LEN])
{
    memset(g, 0, sizeof(*g));
    g->num_boards = num_boards;
    g->max_guesses = game_max_guesses(num_boards);
    for (int b = 0; b < num_boards; b++) {
        memcpy(g->answers[b], answers[b], WORD_LEN);
        g->solved_at[b] = -1;
    }
}

void game_guess(struct game *g, const char *guess)
{
    if (game_over(g))
        return;

    memcpy(g->guesses[g->num_guesses], guess, WORD_LEN);
    for (int b = 0; b < g->num_boards; b++) {
        if (g->solved_at[b] < 0 && memcmp(guess, g->answers[b], WORD_LEN) == 0)
            g->solved_at[b] = g->num_guesses;
    }
    g->num_guesses++;
}

bool game_won(const struct game *g)
{
    for (int b = 0; b < g->num_boards; b++) {
        if (g->solved_at[b] < 0)
            return false;
    }
    return true;
}

bool game_over(const struct game *g)
{
    return game_won(g) || g->num_guesses == g->max_guesses;
}

int game_rows(const struct game *g, int board)
{
    if (g->solved_at[board] >= 0)
        return g->solved_at[board] + 1;
    return g->num_guesses;
}

void game_hints(const struct game *g, int board, int row, uint8_t hints[WORD_LEN])
{
    evaluate_guess(g->guesses[row], g->answers[board], hints);
}

enum hint game_key_hint(const struct game *g, int board, char letter)
{
    enum hint best = HINT_NONE;
    uint8_t hints[WORD_LEN];

    for (int row = 0; row < game_rows(g, board); row++) {
        game_hints(g, board, row, hints);
        for (int i = 0; i < WORD_LEN; i++) {
            if (g->guesses[row][i] == letter && hints[i] > best)
                best = hints[i];
        }
    }
    return best;
}
