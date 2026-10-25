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
#define ITEM_H 34
#define ITEM_Y 30
#define ITEM_STEP 46
#define ITEM_COUNT 3

#define CHECK_SIZE 20

#define BLINK_FRAMES 40

enum page {
    PAGE_TITLE,
    PAGE_MAIN,
    PAGE_OPTIONS,
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

static int item_y(int i)
{
    return ITEM_Y + i * ITEM_STEP;
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
    } else {
        draw_checkbox(0, "Efeitos sonoros", save.sound_effects);
        draw_checkbox(1, "Música", save.music);
        gfx_button(BOTTOM, ITEM_X, item_y(2), ITEM_W, ITEM_H, "Voltar", FONT_MEDIUM, COLOR_KEY,
                   focused == 2);
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

static enum menu_choice activate(int item)
{
    if (page == PAGE_MAIN) {
        if (item == 0)
            return MENU_DAILY;
        if (item == 1)
            return MENU_FREE;
        page = PAGE_OPTIONS;
        focused = 0;
    } else if (item == 0) {
        save.sound_effects = !save.sound_effects;
        save_commit();
    } else if (item == 1) {
        save.music = !save.music;
        save_commit();
    } else {
        page = PAGE_MAIN;
        focused = 2;
    }
    return MENU_NONE;
}

static int item_hit(void)
{
    touchPosition touch;

    touchRead(&touch);
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (touch.px >= ITEM_X && touch.px < ITEM_X + ITEM_W && touch.py >= item_y(i) &&
            touch.py < item_y(i) + ITEM_H)
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

    if (down & KEY_UP)
        focused = (focused + ITEM_COUNT - 1) % ITEM_COUNT;
    if (down & KEY_DOWN)
        focused = (focused + 1) % ITEM_COUNT;
    if (down & KEY_A)
        choice = activate(focused);
    if ((down & KEY_B) && page == PAGE_OPTIONS) {
        page = PAGE_MAIN;
        focused = 2;
    }
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
