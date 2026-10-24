#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nds.h>

#include "board.h"
#include "daily.h"
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

// The in-game menu lists the modes, then "Sair" and "Cancelar".
#define ITEM_COUNT (MODE_COUNT + 2)
#define ITEM_QUIT MODE_COUNT
#define ITEM_CANCEL (MODE_COUNT + 1)
#define ITEM_X 56
#define ITEM_W 144
#define ITEM_H 22
#define ITEM_STEP 27
#define ITEM_Y 34
#define PANEL_PAD 8

static const int mode_boards[MODE_COUNT] = { 1, 2, 4 };
static const char *const mode_names[MODE_COUNT] = { "Quina", "Duque", "Quadra" };

struct session {
    bool started;
    int day;
    struct game game;
    char typed[WORD_LEN];
    int cursor;
};

// Indexed by daily and then by mode, so each keeps its own progress.
static struct session sessions[2][MODE_COUNT];
static int last_mode[2];

static bool daily;
static int mode;
static struct session *cur;
static bool focus_keys;
static int focused_key;
static const char *message;
static int message_timer;
static bool menu_open;
static int menu_focus;
static bool dirty;

static void draw(void);

static void show_message(const char *text)
{
    message = text;
    message_timer = MESSAGE_FRAMES;
}

static void random_answers(int num_boards, char answers[][WORD_LEN])
{
    for (int b = 0; b < num_boards; b++) {
        bool repeated;
        do {
            memcpy(answers[b], word_text(answer_order[rand() % answer_count]), WORD_LEN);
            repeated = false;
            for (int other = 0; other < b; other++)
                repeated |= memcmp(answers[b], answers[other], WORD_LEN) == 0;
        } while (repeated);
    }
}

static void new_game(struct session *s, int m)
{
    char answers[MAX_BOARDS][WORD_LEN];

    if (daily) {
        s->day = daily_day(time(NULL));
        daily_answers(s->day, mode_boards[m], answers);
    } else {
        random_answers(mode_boards[m], answers);
    }
    game_start(&s->game, mode_boards[m], answers);
    memset(s->typed, 0, sizeof(s->typed));
    s->cursor = 0;
    s->started = true;
}

static void switch_mode(int m)
{
    mode = m;
    last_mode[daily] = m;
    cur = &sessions[daily][m];

    // A daily game from an earlier day gives way to today's.
    if (!cur->started || (daily && cur->day != daily_day(time(NULL))))
        new_game(cur, m);

    focus_keys = true;
    focused_key = 0;
    message = NULL;
    menu_open = false;
    draw();
}

void play_open(bool is_daily)
{
    daily = is_daily;
    switch_mode(last_mode[daily]);
}

int play_daily_done(void)
{
    int today = daily_day(time(NULL));
    int done = 0;

    for (int m = 0; m < MODE_COUNT; m++) {
        struct session *s = &sessions[1][m];
        if (s->started && s->day == today && game_over(&s->game))
            done++;
    }
    return done;
}

static void type_letter(char c)
{
    cur->typed[cur->cursor] = c;
    if (cur->cursor < WORD_LEN - 1)
        cur->cursor++;
}

static void erase(void)
{
    if (!cur->typed[cur->cursor] && cur->cursor > 0)
        cur->cursor--;
    cur->typed[cur->cursor] = 0;
}

static void move_cursor(int dx)
{
    int cursor = cur->cursor + dx;
    if (cursor >= 0 && cursor < WORD_LEN)
        cur->cursor = cursor;
}

static void submit(void)
{
    if (memchr(cur->typed, 0, WORD_LEN)) {
        show_message("Só palavras com 5 letras");
        return;
    }

    int index = word_find(cur->typed);
    if (index < 0) {
        show_message("Essa palavra não é aceita");
        return;
    }

    game_guess(&cur->game, word_text(index));
    memset(cur->typed, 0, sizeof(cur->typed));
    cur->cursor = 0;
    message = NULL;
}

static void press(char c)
{
    if (c == KB_ENTER)
        submit();
    else if (c == KB_ERASE)
        erase();
    else if (c == KB_LEFT)
        move_cursor(-1);
    else if (c == KB_RIGHT)
        move_cursor(1);
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
            cur->cursor = box;
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
        if (down & KEY_LEFT)
            move_cursor(-1);
        if (down & KEY_RIGHT)
            move_cursor(1);
        if (down & (KEY_UP | KEY_DOWN))
            focus_keys = true;
    }
}

static void change_mode(int m)
{
    gfx_fade_out();
    switch_mode(m);
    gfx_fade_in();
}

// Returns true when the player picks "Sair".
static bool choose_item(int item)
{
    if (item == ITEM_QUIT)
        return true;
    if (item < MODE_COUNT && item != mode)
        change_mode(item);
    else
        menu_open = false;
    return false;
}

static bool handle_menu(uint32_t down, int x, int y)
{
    if (down & KEY_TOUCH) {
        for (int i = 0; i < ITEM_COUNT; i++) {
            if (inside(x, y, ITEM_X, ITEM_Y + i * ITEM_STEP, ITEM_W, ITEM_H))
                return choose_item(i);
        }
    }
    if (down & KEY_UP)
        menu_focus = (menu_focus + ITEM_COUNT - 1) % ITEM_COUNT;
    if (down & KEY_DOWN)
        menu_focus = (menu_focus + 1) % ITEM_COUNT;
    if (down & KEY_A)
        return choose_item(menu_focus);
    if (down & (KEY_B | KEY_START))
        menu_open = false;
    return false;
}

