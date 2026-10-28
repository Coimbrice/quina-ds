#include <stdio.h>
#include <string.h>

#include "game.h"

static int failures;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond);                        \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

// Hints as a string: G = correct, Y = present, . = absent.
static void check_eval(const char *guess, const char *answer, const char *expected)
{
    uint8_t hints[WORD_LEN];
    char got[WORD_LEN + 1] = { 0 };

    evaluate_guess(guess, answer, hints);
    for (int i = 0; i < WORD_LEN; i++)
        got[i] = hints[i] == HINT_CORRECT ? 'G' : hints[i] == HINT_PRESENT ? 'Y' : '.';

    if (strcmp(got, expected) != 0) {
        printf("evaluate %s against %s: expected %s, got %s\n", guess, answer, expected, got);
        failures++;
    }
}

static void test_evaluate(void)
{
    check_eval("carta", "carta", "GGGGG");
    check_eval("fuzil", "carta", ".....");
    check_eval("tarca", "carta", "YGGYG");

    // A repeated letter in the guess is yellow only as many times as the
    // answer still has it after the greens.
    check_eval("arara", "carta", "YY..G");
    check_eval("aaaaa", "sabia", ".G..G");
    check_eval("bobos", "pombo", "YG.Y.");
    check_eval("ossos", "posto", "Y.GY.");

    // A repeated letter in the answer can be yellow more than once.
    check_eval("carta", "arara", ".YY.G");
    check_eval("ralar", "arara", "YY.YY");
}

static void test_game(void)
{
    const char answers[][WORD_LEN] = { { 'c', 'a', 'r', 't', 'a' }, { 'p', 'o', 'm', 'b', 'o' } };
    struct game g;

    game_start(&g, 2, answers);
    CHECK(g.max_guesses == 7);
    CHECK(!game_over(&g));

    game_guess(&g, "bobos");
    game_guess(&g, "carta");
    CHECK(g.solved_at[0] == 1);
    CHECK(g.solved_at[1] == -1);
    CHECK(!game_won(&g));

    game_guess(&g, "fuzil");
    CHECK(game_rows(&g, 0) == 2);
    CHECK(game_rows(&g, 1) == 3);

    CHECK(game_key_hint(&g, 0, 'c') == HINT_CORRECT);
    CHECK(game_key_hint(&g, 0, 'b') == HINT_ABSENT);
    CHECK(game_key_hint(&g, 0, 'f') == HINT_NONE);
    CHECK(game_key_hint(&g, 1, 'o') == HINT_CORRECT);
    CHECK(game_key_hint(&g, 1, 'b') == HINT_PRESENT);
    CHECK(game_key_hint(&g, 1, 'f') == HINT_ABSENT);
    CHECK(game_key_hint(&g, 1, 'q') == HINT_NONE);

    game_guess(&g, "pombo");
    CHECK(game_won(&g));
    CHECK(game_over(&g));

    game_guess(&g, "fuzil");
    CHECK(g.num_guesses == 4);
}

static void test_game_lost(void)
{
    const char answers[][WORD_LEN] = { { 'c', 'a', 'r', 't', 'a' } };
    struct game g;

    game_start(&g, 1, answers);
    for (int i = 0; i < 5; i++)
        game_guess(&g, "fuzil");
    CHECK(!game_over(&g));

    game_guess(&g, "fuzil");
    CHECK(game_over(&g));
    CHECK(!game_won(&g));
}

static void test_hard_mode(void)
{
    const char answers[][WORD_LEN] = { { 'c', 'a', 'r', 't', 'a' }, { 'p', 'o', 'm', 'b', 'o' } };
    struct game g;
    int position = -1;
    char letter = 0;

    game_start(&g, 1, answers);
    CHECK(game_check_hard(&g, "fuzil", &position, &letter) == HARD_OK);

    // tarca against carta is YGGYG.
    game_guess(&g, "tarca");
    CHECK(game_check_hard(&g, "fuzil", &position, &letter) == HARD_POSITION);
    CHECK(position == 1 && letter == 'a');
    CHECK(game_check_hard(&g, "parda", &position, &letter) == HARD_MISSING);
    CHECK(letter == 'c' || letter == 't');
    CHECK(game_check_hard(&g, "carta", &position, &letter) == HARD_OK);
    CHECK(game_check_hard(&g, "tarca", &position, &letter) == HARD_OK);

    // A repeated letter has to show up as many times as it was found:
    // arara against carta is YY..G, so two a's and an r.
    game_start(&g, 1, answers);
    game_guess(&g, "arara");
    CHECK(game_check_hard(&g, "aorta", &position, &letter) == HARD_OK);
    CHECK(game_check_hard(&g, "terra", &position, &letter) == HARD_MISSING);
    CHECK(letter == 'a');

    // Solved boards don't constrain the next guesses.
    game_start(&g, 2, answers);
    game_guess(&g, "carta");
    CHECK(game_check_hard(&g, "fuzil", &position, &letter) == HARD_OK);
}

int main(void)
{
    test_evaluate();
    test_game();
    test_game_lost();
    test_hard_mode();

    if (failures > 0) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("All tests passed.\n");
    return 0;
}
