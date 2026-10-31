#include <stdio.h>

#include <nds.h>

#include "board.h"
#include "gfx.h"
#include "help.h"
#include "logo.h"
#include "menu.h"
#include "play.h"
#include "save.h"
#include "sound.h"
#include "stats.h"

#define ITEM_X 38
#define ITEM_W 180
#define ITEM_GAP 6

#define CREDITS_Y 20
#define CREDITS_STEP 11

#define DIALOG_X 28
#define DIALOG_Y 46
#define DIALOG_W 200
#define DIALOG_H 100
#define CHOICE_W 76
#define CHOICE_H 26
#define CHOICE_Y (DIALOG_Y + DIALOG_H - CHOICE_H - 12)

#define CHECK_SIZE 20

// The hard mode option is narrower to fit a "?" to its right that explains
// it, lined up with the buttons below.
#define HARD_MODE_ROW 3
#define HARD_HELP_W 26
#define HARD_HELP_X (ITEM_X + ITEM_W - HARD_HELP_W)

#define BLINK_FRAMES 40
#define PRESS_FRAMES 6

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
    "Música \"Gem Waltz\", de Haley Halcyon,",
    "sob CC-BY 4.0, convertida para o DS.",
    "Efeitos sonoros de Kenney (CC0).",
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
static bool on_hard_help;
static bool showing_hard_help;
static int pressed = -1;
static int press_timer;
static bool stats_daily = true;
static int stats_mode;

static void draw_logo(void)
{
    gfx_clear(TOP, COLOR_BG);
    gfx_text(TOP, SCREEN_W / 2, 126, "Jogo de palavras", FONT_MEDIUM, COLOR_DIM_TEXT);
    logo_show();
    logo_update(frame);
}

// Look of an item's button: focused, and sunk while it's being pressed.
static int state(int i)
{
    return (focused == i ? BUTTON_FOCUSED : 0) | (pressed == i ? BUTTON_PRESSED : 0);
}

// Pages with more items have them a bit shorter.
static int item_h(void)
{
    return page == PAGE_MAIN || page == PAGE_OPTIONS ? 26 : 30;
}

static int item_step(void)
{
    return page == PAGE_MAIN || page == PAGE_OPTIONS ? 30 : 36;
}

static int item_count(void)
{
    static const int counts[] = {
        [PAGE_MAIN] = 6, [PAGE_OPTIONS] = 6, [PAGE_ERASE] = 2, [PAGE_STATS] = 3, [PAGE_CREDITS] = 1,
    };
    return counts[page];
}

// Items are centered vertically on the bottom screen, except for the
// credits, where the only item goes below the text.
static int item_y(int i)
{
    if (page == PAGE_CREDITS)
        return SCREEN_H - item_h() - 8;
    return (SCREEN_H - item_count() * item_step() + (item_step() - item_h())) / 2 + i * item_step();
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
               state(0));
    gfx_button(BOTTOM, choice_x(1), CHOICE_Y, CHOICE_W, CHOICE_H, "Sim", FONT_MEDIUM, COLOR_KEY,
               state(1));
}

static void draw_stats_page(void)
{
    static const char *const kinds[] = { "Diário", "Livre" };
    int x, w;

    for (int i = 0; i < 2; i++) {
        stats_button(0, i, &x, &w);
        gfx_button(BOTTOM, x, item_y(0), w, item_h(), kinds[i], FONT_MEDIUM,
                   stats_daily == (i == 0) ? COLOR_CORRECT : COLOR_KEY,
                   focused == 0 && stats_daily == (i == 0) ? BUTTON_FOCUSED : 0);
    }
    for (int m = 0; m < MODE_COUNT; m++) {
        stats_button(1, m, &x, &w);
        gfx_button(BOTTOM, x, item_y(1), w, item_h(), mode_names[m], FONT_MEDIUM,
                   stats_mode == m ? COLOR_CORRECT : COLOR_KEY,
                   focused == 1 && stats_mode == m ? BUTTON_FOCUSED : 0);
    }
    gfx_button(BOTTOM, ITEM_X, item_y(2), ITEM_W, item_h(), "Voltar", FONT_MEDIUM, COLOR_KEY,
               state(2));
}

static void draw_credits(void)
{
    for (unsigned i = 0; i < sizeof(credits) / sizeof(credits[0]); i++) {
        gfx_text(BOTTOM, SCREEN_W / 2, CREDITS_Y + i * CREDITS_STEP, credits[i],
                 i == 0 ? FONT_MEDIUM : FONT_SMALL, i == 0 ? COLOR_TEXT : COLOR_DIM_TEXT);
    }
    gfx_button(BOTTOM, ITEM_X, item_y(0), ITEM_W, item_h(), "Voltar", FONT_MEDIUM, COLOR_KEY,
               state(0));
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

    gfx_button(BOTTOM, ITEM_X, item_y(0), ITEM_W, item_h(), daily, FONT_MEDIUM,
               done == MODE_COUNT ? COLOR_CORRECT : COLOR_KEY, state(0));
    gfx_button(BOTTOM, ITEM_X, item_y(1), ITEM_W, item_h(), "Jogo livre", FONT_MEDIUM, COLOR_KEY,
               state(1));
    gfx_button(BOTTOM, ITEM_X, item_y(2), ITEM_W, item_h(), "Estatísticas", FONT_MEDIUM, COLOR_KEY,
               state(2));
    gfx_button(BOTTOM, ITEM_X, item_y(3), ITEM_W, item_h(), "Opções", FONT_MEDIUM, COLOR_KEY,
               state(3));
    gfx_button(BOTTOM, ITEM_X, item_y(4), ITEM_W, item_h(), "Créditos", FONT_MEDIUM, COLOR_KEY,
               state(4));
    gfx_button(BOTTOM, ITEM_X, item_y(5), ITEM_W, item_h(), "Sair", FONT_MEDIUM, COLOR_KEY,
               state(5));
}