static void draw_header(void)
{
    const char *name = mode_names[mode];
    const char *kind = daily ? "diário" : "livre";
    int name_w = gfx_text_width(name, FONT_MEDIUM);
    int baseline = HEADER_Y + (HEADER_H + fonts[FONT_MEDIUM].cap_height) / 2;

    gfx_button(BOTTOM, MENU_X, HEADER_Y, MENU_W, HEADER_H, "Menu", FONT_SMALL, COLOR_KEY, false);
    gfx_text(BOTTOM, SCREEN_W - 6 - name_w / 2, baseline, name, FONT_MEDIUM, COLOR_TEXT);
    gfx_text(BOTTOM, SCREEN_W - 12 - name_w - gfx_text_width(kind, FONT_SMALL) / 2, baseline, kind,
             FONT_SMALL, COLOR_DIM_TEXT);
}

static void draw_prompt(void)
{
    for (int i = 0; i < WORD_LEN; i++) {
        int x = PROMPT_X + i * (PROMPT_TILE + PROMPT_GAP);
        bool selected = i == cur->cursor;
        draw_tile(BOTTOM, x, PROMPT_Y, PROMPT_TILE, PROMPT_TILE, FONT_LARGE, cur->typed[i],
                  COLOR_BG, true, selected);
        if (selected && !focus_keys)
            gfx_frame(BOTTOM, x, PROMPT_Y, PROMPT_TILE, PROMPT_TILE, 2, COLOR_TEXT);
    }
}

// Writes the answers in uppercase UTF-8, separated by spaces. Display forms
// are lowercase Latin-1, where uppercase letters are 0x20 below.
static void answers_text(char *out)
{
    char word[WORD_LEN];

    for (int b = 0; b < cur->game.num_boards; b++) {
        if (b > 0)
            *out++ = ' ';
        word_display(word_find(cur->game.answers[b]), word);
        for (int i = 0; i < WORD_LEN; i++) {
            unsigned char c = word[i];
            if ((c >= 'a' && c <= 'z') || (c >= 0xE0 && c != 0xF7))
                c -= 0x20;
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
    int num_boards = cur->game.num_boards;

    if (game_won(&cur->game)) {
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 21, "Parabéns!", FONT_LARGE, COLOR_TEXT);
    } else {
        char text[MAX_BOARDS * (WORD_LEN * 2 + 1)];

        answers_text(text);
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 6,
                 num_boards == 1 ? "A palavra era" : "As palavras eram", FONT_SMALL,
                 COLOR_DIM_TEXT);
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 26, text,
                 num_boards == 4 ? FONT_MEDIUM : FONT_LARGE, COLOR_TEXT);
    }
    gfx_text(BOTTOM, SCREEN_W / 2, MESSAGE_Y, daily ? "Novo desafio amanhã" : "A: jogar de novo",
             FONT_MEDIUM, COLOR_DIM_TEXT);
}

static void draw_menu(void)
{
    int panel_y = ITEM_Y - PANEL_PAD;
    int panel_h = ITEM_COUNT * ITEM_STEP - (ITEM_STEP - ITEM_H) + 2 * PANEL_PAD;

    gfx_rect(BOTTOM, ITEM_X - PANEL_PAD - 2, panel_y - 2, ITEM_W + 2 * PANEL_PAD + 4, panel_h + 4,
             COLOR_BG);
    gfx_rect(BOTTOM, ITEM_X - PANEL_PAD, panel_y, ITEM_W + 2 * PANEL_PAD, panel_h, COLOR_ABSENT);

    for (int i = 0; i < ITEM_COUNT; i++) {
        const char *label = i < MODE_COUNT ? mode_names[i] : i == ITEM_QUIT ? "Sair" : "Cancelar";
        enum color fill = i == mode ? COLOR_CORRECT : COLOR_KEY;
        gfx_button(BOTTOM, ITEM_X, ITEM_Y + i * ITEM_STEP, ITEM_W, ITEM_H, label, FONT_MEDIUM, fill,
                   i == menu_focus);
    }
}

static void draw(void)
{
    bool over = game_over(&cur->game);

    board_draw(&cur->game, cur->typed, cur->cursor);

    gfx_clear(BOTTOM, COLOR_BG);
    draw_header();
    if (over)
        draw_result();
    else
        draw_prompt();
    if (message)
        gfx_text(BOTTOM, SCREEN_W / 2, MESSAGE_Y, message, FONT_MEDIUM, COLOR_TEXT);
    keyboard_draw(&cur->game, focus_keys && !over && !menu_open ? focused_key : -1);
    if (menu_open)
        draw_menu();
}

bool play_update(void)
{
    uint32_t down = keysDown();
    uint32_t repeat = keysDownRepeat();
    touchPosition touch = { 0 };

    if (down & KEY_TOUCH)
        touchRead(&touch);

    if (menu_open) {
        if (handle_menu(down, touch.px, touch.py))
            return true;
        dirty |= down != 0;
    } else if ((down & KEY_START) || ((down & KEY_TOUCH) && inside(touch.px, touch.py, MENU_X,
                                                                   HEADER_Y, MENU_W, HEADER_H))) {
        menu_open = true;
        menu_focus = mode;
        dirty = true;
    } else if (down & KEY_SELECT) {
        change_mode((mode + 1) % MODE_COUNT);
    } else if (game_over(&cur->game)) {
        if (!daily && (down & (KEY_A | KEY_TOUCH))) {
            new_game(cur, mode);
            dirty = true;
        }
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
