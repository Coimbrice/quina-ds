#include <stdint.h>

#include <nds.h>

#include "board.h"
#include "gfx.h"
#include "logo.h"

#define LETTERS 5
#define TILE 42
#define GAP 6
#define CENTER_Y 73
#define SPRITE 64

// Each letter bobs and tilts on a sine wave, a bit later than the one
// before it. Speeds are in angle units per frame, 32768 to a full turn.
#define BOB_SPEED 270
#define BOB_PIXELS 3
#define TILT_SPEED 180
#define TILT_ANGLE degreesToAngle(5)
#define LETTER_PHASE 5000

static const enum color colors[LETTERS] = {
    COLOR_CORRECT, COLOR_PRESENT, COLOR_ABSENT, COLOR_CORRECT, COLOR_PRESENT,
};

static uint16_t *sprites[LETTERS];
static bool shown;

void logo_init(void)
{
    for (int i = 0; i < LETTERS; i++) {
        sprites[i] = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_256Color);
        gfx_clear(TOP, COLOR_BG);
        draw_tile(TOP, (SPRITE - TILE) / 2, (SPRITE - TILE) / 2, TILE, TILE, FONT_TITLE, "QUINA"[i],
                  colors[i], false, false);
        gfx_capture(TOP, 0, 0, sprites[i]);
    }
    gfx_clear(TOP, COLOR_BG);
}

void logo_show(void)
{
    shown = true;
}

void logo_hide(void)
{
    shown = false;
    oamClear(&oamMain, 0, LETTERS);
}

void logo_update(int frame)
{
    int left = (SCREEN_W - LETTERS * TILE - (LETTERS - 1) * GAP) / 2 + TILE / 2;

    if (!shown)
        return;

    for (int i = 0; i < LETTERS; i++) {
        int bob = sinLerp((int16_t)(frame * BOB_SPEED + i * LETTER_PHASE)) * BOB_PIXELS >> 12;
        int tilt = sinLerp((int16_t)(frame * TILT_SPEED + i * LETTER_PHASE)) * TILT_ANGLE >> 12;
        int x = left + i * (TILE + GAP) - SPRITE / 2;

        oamRotateScale(&oamMain, i, tilt, 1 << 8, 1 << 8);
        oamSet(&oamMain, i, x, CENTER_Y - SPRITE / 2 + bob, 0, 0, SpriteSize_64x64,
               SpriteColorFormat_256Color, sprites[i], i, false, false, false, false, false);
    }
}
