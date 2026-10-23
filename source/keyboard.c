#include <stdlib.h>

#include "board.h"
#include "font.h"
#include "gfx.h"
#include "keyboard.h"

// The bottom row has one more slot than the others, so it's shifted half a
// key to the left, like on a real keyboard. ENTER takes two slots.
#define KEY_Y 66
#define KEY_W 21
#define KEY_H 34
#define KEY_STEP_X 23
#define KEY_STEP_Y 38
#define ENTER_SLOTS 2

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

static const char *const left_icon[] = {
    "....#", "...##", "..###", ".####", "#####", ".####", "..###", "...##", "....#",
};

static const char *const right_icon[] = {
    "#....", "##...", "###..", "####.", "#####", "####.", "###..", "##...", "#....",
};

static struct key keys[MAX_KEYS];
static int key_count;
static int wanted_x;

void keyboard_init(void)
{
    key_count = 0;
    for (int row = 0; row < 3; row++) {
        int slots = 0;
        for (const char *c = rows[row]; *c; c++)
            slots += *c == KB_ENTER ? ENTER_SLOTS : 1;

        int x = (SCREEN_W - slots * KEY_STEP_X + (KEY_STEP_X - KEY_W)) / 2;
        for (const char *c = rows[row]; *c; c++) {
            struct key *k = &keys[key_count++];
            int span = *c == KB_ENTER ? ENTER_SLOTS : 1;
            k->c = *c;
            k->row = row;
            k->x = x;
            k->w = span * KEY_STEP_X - (KEY_STEP_X - KEY_W);
            x += span * KEY_STEP_X;
        }
    }
    wanted_x = keys[0].x;
}

static void draw_icon(int k, const char *const *icon, int count, int scale, enum color color)
{
    int w = 0;
    while (icon[0][w])
        w++;
    gfx_art(BOTTOM, keys[k].x + (keys[k].w - w * scale) / 2,
            KEY_Y + keys[k].row * KEY_STEP_Y + (KEY_H - count * scale) / 2, icon, count, scale,
            color);
}

static void draw_key(const struct game *g, int k, bool focused)
{
    struct key *key = &keys[k];
    int y = KEY_Y + key->row * KEY_STEP_Y;
    enum hint hint = HINT_NONE;

    if (key->c >= 'a' && key->c <= 'z')
        hint = game_key_hint(g, 0, key->c);

    enum color fill = hint == HINT_NONE ? COLOR_KEY : hint_color(hint);
    enum color text = hint == HINT_ABSENT ? COLOR_DIM_TEXT : COLOR_TEXT;

    gfx_rect(BOTTOM, key->x, y, key->w, KEY_H, fill);
    if (focused)
        gfx_frame(BOTTOM, key->x, y, key->w, KEY_H, 2, COLOR_TEXT);

    switch (key->c) {
    case KB_ENTER:
        gfx_text(BOTTOM, key->x + key->w / 2, y + (KEY_H - GLYPH_H) / 2, "ENTER", 1, text);
        break;
    case KB_ERASE:
        draw_icon(k, erase_icon, 9, 1, text);
        break;
    case KB_LEFT:
        draw_icon(k, left_icon, 9, 2, text);
        break;
    case KB_RIGHT:
        draw_icon(k, right_icon, 9, 2, text);
        break;
    default:
        gfx_char(BOTTOM, key->x + (key->w - GLYPH_W * 2) / 2, y + (KEY_H - GLYPH_H * 2) / 2, key->c,
                 2, text);
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
