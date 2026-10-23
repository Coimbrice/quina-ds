#include "board.h"
#include "words.h"

#define TILE 28
#define TILE_GAP 3
#define TILE_FONT FONT_LARGE

enum color hint_color(enum hint hint)
{
    switch (hint) {
    case HINT_CORRECT:
        return COLOR_CORRECT;
    case HINT_PRESENT:
        return COLOR_PRESENT;
    case HINT_ABSENT:
        return COLOR_ABSENT;
    default:
        return COLOR_EMPTY;
    }
}

void draw_tile(enum screen s, int x, int y, int w, int h, enum font_size size, int c,
               enum color fill, bool outlined, bool cursor)
{
    if (outlined) {
        gfx_frame(s, x, y, w, h, 2, COLOR_BORDER);
        if (cursor)
            gfx_rect(s, x, y + h - 5, w, 5, COLOR_BORDER);
    } else {
        gfx_rect(s, x, y, w, h, fill);
    }

    if (c)
        gfx_letter(s, x + w / 2, y + h / 2, c, size, COLOR_TEXT);
}

static void draw_row(const struct game *g, int board, int row, int x, int y,
                     const char typed[WORD_LEN], int cursor)
{
    uint8_t hints[WORD_LEN];
    char text[WORD_LEN];
    bool typing = row == g->num_guesses && g->solved_at[board] < 0 && !game_over(g);

    if (row < game_rows(g, board)) {
        game_hints(g, board, row, hints);
        word_display(word_find(g->guesses[row]), text);
    }

    for (int i = 0; i < WORD_LEN; i++) {
        int tx = x + i * (TILE + TILE_GAP);
        if (row < game_rows(g, board))
            draw_tile(TOP, tx, y, TILE, TILE, TILE_FONT, (unsigned char)text[i],
                      hint_color(hints[i]), false, false);
        else if (typing)
            draw_tile(TOP, tx, y, TILE, TILE, TILE_FONT, typed[i], COLOR_BG, true, i == cursor);
        else
            draw_tile(TOP, tx, y, TILE, TILE, TILE_FONT, 0, COLOR_EMPTY, false, false);
    }
}

void board_draw(const struct game *g, const char typed[WORD_LEN], int cursor)
{
    int width = WORD_LEN * TILE + (WORD_LEN - 1) * TILE_GAP;
    int height = g->max_guesses * TILE + (g->max_guesses - 1) * TILE_GAP;
    int x = (SCREEN_W - width) / 2;
    int y = (SCREEN_H - height) / 2;

    gfx_clear(TOP, COLOR_BG);
    for (int row = 0; row < g->max_guesses; row++)
        draw_row(g, 0, row, x, y + row * (TILE + TILE_GAP), typed, cursor);
}