// Width of an item on the options page.
static int item_w(int i)
{
    if (page == PAGE_OPTIONS && i == HARD_MODE_ROW)
        return ITEM_W - HARD_HELP_W - ITEM_GAP;
    return ITEM_W;
}

static void draw_checkbox(int i, const char *label, bool checked)
{
    int y = item_y(i) + (pressed == i ? 2 : 0);
    int box_x = ITEM_X + 8;
    int box_y = y + (item_h() - CHECK_SIZE) / 2;

    if (focused == i && !on_hard_help)
        gfx_frame(BOTTOM, ITEM_X, y, item_w(i), item_h(), 2, COLOR_TEXT);
    gfx_rect(BOTTOM, box_x, box_y, CHECK_SIZE, CHECK_SIZE, checked ? COLOR_CORRECT : COLOR_KEY);
    if (checked)
        gfx_art(BOTTOM, box_x + 4, box_y + 6, check_icon, 8, 1, COLOR_TEXT);

    int text_x = box_x + CHECK_SIZE + 10;
    gfx_text(BOTTOM, text_x + gfx_text_width(label, FONT_MEDIUM) / 2,
             y + (item_h() + fonts[FONT_MEDIUM].cap_height) / 2, label, FONT_MEDIUM, COLOR_TEXT);
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
        draw_checkbox(2, "Alto contraste", save.high_contrast);
        draw_checkbox(HARD_MODE_ROW, "Modo difícil", save.hard_mode);
        gfx_button(BOTTOM, HARD_HELP_X, item_y(HARD_MODE_ROW), HARD_HELP_W, item_h(), "?",
                   FONT_SMALL, showing_hard_help ? COLOR_CORRECT : COLOR_KEY,
                   on_hard_help ? BUTTON_FOCUSED : 0);
        gfx_button(BOTTOM, ITEM_X, item_y(4), ITEM_W, item_h(), "Apagar dados", FONT_MEDIUM,
                   COLOR_KEY, state(4));
        gfx_button(BOTTOM, ITEM_X, item_y(5), ITEM_W, item_h(), "Voltar", FONT_MEDIUM, COLOR_KEY,
                   state(5));
    } else if (page == PAGE_ERASE) {
        draw_erase();
    } else if (page == PAGE_STATS) {
        draw_stats_page();
    } else {
        draw_credits();
    }
}

void menu_close(void)
{
    logo_hide();
}

void menu_start(bool title)
{
    page = title ? PAGE_TITLE : PAGE_MAIN;
    focused = 0;
    on_hard_help = false;
    showing_hard_help = false;
    frame = 0;
    draw_logo();
    draw();
}

static void toggle_hard_help(void)
{
    showing_hard_help = !showing_hard_help;
    sound_play(showing_hard_help ? SOUND_OPEN : SOUND_CLOSE);
    gfx_fade_out(true);
    if (showing_hard_help) {
        logo_hide();
        help_draw_hard_mode();
    } else {
        draw_logo();
    }
    gfx_fade_in(true);
}

static void go_to(enum page p, int item)
{
    if (showing_hard_help) {
        showing_hard_help = false;
        draw_logo();
    }
    on_hard_help = false;
    if (p == PAGE_STATS) {
        logo_hide();
        stats_draw(stats_daily, stats_mode, -1);
    } else if (page == PAGE_STATS) {
        draw_logo();
    }
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
        go_to(PAGE_OPTIONS, 4);
    else if (page == PAGE_STATS)
        go_to(PAGE_MAIN, 2);
    else if (page == PAGE_CREDITS)
        go_to(PAGE_MAIN, 4);
}

