#include <stdio.h>
#include <string.h>

#include "words.h"

static int failures;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond);                        \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

// These only check properties of the generated tables, so they keep
// passing when the word lists change.

static void test_find(void)
{
    for (int i = 0; i < word_count; i++) {
        if (word_find(word_text(i)) != i) {
            printf("word %d (%.5s) not found\n", i, word_text(i));
            failures++;
        }
        if (i > 0 && memcmp(word_text(i - 1), word_text(i), WORD_LEN) >= 0) {
            printf("words %d and %d out of order\n", i - 1, i);
            failures++;
        }
    }

    CHECK(word_find("aaaaa") == -1);
    CHECK(word_find("zzzzz") == -1);
}

static void test_answers(void)
{
    static char seen[0x10000];

    CHECK(answer_count > 0);
    for (int i = 0; i < answer_count; i++) {
        int index = answer_order[i];
        CHECK(index < word_count);
        CHECK(!seen[index]);
        seen[index] = 1;
    }
}

static void test_display(void)
{
    char text[WORD_LEN];

    for (int i = 0; i < word_count; i++) {
        word_display(i, text);
        // Only accented letters may differ from the plain word.
        for (int j = 0; j < WORD_LEN; j++) {
            if ((unsigned char)text[j] < 0x80)
                CHECK(text[j] == word_text(i)[j]);
        }
    }

    for (int i = 0; i < accented_count; i++) {
        word_display(accented_index[i], text);
        CHECK(memcmp(text, word_text(accented_index[i]), WORD_LEN) != 0);
    }
}

int main(void)
{
    test_find();
    test_answers();
    test_display();

    if (failures > 0) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("All word tests passed.\n");
    return 0;
}
