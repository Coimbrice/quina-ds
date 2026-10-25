#include <stdio.h>

#include <nds.h>

#include "board.h"
#include "gfx.h"
#include "menu.h"
#include "play.h"
#include "save.h"

#define LOGO_TILE 44
#define LOGO_GAP 5
#define LOGO_Y 50

#define ITEM_X 38
#define ITEM_W 180
#define ITEM_H 32
#define ITEM_STEP 40

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
    return page == PAGE_OPTIONS ? 4 : page == PAGE_ERASE ? 2 : 3;
}

// Items are centered vertically on the bottom screen.
static int item_y(int i)
{
    return (SCREEN_H - item_count() * ITEM_STEP + (ITEM_STEP - ITEM_H)) / 2 + i * ITEM_STEP;
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
    gfx_button(BOTTOM, ITEM_X, item_y(2), ITEM_W, ITEM_H, "Opções", FONT_MEDIUM, COLOR_KEY,
               focused == 2);
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
    } else {
        draw_erase();
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
    page = p;
    focused = item;
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
        go_to(PAGE_OPTIONS, 0);
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
            go_to(PAGE_MAIN, 2);
        }
        break;
    case PAGE_ERASE:
        if (item == 1)
            erase_data();
        go_to(PAGE_OPTIONS, 2);
        break;
    default:
        break;
    }
    return MENU_NONE;
}

static int item_hit(void)
{
    touchPosition touch;

    touchRead(&touch);
    for (int i = 0; i < item_count(); i++) {
        int x = page == PAGE_ERASE ? choice_x(i) : ITEM_X;
        int y = page == PAGE_ERASE ? CHOICE_Y : item_y(i);
        int w = page == PAGE_ERASE ? CHOICE_W : ITEM_W;
        int h = page == PAGE_ERASE ? CHOICE_H : ITEM_H;
        if (touch.px >= x && touch.px < x + w && touch.py >= y && touch.py < y + h)
            return i;
    }
    return -1;
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
    uint32_t back = page == PAGE_ERASE ? KEY_LEFT : KEY_UP;
    uint32_t next = page == PAGE_ERASE ? KEY_RIGHT : KEY_DOWN;

    if (down & back)
        focused = (focused + item_count() - 1) % item_count();
    if (down & next)
        focused = (focused + 1) % item_count();
    if (down & KEY_A)
        choice = activate(focused);
    if ((down & KEY_B) && page == PAGE_OPTIONS)
        go_to(PAGE_MAIN, 2);
    else if ((down & KEY_B) && page == PAGE_ERASE)
        go_to(PAGE_OPTIONS, 2);
    if (down & KEY_TOUCH) {
        int item = item_hit();
        if (item >= 0) {
            focused = item;
            choice = activate(item);
        }
    }

    draw();
    return choice;
}
