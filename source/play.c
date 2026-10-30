#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nds.h>

#include "board.h"
#include "daily.h"
#include "game.h"
#include "gfx.h"
#include "help.h"
#include "keyboard.h"
#include "play.h"
#include "save.h"
#include "sound.h"
#include "stats.h"
#include "words.h"

#define HEADER_Y 3
#define HEADER_H 18
#define MENU_X 4
#define MENU_W 52
#define HELP_X (MENU_X + MENU_W + 4)
#define HELP_W 20
#define PROMPT_TILE 28
#define PROMPT_GAP 4
#define PROMPT_X ((SCREEN_W - WORD_LEN * PROMPT_TILE - (WORD_LEN - 1) * PROMPT_GAP) / 2)
#define PROMPT_Y 25
#define MESSAGE_Y 68
#define HINT_Y 79
#define MESSAGE_FRAMES 150
#define STATS_DELAY 60

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
// Pressing the header's menu button sinks it like the items.
#define PRESS_HEADER ITEM_COUNT
#define PRESS_HELP (ITEM_COUNT + 1)
#define PRESS_FRAMES 6

// The games themselves are kept in the save. Sessions also remember what
// was being typed while the player is in another mode.
struct session {
    bool loaded;
    struct game game;
    char typed[WORD_LEN];
    int cursor;
};

// Indexed by daily and then by mode, like the save.
static struct session sessions[2][MODE_COUNT];

static bool daily;
static int mode;
static struct session *cur;
static struct saved_game *saved;
// What the top screen shows.
enum view {
    VIEW_BOARD,
    VIEW_STATS,
    VIEW_HELP,
};

static enum view view;
static int stats_timer;
// While a new guess is being revealed, the keyboard keeps its old colors.
static int reveal = -1;
static struct game before_guess;
static bool focus_keys;
static int focused_key;
static const char *message;
static int message_timer;
static bool menu_open;
static int menu_focus;
static int pressing = -1;
static int press_timer;
static bool dirty;

static const char restart_message[] = "START de novo para recomeçar";

// What the result screen said last, to redraw it when the countdown changes.
static char countdown[48];

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

static void load_game(struct game *g, const struct saved_game *sg, int m)
{
    game_start(g, mode_boards[m], sg->answers);
    for (int i = 0; i < sg->num_guesses; i++)
        game_guess(g, sg->guesses[i]);
}

static void new_game(void)
{
    memset(saved, 0, sizeof(*saved));
    if (daily) {
        saved->day = daily_day(time(NULL));
        daily_answers(saved->day, mode_boards[mode], saved->answers);
    } else {
        random_answers(mode_boards[mode], saved->answers);
    }
    saved->started = 1;
    save_commit();

    load_game(&cur->game, saved, mode);
    memset(cur->typed, 0, sizeof(cur->typed));
    cur->cursor = 0;
    cur->loaded = true;
}

static void switch_mode(int m)
{
    mode = m;
    cur = &sessions[daily][m];
    saved = &save.games[daily][m];
    if (save.last_mode[daily] != m) {
        save.last_mode[daily] = m;
        save_commit();
    }

    // A daily game from an earlier day gives way to today's.
    if (!saved->started || (daily && saved->day != daily_day(time(NULL)))) {
        new_game();
    } else if (!cur->loaded) {
        load_game(&cur->game, saved, m);
        cur->loaded = true;
    }

    focus_keys = true;
    focused_key = 0;
    message = NULL;
    menu_open = false;
    view = VIEW_BOARD;
    stats_timer = 0;
    reveal = -1;
    draw();
}

void play_open(bool is_daily)
{
    daily = is_daily;
    switch_mode(save.last_mode[daily]);
}

int play_daily_done(void)
{
    int today = daily_day(time(NULL));
    int done = 0;

    for (int m = 0; m < MODE_COUNT; m++) {
        const struct saved_game *sg = &save.games[1][m];
        struct game g;

        if (!sg->started || sg->day != today)
            continue;
        load_game(&g, sg, m);
        if (game_over(&g))
            done++;
    }
    return done;
}

void play_forget(void)
{
    memset(sessions, 0, sizeof(sessions));
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
        sound_play(SOUND_CLICK);
        show_message("Só palavras com 5 letras");
        return;
    }

    int index = word_find(cur->typed);
    if (index < 0) {
        sound_play(SOUND_CLICK);
        show_message("Essa palavra não é aceita");
        return;
    }

    if (save.hard_mode) {
        static char text[40];
        int position;
        char letter;

        enum hard_rule rule = game_check_hard(&cur->game, word_text(index), &position, &letter);
        if (rule != HARD_OK) {
            if (rule == HARD_POSITION)
                snprintf(text, sizeof(text), "A letra %d tem que ser %c", position + 1,
                         letter - 'a' + 'A');
            else
                snprintf(text, sizeof(text), "O chute tem que ter a letra %c", letter - 'a' + 'A');
            sound_play(SOUND_CLICK);
            show_message(text);
            return;
        }
    }

    memcpy(saved->guesses[saved->num_guesses++], word_text(index), WORD_LEN);
    before_guess = cur->game;
    game_guess(&cur->game, word_text(index));
    sound_play(SOUND_SUBMIT);
    reveal = 0;
    memset(cur->typed, 0, sizeof(cur->typed));
    cur->cursor = 0;
    message = NULL;

    if (game_over(&cur->game)) {
        stats_record(&save.stats[daily][mode], daily, saved->day, game_won(&cur->game),
                     cur->game.num_guesses);
        stats_timer = STATS_DELAY;
    }
    save_commit();
}

