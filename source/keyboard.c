#include <string.h>

#include "board.h"
#include "font.h"
#include "gfx.h"
#include "keyboard.h"

// Keys sit on a grid of 10 columns. ENTER takes the last three.
#define KEY_X 4
#define KEY_Y 66
#define KEY_W 23
#define KEY_H 34
#define KEY_STEP_X 25
#define KEY_STEP_Y 38
#define ENTER_SPAN 3

#define MAX_KEYS 28

struct key {
    char c;
    int row;
    int col;
    int span;
};

static const char *const rows[] = { "qwertyuiop", "asdfghjkl\b", "zxcvbnm\n" };

static struct key keys[MAX_KEYS];
static int key_count;
static int wanted_col;

void keyboard_init(void)
{
    key_count = 0;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; rows[row][col]; col++) {
            struct key *k = &keys[key_count++];
            k->c = rows[row][col];
            k->row = row;
            k->col = col;
            k->span = k->c == KB_ENTER ? ENTER_SPAN : 1;
        }
    }
}

static void key_rect(int k, int *x, int *y, int *w)
{
    *x = KEY_X + keys[k].col * KEY_STEP_X;
    *y = KEY_Y + keys[k].row * KEY_STEP_Y;
    *w = keys[k].span * KEY_STEP_X - (KEY_STEP_X - KEY_W);
}

static void draw_key(const struct game *g, int k, bool focused)
{
    int x, y, w;
    enum hint hint = HINT_NONE;
    char c = keys[k].c;

    key_rect(k, &x, &y, &w);

    if (c >= 'a' && c <= 'z')
        hint = game_key_hint(g, 0, c);

    enum color fill = hint == HINT_NONE ? COLOR_KEY : hint_color(hint);
    enum color text = hint == HINT_ABSENT ? COLOR_DIM_TEXT : COLOR_TEXT;

    gfx_rect(BOTTOM, x, y, w, KEY_H, fill);
    if (focused)
        gfx_frame(BOTTOM, x, y, w, KEY_H, 2, COLOR_TEXT);

    int text_y = y + (KEY_H - GLYPH_H * 2) / 2;
    if (c == KB_ENTER)
        gfx_text(BOTTOM, x + w / 2, text_y, "ENTER", 2, text);
    else if (c == KB_ERASE)
        gfx_char(BOTTOM, x + (w - GLYPH_W * 2) / 2, text_y, FONT_ERASE, 2, text);
    else
        gfx_char(BOTTOM, x + (w - GLYPH_W * 2) / 2, text_y, c, 2, text);
}

void keyboard_draw(const struct game *g, int focused)
{
    for (int k = 0; k < key_count; k++)
        draw_key(g, k, k == focused);
}

int keyboard_hit(int x, int y)
{
    for (int k = 0; k < key_count; k++) {
        int kx, ky, kw;
        key_rect(k, &kx, &ky, &kw);
        if (x >= kx && x < kx + kw && y >= ky && y < ky + KEY_H)
            return k;
    }
    return -1;
}

static int find_key(int row, int col)
{
    int last = -1;

    for (int k = 0; k < key_count; k++) {
        if (keys[k].row != row)
            continue;
        if (col >= keys[k].col && col < keys[k].col + keys[k].span)
            return k;
        last = k;
    }
    return last;
}

int keyboard_move(int k, int dx, int dy)
{
    int row = keys[k].row;

    if (dx != 0) {
        int first = find_key(row, 0);
        int last = find_key(row, 9);
        k += dx;
        if (k < first)
            k = last;
        if (k > last)
            k = first;
        wanted_col = keys[k].col;
        return k;
    }

    row += dy;
    if (row < 0 || row > 2)
        return k;
    return find_key(row, wanted_col);
}

char keyboard_char(int k)
{
    return keys[k].c;
}
