#include <stdlib.h>
#include <string.h>

#include <nds.h>

#include "board.h"
#include "game.h"
#include "gfx.h"
#include "keyboard.h"
#include "play.h"
#include "words.h"

#define HEADER_Y 3
#define HEADER_H 18
#define MENU_X 4
#define MENU_W 52
#define PROMPT_TILE 28
#define PROMPT_GAP 4
#define PROMPT_X ((SCREEN_W - WORD_LEN * PROMPT_TILE - (WORD_LEN - 1) * PROMPT_GAP) / 2)
#define PROMPT_Y 25
#define MESSAGE_Y 66
#define MESSAGE_FRAMES 150

#define DIALOG_X 28
#define DIALOG_Y 56
#define DIALOG_W 200
#define DIALOG_H 84
#define CHOICE_W 76
#define CHOICE_H 26
#define CHOICE_Y (DIALOG_Y + 46)
#define NO_X (DIALOG_X + 16)
#define YES_X (DIALOG_X + DIALOG_W - 16 - CHOICE_W)

static const char *const mode_names[MAX_BOARDS + 1] = {
    [1] = "Quina",
    [2] = "Duque",
    [4] = "Quadra",
};

static struct game game;
static char typed[WORD_LEN];
static int cursor;
static bool focus_keys;
static int focused_key;
static const char *message;
static int message_timer;
static bool confirming;
static bool confirm_yes;
static bool dirty;

static void draw(void);

static void show_message(const char *text)
{
    message = text;
    message_timer = MESSAGE_FRAMES;
}

void play_start(int num_boards)
{
    char answers[MAX_BOARDS][WORD_LEN];

    for (int b = 0; b < num_boards; b++) {
        bool repeated;
        do {
            memcpy(answers[b], word_text(answer_order[rand() % answer_count]), WORD_LEN);
            repeated = false;
            for (int other = 0; other < b; other++)
                repeated |= memcmp(answers[b], answers[other], WORD_LEN) == 0;
        } while (repeated);
    }

    game_start(&game, num_boards, answers);
    memset(typed, 0, sizeof(typed));
    cursor = 0;
    focus_keys = true;
    focused_key = 0;
    message = NULL;
    confirming = false;
    draw();
}

static void type_letter(char c)
{
    typed[cursor] = c;
    if (cursor < WORD_LEN - 1)
        cursor++;
}

static void erase(void)
{
    if (!typed[cursor] && cursor > 0)
        cursor--;
    typed[cursor] = 0;
}

static void submit(void)
{
    if (memchr(typed, 0, WORD_LEN)) {
        show_message("Só palavras com 5 letras");
        return;
    }

    int index = word_find(typed);
    if (index < 0) {
        show_message("Essa palavra não é aceita");
        return;
    }

    game_guess(&game, word_text(index));
    memset(typed, 0, sizeof(typed));
    cursor = 0;
    message = NULL;
}

static void press(char c)
{
    if (c == KB_ENTER)
        submit();
    else if (c == KB_ERASE)
        erase();
    else if (c == KB_LEFT && cursor > 0)
        cursor--;
    else if (c == KB_RIGHT && cursor < WORD_LEN - 1)
        cursor++;
    else if (c >= 'a' && c <= 'z')
        type_letter(c);
}

static bool inside(int px, int py, int x, int y, int w, int h)
{
    return px >= x && px < x + w && py >= y && py < y + h;
}

static void handle_touch(int x, int y)
{
    if (inside(x, y, PROMPT_X, PROMPT_Y, SCREEN_W - 2 * PROMPT_X, PROMPT_TILE)) {
        int box = (x - PROMPT_X) / (PROMPT_TILE + PROMPT_GAP);
        if (box < WORD_LEN) {
            cursor = box;
            focus_keys = false;
        }
        return;
    }

    int k = keyboard_hit(x, y);
    if (k >= 0) {
        focus_keys = false;
        press(keyboard_char(k));
    }
}

static void handle_buttons(uint32_t down)
{
    if (down & (KEY_B | KEY_L))
        erase();
    if (down & KEY_R)
        submit();

    if (focus_keys) {
        if (down & KEY_A)
            press(keyboard_char(focused_key));
        if (down & KEY_LEFT)
            focused_key = keyboard_move(focused_key, -1, 0);
        if (down & KEY_RIGHT)
            focused_key = keyboard_move(focused_key, 1, 0);
        if (down & KEY_UP)
            focused_key = keyboard_move(focused_key, 0, -1);
        if (down & KEY_DOWN)
            focused_key = keyboard_move(focused_key, 0, 1);
    } else {
        if (down & KEY_A)
            submit();
        if ((down & KEY_LEFT) && cursor > 0)
            cursor--;
        if ((down & KEY_RIGHT) && cursor < WORD_LEN - 1)
            cursor++;
        if (down & (KEY_UP | KEY_DOWN))
            focus_keys = true;
    }
}

// Returns true when the player confirms going back to the menu.
static bool handle_confirm(uint32_t down, int x, int y)
{
    if (down & KEY_TOUCH) {
        if (inside(x, y, YES_X, CHOICE_Y, CHOICE_W, CHOICE_H))
            return true;
        if (inside(x, y, NO_X, CHOICE_Y, CHOICE_W, CHOICE_H))
            confirming = false;
    }
    if (down & (KEY_LEFT | KEY_RIGHT))
        confirm_yes = !confirm_yes;
    if (down & KEY_A) {
        if (confirm_yes)
            return true;
        confirming = false;
    }
    if (down & (KEY_B | KEY_START))
        confirming = false;
    return false;
}

