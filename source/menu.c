#include <stdio.h>

#include <nds.h>

#include "board.h"
#include "gfx.h"
#include "menu.h"
#include "play.h"
#include "save.h"
#include "stats.h"

#define LOGO_TILE 44
#define LOGO_GAP 5
#define LOGO_Y 50

#define ITEM_X 38
#define ITEM_W 180
#define ITEM_H 30
#define ITEM_STEP 36
#define ITEM_GAP 6

#define CREDITS_Y 22
#define CREDITS_STEP 14

#define DIALOG_X 28
#define DIALOG_Y 46
#define DIALOG_W 200
#define DIALOG_H 100
#define CHOICE_W 76
#define CHOICE_H 26
#define CHOICE_Y (DIALOG_Y + DIALOG_H - CHOICE_H - 12)

#define CHECK_SIZE 20

#define BLINK_FRAMES 40

enum page {
    PAGE_TITLE,
    PAGE_MAIN,
    PAGE_OPTIONS,
    PAGE_ERASE,
    PAGE_STATS,
    PAGE_CREDITS,
};

static const char *const credits[] = {
    "Quina, por Coimbrice",
    "",
    "Inspirado em Termo (term.ooo),",
    "de Fernando Serboncini.",
    "Lista de palavras usada com",
    "permissão do autor.",
    "",
    "Fonte Mitr, de Cadson Demak (OFL).",
    "Feito com BlocksDS.",
};

// clang-format off
static const char *const check_icon[] = {
    "..........##",
    ".........###",
    "........###.",
    "##.....###..",
    "###...###...",
    ".###.###....",
    "..#####.....",
    "...###......",
};
// clang-format on

static enum page page;
static int focused;
static int frame;
static bool stats_daily = true;
static int stats_mode;

static void draw_logo(void)
{
    static const enum color colors[] = {
        COLOR_CORRECT, COLOR_PRESENT, COLOR_ABSENT, COLOR_CORRECT, COLOR_PRESENT,
    };
    int x = (SCREEN_W - 5 * LOGO_TILE - 4 * LOGO_GAP) / 2;

    gfx_clear(TOP, COLOR_BG);
    for (int i = 0; i < 5; i++) {
        draw_tile(TOP, x + i * (LOGO_TILE + LOGO_GAP), LOGO_Y, LOGO_TILE, LOGO_TILE, FONT_TITLE,
                  "QUINA"[i], colors[i], false, false);
    }
    gfx_text(TOP, SCREEN_W / 2, LOGO_Y + LOGO_TILE + 30, "Jogo de palavras", FONT_MEDIUM,
             COLOR_DIM_TEXT);
}

static int item_count(void)
{
    static const int counts[] = {
        [PAGE_MAIN] = 5, [PAGE_OPTIONS] = 4, [PAGE_ERASE] = 2, [PAGE_STATS] = 3, [PAGE_CREDITS] = 1,
    };
    return counts[page];
}

// Items are centered vertically on the bottom screen, except for the
// credits, where the only item goes below the text.
static int item_y(int i)
{
    if (page == PAGE_CREDITS)
        return SCREEN_H - ITEM_H - 8;
    return (SCREEN_H - item_count() * ITEM_STEP + (ITEM_STEP - ITEM_H)) / 2 + i * ITEM_STEP;
}

// The first two rows of the stats page split into one button per choice.
static int stats_choices(int row)
{
    return row == 0 ? 2 : row == 1 ? MODE_COUNT : 1;
}

static void stats_button(int row, int i, int *x, int *w)
{
    int n = stats_choices(row);
    *w = (ITEM_W - (n - 1) * ITEM_GAP) / n;
    *x = ITEM_X + i * (*w + ITEM_GAP);
}

static int choice_x(int i)
{
    return i == 0 ? DIALOG_X + 16 : DIALOG_X + DIALOG_W - 16 - CHOICE_W;
}

static void draw_erase(void)
{
    gfx_rect(BOTTOM, DIALOG_X, DIALOG_Y, DIALOG_W, DIALOG_H, COLOR_ABSENT);
    gfx_text(BOTTOM, SCREEN_W / 2, DIALOG_Y + 24, "Apagar estatísticas", FONT_MEDIUM, COLOR_TEXT);
    gfx_text(BOTTOM, SCREEN_W / 2, DIALOG_Y + 42, "e partidas salvas?", FONT_MEDIUM, COLOR_TEXT);
    gfx_button(BOTTOM, choice_x(0), CHOICE_Y, CHOICE_W, CHOICE_H, "Não", FONT_MEDIUM, COLOR_KEY,
               focused == 0);
    gfx_button(BOTTOM, choice_x(1), CHOICE_Y, CHOICE_W, CHOICE_H, "Sim", FONT_MEDIUM, COLOR_KEY,
               focused == 1);
}

