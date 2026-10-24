#include "board.h"
#include "words.h"

struct layout {
    int tile_w;
    int tile_h;
    int gap;
    int board_gap;
    enum font_size font;
};

// Indexed by number of boards. Everything must fit in 256x192.
static const struct layout layouts[MAX_BOARDS + 1] = {
    [1] = { 28, 28, 3, 0, FONT_LARGE },
    [2] = { 22, 22, 2, 8, FONT_MEDIUM },
    [4] = { 11, 19, 1, 4, FONT_SMALL },
};

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
        // Small tiles get thin borders so the letter still fits.
        gfx_frame(s, x, y, w, h, w < 16 ? 1 : 2, COLOR_BORDER);
        if (cursor)
            gfx_rect(s, x, y + h - h / 6, w, h / 6, COLOR_BORDER);
    } else {
        gfx_rect(s, x, y, w, h, fill);
    }

    if (c)
        gfx_letter(s, x + w / 2, y + h / 2, c, size, COLOR_TEXT);
}

static void draw_row(const struct game *g, const struct layout *l, int board, int row, int x, int y,
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
        int tx = x + i * (l->tile_w + l->gap);
        if (row < game_rows(g, board))
            draw_tile(TOP, tx, y, l->tile_w, l->tile_h, l->font, (unsigned char)text[i],
                      hint_color(hints[i]), false, false);
        else if (typing)
            draw_tile(TOP, tx, y, l->tile_w, l->tile_h, l->font, typed[i], COLOR_BG, true,
                      i == cursor);
        else
            draw_tile(TOP, tx, y, l->tile_w, l->tile_h, l->font, 0, COLOR_EMPTY, false, false);
    }
}

void board_draw(const struct game *g, const char typed[WORD_LEN], int cursor)
{
    const struct layout *l = &layouts[g->num_boards];
    int board_w = WORD_LEN * l->tile_w + (WORD_LEN - 1) * l->gap;
    int width = g->num_boards * board_w + (g->num_boards - 1) * l->board_gap;
    int height = g->max_guesses * l->tile_h + (g->max_guesses - 1) * l->gap;
    int x = (SCREEN_W - width) / 2;
    int y = (SCREEN_H - height) / 2;

    gfx_clear(TOP, COLOR_BG);
    for (int b = 0; b < g->num_boards; b++) {
        int bx = x + b * (board_w + l->board_gap);
        for (int row = 0; row < g->max_guesses; row++)
            draw_row(g, l, b, row, bx, y + row * (l->tile_h + l->gap), typed, cursor);
    }
}
