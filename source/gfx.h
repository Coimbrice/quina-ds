#ifndef GFX_H
#define GFX_H

#include "font.h"

#define SCREEN_W 256
#define SCREEN_H 192

enum screen {
    TOP,
    BOTTOM,
};

enum color {
    COLOR_BG,
    COLOR_EMPTY,
    COLOR_BORDER,
    COLOR_ABSENT,
    COLOR_PRESENT,
    COLOR_CORRECT,
    COLOR_KEY,
    COLOR_TEXT,
    COLOR_DIM_TEXT,
    COLOR_COUNT,
};

void gfx_init(void);
// Copies what was drawn to the screens. Call it right after the vblank.
void gfx_present(void);

void gfx_clear(enum screen s, enum color color);
// Rectangles have slightly rounded corners.
void gfx_rect(enum screen s, int x, int y, int w, int h, enum color color);
void gfx_frame(enum screen s, int x, int y, int w, int h, int thickness, enum color color);

// Draws pixel art given as rows of '#' and '.'.
void gfx_art(enum screen s, int x, int y, const char *const *rows, int count, int scale,
             enum color color);

// Draws a Latin-1 letter in uppercase, centered on (x, y) by its capital
// height, so accents stick out above.
void gfx_letter(enum screen s, int x, int y, int c, enum font_size size, enum color color);
// Draws UTF-8 text centered on x, with its baseline at y.
void gfx_text(enum screen s, int x, int y, const char *text, enum font_size size, enum color color);

#endif
