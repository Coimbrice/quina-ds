#include <stdint.h>
#include <string.h>

#include <nds.h>

#include "font.h"
#include "gfx.h"

#define RGB(hex) RGB15(((hex) >> 19) & 31, ((hex) >> 11) & 31, ((hex) >> 3) & 31)

// clang-format off
static const uint16_t palette[] = {
    [COLOR_BG] = RGB(0x6e5c62),
    [COLOR_EMPTY] = RGB(0x615458),
    [COLOR_BORDER] = RGB(0x4c4347),
    [COLOR_ABSENT] = RGB(0x312a2c),
    [COLOR_PRESENT] = RGB(0xd3ad69),
    [COLOR_CORRECT] = RGB(0x3aa394),
    [COLOR_KEY] = RGB(0x4c4347),
    [COLOR_TEXT] = RGB(0xfafaff),
    [COLOR_DIM_TEXT] = RGB(0xb7aeb4),
};
// clang-format on

// Drawing happens in main RAM because VRAM can't take 8-bit writes.
static uint8_t buffers[2][SCREEN_W * SCREEN_H] ALIGN(32);
static int bg_ids[2];

void gfx_init(void)
{
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    bg_ids[TOP] = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    bg_ids[BOTTOM] = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);

    for (unsigned i = 0; i < sizeof(palette) / sizeof(palette[0]); i++) {
        BG_PALETTE[i] = palette[i];
        BG_PALETTE_SUB[i] = palette[i];
    }
}

void gfx_present(void)
{
    DC_FlushRange(buffers, sizeof(buffers));
    dmaCopyWords(3, buffers[TOP], bgGetGfxPtr(bg_ids[TOP]), sizeof(buffers[TOP]));
    dmaCopyWords(3, buffers[BOTTOM], bgGetGfxPtr(bg_ids[BOTTOM]), sizeof(buffers[BOTTOM]));
}

void gfx_clear(enum screen s, enum color color)
{
    memset(buffers[s], color, sizeof(buffers[s]));
}

static void fill(enum screen s, int x, int y, int w, int h, enum color color)
{
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > SCREEN_W)
        w = SCREEN_W - x;
    if (y + h > SCREEN_H)
        h = SCREEN_H - y;
    if (w <= 0 || h <= 0)
        return;

    for (int row = y; row < y + h; row++)
        memset(&buffers[s][row * SCREEN_W + x], color, w);
}

void gfx_rect(enum screen s, int x, int y, int w, int h, enum color color)
{
    fill(s, x + 2, y, w - 4, 1, color);
    fill(s, x + 1, y + 1, w - 2, 1, color);
    fill(s, x, y + 2, w, h - 4, color);
    fill(s, x + 1, y + h - 2, w - 2, 1, color);
    fill(s, x + 2, y + h - 1, w - 4, 1, color);
}

void gfx_frame(enum screen s, int x, int y, int w, int h, int thickness, enum color color)
{
    int t = thickness;

    fill(s, x + 2, y, w - 4, t, color);
    fill(s, x + 2, y + h - t, w - 4, t, color);
    fill(s, x, y + 2, t, h - 4, color);
    fill(s, x + w - t, y + 2, t, h - 4, color);
    fill(s, x + 1, y + 1, t, t, color);
    fill(s, x + w - 1 - t, y + 1, t, t, color);
    fill(s, x + 1, y + h - 1 - t, t, t, color);
    fill(s, x + w - 1 - t, y + h - 1 - t, t, t, color);
}

void gfx_art(enum screen s, int x, int y, const char *const *rows, int count, int scale,
             enum color color)
{
    for (int row = 0; row < count; row++) {
        for (int col = 0; rows[row][col]; col++) {
            if (rows[row][col] == '#')
                fill(s, x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

void gfx_char(enum screen s, int x, int y, int c, int scale, enum color color)
{
    const char *const *mark;
    bool below;
    const char *const *glyph = font_glyph(c, &mark, &below);

    if (!glyph)
        return;

    gfx_art(s, x, y, glyph, GLYPH_H, scale, color);
    if (mark && below)
        gfx_art(s, x, y + GLYPH_H * scale, mark, MARK_H, scale, color);
    else if (mark)
        gfx_art(s, x, y - (MARK_H + 1) * scale, mark, MARK_H, scale, color);
}

// Reads one character from UTF-8 text, returning it as Latin-1.
static int next_char(const char **text)
{
    const unsigned char *p = (const unsigned char *)*text;
    int c = *p++;

    if ((c & 0xE0) == 0xC0 && (*p & 0xC0) == 0x80) {
        c = ((c & 0x1F) << 6) | (*p++ & 0x3F);
    } else if (c >= 0x80) {
        while ((*p & 0xC0) == 0x80)
            p++;
        c = '?';
    }
    *text = (const char *)p;
    return c;
}

void gfx_text(enum screen s, int x, int y, const char *text, int scale, enum color color)
{
    int advance = (GLYPH_W + 1) * scale;
    int length = 0;

    for (const char *p = text; *p; length++)
        next_char(&p);

    x -= (length * advance - scale) / 2;
    while (*text) {
        gfx_char(s, x, y, next_char(&text), scale, color);
        x += advance;
    }
}
