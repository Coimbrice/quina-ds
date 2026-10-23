#include <stdint.h>
#include <string.h>

#include <nds.h>

#include "font.h"
#include "gfx.h"

#define RGB(hex) RGB15(((hex) >> 19) & 31, ((hex) >> 11) & 31, ((hex) >> 3) & 31)

// clang-format off
static const uint16_t palette[COLOR_COUNT] = {
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

// Antialiased text needs colors between the text and whatever is under it.
// They're added to the palette the first time each pair is drawn.
static uint16_t colors[256];
static int color_count;
static uint8_t shades[COLOR_COUNT][256][2];

void gfx_init(void)
{
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    bg_ids[TOP] = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    bg_ids[BOTTOM] = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);

    for (int i = 0; i < COLOR_COUNT; i++) {
        colors[i] = palette[i];
        BG_PALETTE[i] = palette[i];
        BG_PALETTE_SUB[i] = palette[i];
    }
    color_count = COLOR_COUNT;
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

static int shade(enum color fg, int bg, int level)
{
    if (level == 3)
        return fg;
    if (!shades[fg][bg][level - 1]) {
        if (color_count == 256)
            return level == 2 ? fg : bg;

        int a = colors[fg];
        int b = colors[bg];
        int mixed = 0;
        for (int shift = 0; shift < 15; shift += 5) {
            int ca = (a >> shift) & 31;
            int cb = (b >> shift) & 31;
            mixed |= ((ca * level + cb * (3 - level)) / 3) << shift;
        }

        colors[color_count] = mixed;
        BG_PALETTE[color_count] = mixed;
        BG_PALETTE_SUB[color_count] = mixed;
        shades[fg][bg][level - 1] = color_count++;
    }
    return shades[fg][bg][level - 1];
}

// Draws a glyph with the pen at (x, y) on the baseline.
static void draw_glyph(enum screen s, int x, int y, const struct font *font, int c,
                       enum color color)
{
    const struct glyph *g = &font->glyphs[c];
    const uint8_t *pixels = &font->pixels[g->offset];

    for (int row = 0; row < g->h; row++) {
        int py = y + g->y + row;
        for (int col = 0; col < g->w; col++) {
            int px = x + g->x + col;
            int i = row * g->w + col;
            int level = (pixels[i / 4] >> (i % 4 * 2)) & 3;
            if (level && px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
                uint8_t *p = &buffers[s][py * SCREEN_W + px];
                *p = shade(color, *p, level);
            }
        }
    }
}

void gfx_letter(enum screen s, int x, int y, int c, enum font_size size, enum color color)
{
    const struct font *font = &fonts[size];

    if (c >= 'a' && c <= 'z')
        c -= 'a' - 'A';
    // Latin-1 lowercase accented letters are 0x20 above their uppercase.
    if (c >= 0xE0 && c != 0xF7 && c != 0xFF)
        c -= 0x20;

    const struct glyph *g = &font->glyphs[c];
    draw_glyph(s, x - g->x - g->w / 2, y + font->cap_height / 2, font, c, color);
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

void gfx_text(enum screen s, int x, int y, const char *text, enum font_size size, enum color color)
{
    const struct font *font = &fonts[size];
    int width = 0;

    for (const char *p = text; *p;)
        width += font->glyphs[next_char(&p)].advance;

    x -= width / 2;
    while (*text) {
        int c = next_char(&text);
        draw_glyph(s, x, y, font, c, color);
        x += font->glyphs[c].advance;
    }
}