// Every key press goes through here so the key on screen sinks, even when it
// came from a button.
static void press(char c)
{
    keyboard_press(keyboard_find(c));
    if (c != KB_ENTER)
        sound_play(SOUND_CLICK);

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
            sound_play(SOUND_CLICK);
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
        press(KB_ERASE);
    if (down & KEY_R)
        press(KB_ENTER);
    if (down & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))
        sound_play(SOUND_CLICK);

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
            press(KB_ENTER);
        if (down & KEY_LEFT)
            move_cursor(-1);
        if (down & KEY_RIGHT)
            move_cursor(1);
        if (down & (KEY_UP | KEY_DOWN))
            focus_keys = true;
    }
}

static void restart(void)
{
    // Giving up on a game that has guesses counts as a loss.
    if (!game_over(&cur->game) && cur->game.num_guesses > 0) {
        stats_record(&save.stats[0][mode], false, 0, false, 0);
        save_commit();
    }
    new_game();
    view = VIEW_BOARD;
    message = NULL;
}

static void change_mode(int m)
{
    gfx_fade_out(false);
    switch_mode(m);
    gfx_fade_in(false);
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

// Changes what the top screen shows, fading only that screen.
static void show_view(enum view v)
{
    view = v;
    gfx_fade_out(true);
    draw();
    gfx_fade_in(true);
}

static void toggle_help(void)
{
    show_view(view == VIEW_HELP ? VIEW_BOARD : VIEW_HELP);
}

static void start_press(int item)
{
    bool opens = item == PRESS_HEADER || (item == PRESS_HELP && view != VIEW_HELP) ||
                 (item < MODE_COUNT && item != mode);

    sound_play(opens ? SOUND_OPEN : SOUND_CLOSE);
    pressing = item;
    press_timer = PRESS_FRAMES;
}

static void handle_menu(uint32_t down, int x, int y)
{
    if (down & KEY_TOUCH) {
        for (int i = 0; i < ITEM_COUNT; i++) {
            if (inside(x, y, ITEM_X, ITEM_Y + i * ITEM_STEP, ITEM_W, ITEM_H)) {
                menu_focus = i;
                start_press(i);
            }
        }
    }
    if (down & (KEY_UP | KEY_DOWN))
        sound_play(SOUND_CLICK);
    if (down & KEY_UP)
        menu_focus = (menu_focus + ITEM_COUNT - 1) % ITEM_COUNT;
    if (down & KEY_DOWN)
        menu_focus = (menu_focus + 1) % ITEM_COUNT;
    if (down & KEY_A)
        start_press(menu_focus);
    if (down & (KEY_B | KEY_START)) {
        sound_play(SOUND_CLOSE);
        menu_open = false;
    }
}

// Runs what was pressed once the button is done sinking. Returns true when
// it was "Sair".
static bool finish_press(void)
{
    int item = pressing;

    pressing = -1;
    if (item == PRESS_HELP) {
        toggle_help();
        return false;
    }
    if (item != PRESS_HEADER)
        return choose_item(item);
    menu_open = true;
    menu_focus = mode;
    return false;
}

static void draw_header(void)
{
    const char *name = mode_names[mode];
    const char *kind = daily ? "diário" : "livre";
    int name_w = gfx_text_width(name, FONT_MEDIUM);
    int baseline = HEADER_Y + (HEADER_H + fonts[FONT_MEDIUM].cap_height) / 2;

    gfx_button(BOTTOM, MENU_X, HEADER_Y, MENU_W, HEADER_H, "Menu", FONT_SMALL, COLOR_KEY,
               pressing == PRESS_HEADER ? BUTTON_PRESSED : 0);
    gfx_button(BOTTOM, HELP_X, HEADER_Y, HELP_W, HEADER_H, "?", FONT_SMALL,
               view == VIEW_HELP ? COLOR_CORRECT : COLOR_KEY,
               pressing == PRESS_HELP ? BUTTON_PRESSED : 0);
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

static void countdown_text(char *out, size_t size)
{
    int left = 86400 - time(NULL) % 86400;

    if (left >= 3600) {
        int hours = left / 3600;
        snprintf(out, size, "Novo desafio em %d hora%s", hours, hours == 1 ? "" : "s");
    } else {
        int minutes = (left + 59) / 60;
        snprintf(out, size, "Novo desafio em %d minuto%s", minutes, minutes == 1 ? "" : "s");
    }
}

static void draw_result(void)
{
    int num_boards = cur->game.num_boards;

    if (game_won(&cur->game)) {
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 23, "Parabéns!", FONT_LARGE, COLOR_TEXT);
    } else {
        char text[MAX_BOARDS * (WORD_LEN * 2 + 1)];

        answers_text(text);
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 8,
                 num_boards == 1 ? "A palavra era" : "As palavras eram", FONT_SMALL,
                 COLOR_DIM_TEXT);
        gfx_text(BOTTOM, SCREEN_W / 2, PROMPT_Y + 28, text,
                 num_boards == 4 ? FONT_MEDIUM : FONT_LARGE, COLOR_TEXT);
    }
    if (daily)
        countdown_text(countdown, sizeof(countdown));
    gfx_text(BOTTOM, SCREEN_W / 2, MESSAGE_Y, daily ? countdown : "A ou START: jogar de novo",
             FONT_MEDIUM, COLOR_DIM_TEXT);
    gfx_text(BOTTOM, SCREEN_W / 2, HINT_Y, "SELECT: estatísticas", FONT_SMALL, COLOR_DIM_TEXT);
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
                   (i == menu_focus ? BUTTON_FOCUSED : 0) | (i == pressing ? BUTTON_PRESSED : 0));
    }
}

