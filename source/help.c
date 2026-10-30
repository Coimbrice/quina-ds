#include "board.h"
#include "gfx.h"
#include "help.h"

#define TILE 18
#define TILE_GAP 2
#define EXAMPLE_Y 44
#define EXAMPLE_STEP 42

static const struct {
    const char *word;
    int highlight;
    enum color color;
    const char *text;
} examples[] = {
    { "TURMA", 0, COLOR_CORRECT, "T está na palavra e na posição certa." },
    { "VIOLA", 2, COLOR_PRESENT, "O está na palavra, mas em outra posição." },
    { "PULGA", 3, COLOR_ABSENT, "G não está na palavra." },
};

void help_draw(void)
{
    int x0 = (SCREEN_W - WORD_LEN * TILE - (WORD_LEN - 1) * TILE_GAP) / 2;

    gfx_clear(TOP, COLOR_BG);
    gfx_text(TOP, SCREEN_W / 2, 16, "Como jogar", FONT_MEDIUM, COLOR_TEXT);
    gfx_text(TOP, SCREEN_W / 2, 32, "Descubra a palavra. As cores mostram", FONT_SMALL,
             COLOR_DIM_TEXT);
    gfx_text(TOP, SCREEN_W / 2, 42, "o quanto cada chute chegou perto.", FONT_SMALL,
             COLOR_DIM_TEXT);

    for (int e = 0; e < 3; e++) {
        int y = EXAMPLE_Y + 8 + e * EXAMPLE_STEP;
        for (int i = 0; i < WORD_LEN; i++) {
            bool lit = i == examples[e].highlight;
            draw_tile(TOP, x0 + i * (TILE + TILE_GAP), y, TILE, TILE, FONT_MEDIUM,
                      examples[e].word[i], lit ? examples[e].color : COLOR_BG, !lit, false);
        }
        gfx_text(TOP, SCREEN_W / 2, y + TILE + 12, examples[e].text, FONT_SMALL, COLOR_TEXT);
    }

    gfx_text(TOP, SCREEN_W / 2, SCREEN_H - 8, "Os acentos aparecem sozinhos.", FONT_SMALL,
             COLOR_DIM_TEXT);
}

void help_draw_hard_mode(void)
{
    static const char *const before[] = {
        "As dicas reveladas têm que ser usadas",
        "nos chutes seguintes.",
    };
    static const char *const after[] = {
        "Depois deste chute, o próximo tem que ter",
        "T na primeira posição e um R em qualquer",
        "lugar, senão ele não é aceito.",
        "",
        "No Duque e na Quadra, só contam",
        "as grades ainda não resolvidas.",
    };
    int x0 = (SCREEN_W - WORD_LEN * TILE - (WORD_LEN - 1) * TILE_GAP) / 2;

    gfx_clear(TOP, COLOR_BG);
    gfx_text(TOP, SCREEN_W / 2, 18, "Modo difícil", FONT_MEDIUM, COLOR_TEXT);
    for (int i = 0; i < 2; i++)
        gfx_text(TOP, SCREEN_W / 2, 38 + i * 11, before[i], FONT_SMALL, COLOR_TEXT);

    for (int i = 0; i < WORD_LEN; i++) {
        enum color fill = i == 0 ? COLOR_CORRECT : i == 2 ? COLOR_PRESENT : COLOR_ABSENT;
        draw_tile(TOP, x0 + i * (TILE + TILE_GAP), 66, TILE, TILE, FONT_MEDIUM, "TURMA"[i], fill,
                  false, false);
    }

    for (int i = 0; i < 6; i++)
        gfx_text(TOP, SCREEN_W / 2, 108 + i * 11, after[i], FONT_SMALL,
                 i < 3 ? COLOR_TEXT : COLOR_DIM_TEXT);
}