// Clears stats and games but keeps the options.
static void erase_data(void)
{
    struct save options = save;

    save_defaults(&save);
    save.sound_effects = options.sound_effects;
    save.music = options.music;
    save.high_contrast = options.high_contrast;
    save.hard_mode = options.hard_mode;
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
        else if (item == 4)
            go_to(PAGE_CREDITS, 0);
        else
            return MENU_QUIT;
        break;
    case PAGE_OPTIONS:
        if (item == 0) {
            save.sound_effects = !save.sound_effects;
            save_commit();
            // Only heard when turning them on, as they were off until now.
            sound_play(SOUND_ON);
        } else if (item == 1) {
            save.music = !save.music;
            save_commit();
            sound_update_music();
        } else if (item == 2) {
            save.high_contrast = !save.high_contrast;
            save_commit();
            gfx_set_high_contrast(save.high_contrast);
        } else if (item == 3) {
            save.hard_mode = !save.hard_mode;
            save_commit();
        } else if (item == 4) {
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

// Sound of pressing an item, or -1 when it plays only after the action.
static int item_sound(int item)
{
    switch (page) {
    case PAGE_MAIN:
        return item == 5 ? SOUND_CLOSE : SOUND_OPEN;
    case PAGE_OPTIONS:
        if (item == 0)
            return save.sound_effects ? SOUND_OFF : -1;
        if (item == 1)
            return save.music ? SOUND_OFF : SOUND_ON;
        if (item == 2)
            return save.high_contrast ? SOUND_OFF : SOUND_ON;
        if (item == 3)
            return save.hard_mode ? SOUND_OFF : SOUND_ON;
        return item == 4 ? SOUND_OPEN : SOUND_CLOSE;
    case PAGE_STATS:
        return item < 2 ? SOUND_CLICK : SOUND_CLOSE;
    default:
        return SOUND_CLOSE;
    }
}

// The action of an item happens once its button is done sinking.
static void press(int item)
{
    int sound = item_sound(item);
    if (sound >= 0)
        sound_play(sound);

    focused = item;
    pressed = item;
    press_timer = PRESS_FRAMES;
}

static bool inside(const touchPosition *t, int x, int y, int w, int h)
{
    return t->px >= x && t->px < x + w && t->py >= y && t->py < y + h;
}

static void touch_item(void)
{
    touchPosition touch;

    touchRead(&touch);
    if (page == PAGE_OPTIONS &&
        inside(&touch, HARD_HELP_X, item_y(HARD_MODE_ROW), HARD_HELP_W, item_h())) {
        focused = HARD_MODE_ROW;
        on_hard_help = true;
        toggle_hard_help();
        return;
    }
    for (int i = 0; i < item_count(); i++) {
        if (page == PAGE_ERASE) {
            if (inside(&touch, choice_x(i), CHOICE_Y, CHOICE_W, CHOICE_H))
                press(i);
        } else if (page == PAGE_STATS && i < 2) {
            for (int c = 0; c < stats_choices(i); c++) {
                int x, w;
                stats_button(i, c, &x, &w);
                if (inside(&touch, x, item_y(i), w, item_h())) {
                    focused = i;
                    pick_stats(i, c);
                    sound_play(SOUND_CLICK);
                }
            }
        } else if (inside(&touch, ITEM_X, item_y(i), item_w(i), item_h())) {
            on_hard_help = false;
            press(i);
        }
    }
}

enum menu_choice menu_update(void)
{
    uint32_t down = keysDown();
    enum menu_choice choice = MENU_NONE;

    frame++;
    logo_update(frame);
    if (page == PAGE_TITLE) {
        if (down & (KEY_START | KEY_A | KEY_TOUCH)) {
            sound_play(SOUND_OPEN);
            page = PAGE_MAIN;
            draw();
        } else if (frame % BLINK_FRAMES == 0) {
            draw();
        }
        return MENU_NONE;
    }

    if (press_timer) {
        if (--press_timer == 0) {
            int item = pressed;
            pressed = -1;
            choice = activate(item);
            draw();
        }
        return choice;
    }

    if (!down)
        return MENU_NONE;

    // The erase dialog lays its two choices side by side.
    uint32_t prev_key = page == PAGE_ERASE ? KEY_LEFT : KEY_UP;
    uint32_t next_key = page == PAGE_ERASE ? KEY_RIGHT : KEY_DOWN;

    if (down & (prev_key | next_key))
        sound_play(SOUND_CLICK);
    if (down & (prev_key | next_key))
        on_hard_help = false;
    if (down & prev_key)
        focused = (focused + item_count() - 1) % item_count();
    if (down & next_key)
        focused = (focused + 1) % item_count();
    if (page == PAGE_OPTIONS && focused == HARD_MODE_ROW && (down & (KEY_LEFT | KEY_RIGHT))) {
        on_hard_help = (down & KEY_RIGHT) != 0;
        sound_play(SOUND_CLICK);
    }
    if (page == PAGE_STATS && focused < 2 && (down & (KEY_LEFT | KEY_RIGHT))) {
        int n = stats_choices(focused);
        int current = focused == 0 ? !stats_daily : stats_mode;
        pick_stats(focused, (current + ((down & KEY_LEFT) ? n - 1 : 1)) % n);
        sound_play(SOUND_CLICK);
    }
    if ((down & KEY_A) && on_hard_help)
        toggle_hard_help();
    else if (down & KEY_A)
        press(focused);
    if ((down & KEY_Y) && page == PAGE_OPTIONS)
        toggle_hard_help();
    if ((down & KEY_B) && page != PAGE_MAIN) {
        sound_play(SOUND_CLOSE);
        back();
    }
    if (down & KEY_TOUCH)
        touch_item();

    draw();
    return choice;
}