static void draw_stats_page(void)
{
    static const char *const kinds[] = { "Diário", "Livre" };
    int x, w;

    for (int i = 0; i < 2; i++) {
        stats_button(0, i, &x, &w);
        gfx_button(BOTTOM, x, item_y(0), w, ITEM_H, kinds[i], FONT_MEDIUM,
                   stats_daily == (i == 0) ? COLOR_CORRECT : COLOR_KEY,
                   focused == 0 && stats_daily == (i == 0));
    }
    for (int m = 0; m < MODE_COUNT; m++) {
        stats_button(1, m, &x, &w);
        gfx_button(BOTTOM, x, item_y(1), w, ITEM_H, mode_names[m], FONT_MEDIUM,
                   stats_mode == m ? COLOR_CORRECT : COLOR_KEY, focused == 1 && stats_mode == m);
    }
    gfx_button(BOTTOM, ITEM_X, item_y(2), ITEM_W, ITEM_H, "Voltar", FONT_MEDIUM, COLOR_KEY,
               focused == 2);
}

static void draw_credits(void)
{
    for (unsigned i = 0; i < sizeof(credits) / sizeof(credits[0]); i++) {
        gfx_text(BOTTOM, SCREEN_W / 2, CREDITS_Y + i * CREDITS_STEP, credits[i],
                 i == 0 ? FONT_MEDIUM : FONT_SMALL, i == 0 ? COLOR_TEXT : COLOR_DIM_TEXT);
    }
    gfx_button(BOTTOM, ITEM_X, item_y(0), ITEM_W, ITEM_H, "Voltar", FONT_MEDIUM, COLOR_KEY, true);
}

static void draw_main(void)
{
    static char daily[32];
    int done = play_daily_done();

    // The daily button turns green once all of today's challenges are over.
    if (done > 0 && done < MODE_COUNT)
        snprintf(daily, sizeof(daily), "Desafio diário %d/%d", done, MODE_COUNT);
    else
        snprintf(daily, sizeof(daily), "Desafio diário");

    gfx_button(BOTTOM, ITEM_X, item_y(0), ITEM_W, ITEM_H, daily, FONT_MEDIUM,
               done == MODE_COUNT ? COLOR_CORRECT : COLOR_KEY, focused == 0);
    gfx_button(BOTTOM, ITEM_X, item_y(1), ITEM_W, ITEM_H, "Jogo livre", FONT_MEDIUM, COLOR_KEY,
               focused == 1);
    gfx_button(BOTTOM, ITEM_X, item_y(2), ITEM_W, ITEM_H, "Estatísticas", FONT_MEDIUM, COLOR_KEY,
               focused == 2);
    gfx_button(BOTTOM, ITEM_X, item_y(3), ITEM_W, ITEM_H, "Opções", FONT_MEDIUM, COLOR_KEY,
               focused == 3);
    gfx_button(BOTTOM, ITEM_X, item_y(4), ITEM_W, ITEM_H, "Créditos", FONT_MEDIUM, COLOR_KEY,
               focused == 4);
}

static void draw_checkbox(int i, const char *label, bool checked)
{
    int y = item_y(i);
    int box_x = ITEM_X + 8;
    int box_y = y + (ITEM_H - CHECK_SIZE) / 2;

    if (focused == i)
        gfx_frame(BOTTOM, ITEM_X, y, ITEM_W, ITEM_H, 2, COLOR_TEXT);
    gfx_rect(BOTTOM, box_x, box_y, CHECK_SIZE, CHECK_SIZE, checked ? COLOR_CORRECT : COLOR_KEY);
    if (checked)
        gfx_art(BOTTOM, box_x + 4, box_y + 6, check_icon, 8, 1, COLOR_TEXT);

    int text_x = box_x + CHECK_SIZE + 10;
    gfx_text(BOTTOM, text_x + gfx_text_width(label, FONT_MEDIUM) / 2,
             y + (ITEM_H + fonts[FONT_MEDIUM].cap_height) / 2, label, FONT_MEDIUM, COLOR_TEXT);
}

static void draw(void)
{
    gfx_clear(BOTTOM, COLOR_BG);

    if (page == PAGE_TITLE) {
        if (frame / BLINK_FRAMES % 2 == 0)
            gfx_text(BOTTOM, SCREEN_W / 2, 100, "Aperte START", FONT_LARGE, COLOR_TEXT);
    } else if (page == PAGE_MAIN) {
        draw_main();
    } else if (page == PAGE_OPTIONS) {
        draw_checkbox(0, "Efeitos sonoros", save.sound_effects);
        draw_checkbox(1, "Música", save.music);
        gfx_button(BOTTOM, ITEM_X, item_y(2), ITEM_W, ITEM_H, "Apagar dados", FONT_MEDIUM,
                   COLOR_KEY, focused == 2);
        gfx_button(BOTTOM, ITEM_X, item_y(3), ITEM_W, ITEM_H, "Voltar", FONT_MEDIUM, COLOR_KEY,
                   focused == 3);
    } else if (page == PAGE_ERASE) {
        draw_erase();
    } else if (page == PAGE_STATS) {
        draw_stats_page();
    } else {
        draw_credits();
    }
}

