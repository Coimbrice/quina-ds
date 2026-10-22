#include <stdlib.h>
#include <string.h>

#include <nds.h>

#include "board.h"
#include "game.h"
#include "gfx.h"
#include "keyboard.h"
#include "play.h"
#include "words.h"

#define PROMPT_TILE 30
#define PROMPT_GAP 4
#define PROMPT_X ((SCREEN_W - WORD_LEN * PROMPT_TILE - (WORD_LEN - 1) * PROMPT_GAP) / 2)
#define PROMPT_Y 6
#define MESSAGE_Y 46
#define MESSAGE_FRAMES 150

static struct game game;
static char typed[WORD_LEN];
static int cursor;
static bool focus_keys;
static int focused_key;
static const char *message;
static int message_timer;
static bool dirty;

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
    dirty = true;
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
    else
        type_letter(c);
}

static bool prompt_hit(int x, int y, int *box)
{
    if (y < PROMPT_Y || y >= PROMPT_Y + PROMPT_TILE || x < PROMPT_X)
        return false;
    *box = (x - PROMPT_X) / (PROMPT_TILE + PROMPT_GAP);
    return *box < WORD_LEN;
}

static void handle_touch(void)
{
    touchPosition touch;
    int box;

    touchRead(&touch);
    if (prompt_hit(touch.px, touch.py, &box)) {
        cursor = box;
        focus_keys = false;
        return;
    }

    int k = keyboard_hit(touch.px, touch.py);
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

static void draw_prompt(void)
{
    for (int i = 0; i < WORD_LEN; i++) {
        int x = PROMPT_X + i * (PROMPT_TILE + PROMPT_GAP);
        bool selected = i == cursor;
        draw_tile(BOTTOM, x, PROMPT_Y, PROMPT_TILE, PROMPT_TILE, 2, typed[i], COLOR_BG, true,
                  selected);
        if (selected && !focus_keys)
            gfx_frame(BOTTOM, x, PROMPT_Y, PROMPT_TILE, PROMPT_TILE, 2, COLOR_TEXT);
    }
}

static void draw_result(void)
{
    if (game_won(&game)) {
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 10, "Parabéns!", 2, COLOR_TEXT);
    } else {
        char word[WORD_LEN];
        char text[WORD_LEN * 2 + 1];
        char *p = text;

        // Display forms are Latin-1, the text functions take UTF-8.
        word_display(word_find(game.answers[0]), word);
        for (int i = 0; i < WORD_LEN; i++) {
            unsigned char c = word[i];
            if (c >= 0x80) {
                *p++ = 0xC0 | (c >> 6);
                *p++ = 0x80 | (c & 0x3F);
            } else {
                *p++ = c;
            }
        }
        *p = 0;

        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 2, "A palavra era", 1, COLOR_DIM_TEXT);
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 18, text, 2, COLOR_TEXT);
    }
    gfx_text(BOTTOM, SCREEN_W / 2, MESSAGE_Y, "A: jogar de novo", 1, COLOR_DIM_TEXT);
}

static void draw(void)
{
    bool over = game_over(&game);

    board_draw(&game, typed, cursor);

    gfx_clear(BOTTOM, COLOR_BG);
    if (over)
        draw_result();
    else
        draw_prompt();
    if (message)
        gfx_text(BOTTOM, SCREEN_W / 2, MESSAGE_Y, message, 1, COLOR_TEXT);
    keyboard_draw(&game, focus_keys && !over ? focused_key : -1);
}

void play_update(void)
{
    uint32_t down = keysDown();
    uint32_t repeat = keysDownRepeat();

    if (game_over(&game)) {
        if (down & (KEY_A | KEY_START | KEY_TOUCH))
            play_start(game.num_boards);
    } else if (down & KEY_TOUCH) {
        handle_touch();
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
}
