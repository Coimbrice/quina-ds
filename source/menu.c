#include <nds.h>

#include "board.h"
#include "gfx.h"
#include "menu.h"

#define LOGO_TILE 44
#define LOGO_GAP 5
#define LOGO_Y 50

#define BUTTON_X 38
#define BUTTON_W 180
#define BUTTON_H 34
#define BUTTON_Y 30
#define BUTTON_STEP 46
#define BUTTON_COUNT 3

#define BLINK_FRAMES 40

enum page {
    PAGE_TITLE,
    PAGE_MAIN,
    PAGE_OPTIONS,
};

bool sound_effects_on = true;
bool music_on = true;

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

static void button_labels(const char *labels[BUTTON_COUNT])
{
    if (page == PAGE_MAIN) {
        labels[0] = "Desafio diário";
        labels[1] = "Jogo livre";
        labels[2] = "Opções";
    } else {
        labels[0] = sound_effects_on ? "Efeitos sonoros: sim" : "Efeitos sonoros: não";
        labels[1] = music_on ? "Música: sim" : "Música: não";
        labels[2] = "Voltar";
    }
}

static void draw(void)
{
    const char *labels[BUTTON_COUNT];

    gfx_clear(BOTTOM, COLOR_BG);
    if (page == PAGE_TITLE) {
        if (frame / BLINK_FRAMES % 2 == 0)
            gfx_text(BOTTOM, SCREEN_W / 2, 100, "Aperte START", FONT_LARGE, COLOR_TEXT);
        return;
    }

    button_labels(labels);
    for (int i = 0; i < BUTTON_COUNT; i++) {
        gfx_button(BOTTOM, BUTTON_X, BUTTON_Y + i * BUTTON_STEP, BUTTON_W, BUTTON_H, labels[i],
                   FONT_MEDIUM, COLOR_KEY, i == focused);
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

static enum menu_choice activate(int button)
{
    if (page == PAGE_MAIN) {
        if (button == 0)
            return MENU_DAILY;
        if (button == 1)
            return MENU_FREE;
        page = PAGE_OPTIONS;
        focused = 0;
    } else if (button == 0) {
        sound_effects_on = !sound_effects_on;
    } else if (button == 1) {
        music_on = !music_on;
    } else {
        page = PAGE_MAIN;
        focused = 2;
    }
    return MENU_NONE;
}

static int button_hit(void)
{
    touchPosition touch;

    touchRead(&touch);
    for (int i = 0; i < BUTTON_COUNT; i++) {
        int y = BUTTON_Y + i * BUTTON_STEP;
        if (touch.px >= BUTTON_X && touch.px < BUTTON_X + BUTTON_W && touch.py >= y &&
            touch.py < y + BUTTON_H)
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
        focused = (focused + BUTTON_COUNT - 1) % BUTTON_COUNT;
    if (down & KEY_DOWN)
        focused = (focused + 1) % BUTTON_COUNT;
    if (down & KEY_A)
        choice = activate(focused);
    if ((down & KEY_B) && page == PAGE_OPTIONS) {
        page = PAGE_MAIN;
        focused = 2;
    }
    if (down & KEY_TOUCH) {
        int button = button_hit();
        if (button >= 0) {
            focused = button;
            choice = activate(button);
        }
    }

    draw();
    return choice;
}