void menu_start(bool title)
{
    page = title ? PAGE_TITLE : PAGE_MAIN;
    focused = 0;
    frame = 0;
    draw_logo();
    draw();
}

static void go_to(enum page p, int item)
{
    if (p == PAGE_STATS)
        stats_draw(stats_daily, stats_mode, -1);
    else if (page == PAGE_STATS)
        draw_logo();
    page = p;
    focused = item;
}

// Changes the choice on a row of the stats page.
static void pick_stats(int row, int choice)
{
    if (row == 0)
        stats_daily = choice == 0;
    else
        stats_mode = choice;
    stats_draw(stats_daily, stats_mode, -1);
}

static void back(void)
{
    if (page == PAGE_OPTIONS)
        go_to(PAGE_MAIN, 3);
    else if (page == PAGE_ERASE)
        go_to(PAGE_OPTIONS, 2);
    else if (page == PAGE_STATS)
        go_to(PAGE_MAIN, 2);
    else if (page == PAGE_CREDITS)
        go_to(PAGE_MAIN, 4);
}

// Clears stats and games but keeps the sound options.
static void erase_data(void)
{
    uint8_t sound_effects = save.sound_effects;
    uint8_t music = save.music;

    save_defaults(&save);
    save.sound_effects = sound_effects;
    save.music = music;
    save_commit();
    play_forget();
}

static enum menu_choice activate(int item)
{
    switch (page) {
    case PAGE_MAIN:
        if (item == 0)
            return MENU_DAILY;
        if (item == 1)
            return MENU_FREE;
        if (item == 2)
            go_to(PAGE_STATS, 1);
        else if (item == 3)
            go_to(PAGE_OPTIONS, 0);
        else
            go_to(PAGE_CREDITS, 0);
        break;
    case PAGE_OPTIONS:
        if (item == 0) {
            save.sound_effects = !save.sound_effects;
            save_commit();
        } else if (item == 1) {
            save.music = !save.music;
            save_commit();
        } else if (item == 2) {
            go_to(PAGE_ERASE, 0);
        } else {
            back();
        }
        break;
    case PAGE_ERASE:
        if (item == 1)
            erase_data();
        back();
        break;
    case PAGE_STATS:
        // A cycles the choices on the first two rows.
        if (item == 0)
            pick_stats(0, stats_daily);
        else if (item == 1)
            pick_stats(1, (stats_mode + 1) % MODE_COUNT);
        else
            back();
        break;
    default:
        back();
        break;
    }
    return MENU_NONE;
}

static bool inside(const touchPosition *t, int x, int y, int w, int h)
{
    return t->px >= x && t->px < x + w && t->py >= y && t->py < y + h;
}

static enum menu_choice touch_item(void)
{
    touchPosition touch;

    touchRead(&touch);
    for (int i = 0; i < item_count(); i++) {
        if (page == PAGE_ERASE) {
            if (inside(&touch, choice_x(i), CHOICE_Y, CHOICE_W, CHOICE_H)) {
                focused = i;
                return activate(i);
            }
        } else if (page == PAGE_STATS && i < 2) {
            for (int c = 0; c < stats_choices(i); c++) {
                int x, w;
                stats_button(i, c, &x, &w);
                if (inside(&touch, x, item_y(i), w, ITEM_H)) {
                    focused = i;
                    pick_stats(i, c);
                }
            }
        } else if (inside(&touch, ITEM_X, item_y(i), ITEM_W, ITEM_H)) {
            focused = i;
            return activate(i);
        }
    }
    return MENU_NONE;
}

enum menu_choice menu_update(void)
{
    uint32_t down = keysDown();
    enum menu_choice choice = MENU_NONE;

    frame++;
    if (page == PAGE_TITLE) {
        if (down & (KEY_START | KEY_A | KEY_TOUCH)) {
            page = PAGE_MAIN;
            draw();
        } else if (frame % BLINK_FRAMES == 0) {
            draw();
        }
        return MENU_NONE;
    }

    if (!down)
        return MENU_NONE;

    // The erase dialog lays its two choices side by side.
    uint32_t prev_key = page == PAGE_ERASE ? KEY_LEFT : KEY_UP;
    uint32_t next_key = page == PAGE_ERASE ? KEY_RIGHT : KEY_DOWN;

    if (down & prev_key)
        focused = (focused + item_count() - 1) % item_count();
    if (down & next_key)
        focused = (focused + 1) % item_count();
    if (page == PAGE_STATS && focused < 2 && (down & (KEY_LEFT | KEY_RIGHT))) {
        int n = stats_choices(focused);
        int current = focused == 0 ? !stats_daily : stats_mode;
        pick_stats(focused, (current + ((down & KEY_LEFT) ? n - 1 : 1)) % n);
    }
    if (down & KEY_A)
        choice = activate(focused);
    if (down & KEY_B)
        back();
    if (down & KEY_TOUCH)
        choice = touch_item();

    draw();
    return choice;
}
