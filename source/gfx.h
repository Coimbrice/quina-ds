#ifndef GFX_H
#define GFX_H

#include <stdbool.h>
#include <stdint.h>

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
// Switches the hint colors between the usual ones and high contrast.
void gfx_set_high_contrast(bool on);
// Copies what was drawn to the screens. Call it right after the vblank.
void gfx_present(void);
// Fades to black, or back from black showing what was drawn. Either both
// screens fade or only the top one.
void gfx_fade_out(bool top_only);
void gfx_fade_in(bool top_only);

// Copies a 64x64 area into the graphics of an 8bpp sprite. Pixels with the
// background color become transparent.
void gfx_capture(enum screen s, int x, int y, uint16_t *sprite);

void gfx_clear(enum screen s, enum color color);
// Limits drawing to a rectangle until gfx_unclip is called.
void gfx_clip(int x, int y, int w, int h);
void gfx_unclip(void);

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
int gfx_text_width(const char *text, enum font_size size);

#define BUTTON_FOCUSED 1
#define BUTTON_PRESSED 2

// A box with a centered label. flags can make it outlined as focused or
// sunk as pressed.
void gfx_button(enum screen s, int x, int y, int w, int h, const char *text, enum font_size size,
                enum color fill, int flags);

#endif