static void draw_header(void)
{
    const char *name = mode_names[game.num_boards];

    gfx_button(BOTTOM, MENU_X, HEADER_Y, MENU_W, HEADER_H, "Menu", FONT_SMALL, false);
    gfx_text(BOTTOM, SCREEN_W - 6 - gfx_text_width(name, FONT_MEDIUM) / 2,
             HEADER_Y + (HEADER_H + fonts[FONT_MEDIUM].cap_height) / 2, name, FONT_MEDIUM,
             COLOR_TEXT);
}

static void draw_prompt(void)
{
    for (int i = 0; i < WORD_LEN; i++) {
        int x = PROMPT_X + i * (PROMPT_TILE + PROMPT_GAP);
        bool selected = i == cursor;
        draw_tile(BOTTOM, x, PROMPT_Y, PROMPT_TILE, PROMPT_TILE, FONT_LARGE, typed[i], COLOR_BG,
                  true, selected);
        if (selected && !focus_keys)
            gfx_frame(BOTTOM, x, PROMPT_Y, PROMPT_TILE, PROMPT_TILE, 2, COLOR_TEXT);
    }
}

// Writes the answers as UTF-8, separated by spaces. Display forms are
// Latin-1 but the text functions take UTF-8.
static void answers_text(char *out)
{
    char word[WORD_LEN];

    for (int b = 0; b < game.num_boards; b++) {
        if (b > 0)
            *out++ = ' ';
        word_display(word_find(game.answers[b]), word);
        for (int i = 0; i < WORD_LEN; i++) {
            unsigned char c = word[i];
            if (c >= 0x80) {
                *out++ = 0xC0 | (c >> 6);
                *out++ = 0x80 | (c & 0x3F);
            } else {
                *out++ = c;
            }
        }
    }
    *out = 0;
}

static void draw_result(void)
{
    if (game_won(&game)) {
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 21, "Parabéns!", FONT_LARGE, COLOR_TEXT);
    } else {
        char text[MAX_BOARDS * (WORD_LEN * 2 + 1)];
        bool one = game.num_boards == 1;

        answers_text(text);
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 6, one ? "A palavra era" : "As palavras eram",
                 FONT_SMALL, COLOR_DIM_TEXT);
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 26, text,
                 game.num_boards == 4 ? FONT_MEDIUM : FONT_LARGE, COLOR_TEXT);
    }
    gfx_text(BOTTOM, SCREEN_W / 2, MESSAGE_Y, "A: jogar de novo", FONT_MEDIUM, COLOR_DIM_TEXT);
}

static void draw_confirm(void)
{
    gfx_rect(BOTTOM, DIALOG_X - 2, DIALOG_Y - 2, DIALOG_W + 4, DIALOG_H + 4, COLOR_BG);
    gfx_rect(BOTTOM, DIALOG_X, DIALOG_Y, DIALOG_W, DIALOG_H, COLOR_ABSENT);
    gfx_text(BOTTOM, SCREEN_W / 2, DIALOG_Y + 28, "Voltar ao menu?", FONT_MEDIUM, COLOR_TEXT);
    gfx_button(BOTTOM, NO_X, CHOICE_Y, CHOICE_W, CHOICE_H, "Não", FONT_MEDIUM, !confirm_yes);
    gfx_button(BOTTOM, YES_X, CHOICE_Y, CHOICE_W, CHOICE_H, "Sim", FONT_MEDIUM, confirm_yes);
}

static void draw(void)
{
    bool over = game_over(&game);

    board_draw(&game, typed, cursor);

    gfx_clear(BOTTOM, COLOR_BG);
    draw_header();
    if (over)
        draw_result();
    else
        draw_prompt();
    if (message)
        gfx_text(BOTTOM, SCREEN_W / 2, MESSAGE_Y, message, FONT_MEDIUM, COLOR_TEXT);
    keyboard_draw(&game, focus_keys && !over && !confirming ? focused_key : -1);
    if (confirming)
        draw_confirm();
}

bool play_update(void)
{
    uint32_t down = keysDown();
    uint32_t repeat = keysDownRepeat();
    touchPosition touch = { 0 };

    if (down & KEY_TOUCH)
        touchRead(&touch);

    if (confirming) {
        if (handle_confirm(down, touch.px, touch.py))
            return true;
        dirty |= down != 0;
    } else if ((down & KEY_START) || ((down & KEY_TOUCH) && inside(touch.px, touch.py, MENU_X,
                                                                   HEADER_Y, MENU_W, HEADER_H))) {
        confirming = true;
        confirm_yes = false;
        dirty = true;
    } else if (down & KEY_SELECT) {
        // Until modes can be picked from the menu, SELECT cycles through them.
        play_start(game.num_boards == 4 ? 1 : game.num_boards * 2);
    } else if (game_over(&game)) {
        if (down & (KEY_A | KEY_TOUCH))
            play_start(game.num_boards);
    } else if (down & KEY_TOUCH) {
        handle_touch(touch.px, touch.py);
        dirty = true;
    } else if (repeat) {
        handle_buttons(repeat);
        dirty = true;
    }

    if (message && --message_timer == 0) {
        message = NULL;
        dirty = true;
    }

    if (dirty) {
        draw();
        dirty = false;
    }
    return false;
}
