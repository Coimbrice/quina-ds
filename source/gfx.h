#ifndef GFX_H
#define GFX_H

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

// Draws a Latin-1 character in uppercase. y is the top of the letter body;
// accents go above it.
void gfx_char(enum screen s, int x, int y, int c, int scale, enum color color);
// Draws UTF-8 text in uppercase, centered on x.
void gfx_text(enum screen s, int x, int y, const char *text, int scale, enum color color);

#endif
