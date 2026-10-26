#include <stdio.h>
#include <string.h>

#include "gfx.h"
#include "stats.h"

#define NUMBERS_Y 50
#define BARS_Y 74
#define BARS_H (SCREEN_H - BARS_Y - 4)
#define BAR_X 26
#define BAR_MAX_W (SCREEN_W - BAR_X - 10)
#define BAR_MIN_W 16

static void draw_number(int x, int value, const char *label)
{
    char text[8];

    snprintf(text, sizeof(text), "%d", value);
    gfx_text(TOP, x, NUMBERS_Y, text, FONT_LARGE, COLOR_TEXT);
    gfx_text(TOP, x, NUMBERS_Y + 14, label, FONT_SMALL, COLOR_DIM_TEXT);
}

static void draw_bar(int row, int rows, const char *label, int count, int most, bool highlight,
                     enum color color)
{
    char text[8];
    int step = BARS_H / rows;
    int y = BARS_Y + row * step;
    int w = BAR_MIN_W + (most ? count * (BAR_MAX_W - BAR_MIN_W) / most : 0);
    int baseline = y + (step - 2 + fonts[FONT_SMALL].cap_height) / 2;

    gfx_text(TOP, BAR_X / 2, baseline, label, FONT_SMALL, COLOR_DIM_TEXT);
    gfx_rect(TOP, BAR_X, y, w, step - 2, highlight ? color : COLOR_ABSENT);
    snprintf(text, sizeof(text), "%d", count);
    gfx_text(TOP, BAR_X + w - 8, baseline, text, FONT_SMALL, COLOR_TEXT);
}

void stats_draw(bool daily, int mode, int last)
{
    const struct stats *st = &save.stats[daily][mode];
    int max_guesses = game_max_guesses(mode_boards[mode]);
    int losses = st->played - st->wins;
    char title[32];
    int most = losses;
    char label[4];

    for (int i = 0; i < max_guesses; i++) {
        if (st->guesses[i] > most)
            most = st->guesses[i];
    }

    strcpy(title, mode_names[mode]);
    strcat(title, daily ? ", desafio diário" : ", jogo livre");

    gfx_clear(TOP, COLOR_BG);
    gfx_text(TOP, SCREEN_W / 2, 18, title, FONT_MEDIUM, COLOR_TEXT);

    draw_number(32, st->played, "jogos");
    draw_number(96, st->played ? st->wins * 100 / st->played : 0, "% vitórias");
    draw_number(160, st->streak, "sequência");
    draw_number(224, st->max_streak, "melhor");

    for (int i = 0; i < max_guesses; i++) {
        snprintf(label, sizeof(label), "%d", i + 1);
        draw_bar(i, max_guesses + 1, label, st->guesses[i], most, last == i + 1, COLOR_CORRECT);
    }
    draw_bar(max_guesses, max_guesses + 1, "X", losses, most, last == 0, COLOR_PRESENT);
}
