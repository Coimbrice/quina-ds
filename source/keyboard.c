#include <stdlib.h>

#include "board.h"
#include "gfx.h"
#include "keyboard.h"

// Every row has ten keys. The two bottom rows are shifted half a key to
// the right, closer to the shape of a real keyboard.
#define KEY_Y 84
#define KEY_W 20
#define KEY_H 30
#define KEY_STEP_X 23
#define KEY_STEP_Y 34
#define KEYS_PER_ROW 10
#define ROW_SHIFT (KEY_STEP_X / 2)
#define KEY_X ((SCREEN_W - KEYS_PER_ROW * KEY_STEP_X + (KEY_STEP_X - KEY_W) - ROW_SHIFT) / 2)

// A pressed key sinks for a few frames.
#define PRESS_FRAMES 6
#define PRESS_DEPTH 2

#define MAX_KEYS 30

struct key {
    char c;
    int row;
    int x;
    int w;
};

static const char *const rows[] = {
    "qwertyuiop",
    "asdfghjkl\b",
    "zxcvbnm\1\2\n",
};

// clang-format off
static const char *const erase_icon[] = {
    "...#########",
    "..##.......#",
    ".##..#...#.#",
    "##....#.#..#",
    "#......#...#",
    "##....#.#..#",
    ".##..#...#.#",
    "..##.......#",
    "...#########",
};
// clang-format on

// clang-format off
static const char *const enter_icon[] = {
    "..........##",
    "..........##",
    "...#......##",
    "..##......##",
    ".###########",
    "############",
    ".###########",
    "..##........",
    "...#........",
};
// clang-format on

static const char *const left_icon[] = {
    "....#", "...##", "..###", ".####", "#####", ".####", "..###", "...##", "....#",
};

static const char *const right_icon[] = {
    "#....", "##...", "###..", "####.", "#####", "####.", "###..", "##...", "#....",
};

static struct key keys[MAX_KEYS];
static int key_count;
static int wanted_x;
static int pressed = -1;
static int press_timer;

void keyboard_init(void)
{
    key_count = 0;
    for (int row = 0; row < 3; row++) {
        int x = KEY_X + (row > 0 ? ROW_SHIFT : 0);
        for (const char *c = rows[row]; *c; c++) {
            struct key *k = &keys[key_count++];
            k->c = *c;
            k->row = row;
            k->x = x;
            k->w = KEY_W;
            x += KEY_STEP_X;
        }
    }
    wanted_x = keys[0].x;
}

static void draw_icon(int k, int y, int h, const char *const *icon, int count, int scale,
                      enum color color)
{
    int w = 0;
    while (icon[0][w])
        w++;
    gfx_art(BOTTOM, keys[k].x + (keys[k].w - w * scale) / 2, y + (h - count * scale) / 2, icon,
            count, scale, color);
}

// With several boards each key is split into one part per board: halves
// for two, quadrants for four.
static void draw_key_fill(const struct game *g, const struct key *key, int y, int h)
{
    int cols = g->num_boards == 1 ? 1 : 2;
    int rows = g->num_boards == 4 ? 2 : 1;
    int part_w = key->w / cols;
    int part_h = h / rows;

    for (int b = 0; b < g->num_boards; b++) {
        enum hint hint = game_key_hint(g, b, key->c);
        int px = key->x + (b % cols) * part_w;
        int py = y + (b / cols) * part_h;

        gfx_clip(px, py, b % cols == cols - 1 ? key->x + key->w - px : part_w,
                 b / cols == rows - 1 ? y + h - py : part_h);
        gfx_rect(BOTTOM, key->x, y, key->w, h, hint == HINT_NONE ? COLOR_KEY : hint_color(hint));
    }
    gfx_unclip();
}

static void draw_key(const struct game *g, int k, bool focused)
{
    struct key *key = &keys[k];
    int y = KEY_Y + key->row * KEY_STEP_Y;
    int h = KEY_H;
    enum color text = COLOR_TEXT;

    if (k == pressed) {
        y += PRESS_DEPTH;
        h -= PRESS_DEPTH;
    }

    if (key->c >= 'a' && key->c <= 'z') {
        draw_key_fill(g, key, y, h);

        bool absent = true;
        for (int b = 0; b < g->num_boards; b++)
            absent &= game_key_hint(g, b, key->c) == HINT_ABSENT;
        if (absent)
            text = COLOR_DIM_TEXT;
    } else {
        gfx_rect(BOTTOM, key->x, y, key->w, h, COLOR_KEY);
    }

    if (focused)
        gfx_frame(BOTTOM, key->x, y, key->w, h, 2, COLOR_TEXT);

    switch (key->c) {
    case KB_ENTER:
        draw_icon(k, y, h, enter_icon, 9, 1, text);
        break;
    case KB_ERASE:
        draw_icon(k, y, h, erase_icon, 9, 1, text);
        break;
    case KB_LEFT:
        draw_icon(k, y, h, left_icon, 9, 2, text);
        break;
    case KB_RIGHT:
        draw_icon(k, y, h, right_icon, 9, 2, text);
        break;
    default:
        gfx_letter(BOTTOM, key->x + key->w / 2, y + h / 2, key->c, FONT_MEDIUM, text);
    }
}

void keyboard_draw(const struct game *g, int focused)
{
    for (int k = 0; k < key_count; k++)
        draw_key(g, k, k == focused);
}

int keyboard_hit(int x, int y)
{
    for (int k = 0; k < key_count; k++) {
        int ky = KEY_Y + keys[k].row * KEY_STEP_Y;
        if (x >= keys[k].x && x < keys[k].x + keys[k].w && y >= ky && y < ky + KEY_H)
            return k;
    }
    return -1;
}

int keyboard_move(int k, int dx, int dy)
{
    int row = keys[k].row;

    if (dx != 0) {
        int first = k;
        int last = k;
        while (first > 0 && keys[first - 1].row == row)
            first--;
        while (last < key_count - 1 && keys[last + 1].row == row)
            last++;

        k += dx;
        if (k < first)
            k = last;
        if (k > last)
            k = first;
        wanted_x = keys[k].x + keys[k].w / 2;
        return k;
    }

    row += dy;
    if (row < 0 || row > 2)
        return k;

    // Pick the key in the new row closest to where the player was going.
    int best = -1;
    int best_distance = 0;
    for (int i = 0; i < key_count; i++) {
        if (keys[i].row != row)
            continue;
        int distance = abs(keys[i].x + keys[i].w / 2 - wanted_x);
        if (best < 0 || distance < best_distance) {
            best = i;
            best_distance = distance;
        }
    }
    return best;
}

char keyboard_char(int k)
{
    return keys[k].c;
}

int keyboard_find(char c)
{
    for (int k = 0; k < key_count; k++) {
        if (keys[k].c == c)
            return k;
    }
    return -1;
}

void keyboard_press(int k)
{
    pressed = k;
    press_timer = PRESS_FRAMES;
}

bool keyboard_animate(void)
{
    if (press_timer == 0)
        return false;
    if (--press_timer == 0)
        pressed = -1;
    return true;
}
