#ifndef FONT_H
#define FONT_H

#include <stdbool.h>

#define GLYPH_W 5
#define GLYPH_H 7
#define MARK_H 2

enum mark {
    MARK_NONE,
    MARK_ACUTE,
    MARK_GRAVE,
    MARK_CIRCUMFLEX,
    MARK_TILDE,
    MARK_DIAERESIS,
    MARK_CEDILLA,
};

// Returns the rows of a Latin-1 character drawn in uppercase, or NULL if
// there's no glyph for it. Accented letters also return the rows of their
// mark, which goes above the letter, or below it for the cedilla.
const char *const *font_glyph(int c, const char *const **mark, bool *below);

#endif
