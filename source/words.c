#include <string.h>

#include "words.h"

int word_find(const char *word)
{
    int low = 0;
    int high = word_count - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = memcmp(word, word_text(mid), WORD_LEN);
        if (cmp == 0)
            return mid;
        if (cmp < 0)
            high = mid - 1;
        else
            low = mid + 1;
    }
    return -1;
}

const char *word_text(int index)
{
    return &word_list[index * WORD_LEN];
}

void word_display(int index, char out[WORD_LEN])
{
    int low = 0;
    int high = accented_count - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        if (accented_index[mid] == index) {
            memcpy(out, &accented_text[mid * WORD_LEN], WORD_LEN);
            return;
        }
        if (accented_index[mid] > index)
            high = mid - 1;
        else
            low = mid + 1;
    }
    memcpy(out, word_text(index), WORD_LEN);
}