static void draw(void)
{
    // The result only shows once the last guess is revealed.
    bool over = game_over(&cur->game) && reveal < 0;

    if (view == VIEW_STATS) {
        int last = -1;
        if (over)
            last = game_won(&cur->game) ? cur->game.num_guesses : 0;
        stats_draw(daily, mode, last);
    } else if (view == VIEW_HELP) {
        help_draw();
    } else {
        board_draw(&cur->game, cur->typed, cur->cursor, reveal);
    }

    gfx_clear(BOTTOM, COLOR_BG);
    draw_header();
    if (over)
        draw_result();
    else
        draw_prompt();
    if (message)
        gfx_text(BOTTOM, SCREEN_W / 2, MESSAGE_Y, message, FONT_MEDIUM, COLOR_TEXT);
    keyboard_draw(reveal >= 0 ? &before_guess : &cur->game,
                  focus_keys && !over && !menu_open ? focused_key : -1);
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

    // Input waits until the new guess is revealed.
    if (reveal >= 0) {
        if (++reveal == board_reveal_frames()) {
            reveal = -1;
            if (game_over(&cur->game))
                sound_fanfare(game_won(&cur->game));
        }
        keyboard_animate();
        draw();
        return false;
    }

    if (daily && !menu_open) {
        char text[sizeof(countdown)];

        // At midnight a finished daily gives way to the new one.
        if (saved->day != daily_day(time(NULL)) && game_over(&cur->game)) {
            switch_mode(mode);
            return false;
        }
        countdown_text(text, sizeof(text));
        dirty |= game_over(&cur->game) && strcmp(text, countdown) != 0;
    }

    if (press_timer) {
        if (--press_timer == 0 && finish_press())
            return true;
        dirty = true;
    } else if (menu_open) {
        handle_menu(down, touch.px, touch.py);
        dirty |= down != 0;
    } else if ((down & KEY_TOUCH) &&
               inside(touch.px, touch.py, MENU_X, HEADER_Y, MENU_W, HEADER_H)) {
        start_press(PRESS_HEADER);
        dirty = true;
    } else if ((down & KEY_TOUCH) &&
               inside(touch.px, touch.py, HELP_X, HEADER_Y, HELP_W, HEADER_H)) {
        start_press(PRESS_HELP);
        dirty = true;
    } else if (down & KEY_Y) {
        sound_play(view == VIEW_HELP ? SOUND_CLOSE : SOUND_OPEN);
        toggle_help();
    } else if ((down & KEY_X) || (daily && (down & KEY_START))) {
        sound_play(SOUND_OPEN);
        menu_open = true;
        menu_focus = mode;
        dirty = true;
    } else if (down & KEY_START) {
        // In free play START starts over, asking twice if the game is going.
        if (game_over(&cur->game) || cur->game.num_guesses == 0 || message == restart_message) {
            sound_play(SOUND_OPEN);
            restart();
        } else {
            sound_play(SOUND_CLICK);
            show_message(restart_message);
        }
        dirty = true;
    } else if ((down & KEY_SELECT) || (stats_timer && --stats_timer == 0)) {
        // The timer only ever opens the stats, after a game ends.
        enum view next = VIEW_STATS;
        if ((down & KEY_SELECT) && view == VIEW_STATS)
            next = VIEW_BOARD;
        if (down & KEY_SELECT)
            sound_play(next == VIEW_STATS ? SOUND_OPEN : SOUND_CLOSE);
        stats_timer = 0;
        show_view(next);
    } else if (game_over(&cur->game)) {
        if (!daily && (down & (KEY_A | KEY_TOUCH))) {
            sound_play(SOUND_OPEN);
            restart();
            dirty = true;
        }
    } else if (down & KEY_TOUCH) {
        handle_touch(touch.px, touch.py);
        dirty = true;
    } else if (repeat) {
        handle_buttons(repeat);
        dirty = true;
    }

    dirty |= keyboard_animate();

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
