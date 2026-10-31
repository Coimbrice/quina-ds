#include <stdlib.h>

#include <nds.h>
#include <maxmod9.h>

#include "save.h"
#include "sound.h"
#include "soundbank.h"
#include "soundbank_bin.h"

#define MUSIC_VOLUME 160
#define FANFARE_VOLUME 200
#define FADE_FRAMES 60

// Lengths of the fanfares in frames, while the music stays quiet.
#define CLEARED_FRAMES 282
#define GAMEOVER_FRAMES 354

static const mm_word effects[] = {
    SFX_CLICK_002,  SFX_CLICK_003,  SFX_BONG_001, SFX_ERROR_004, SFX_MAXIMIZE_008, SFX_MINIMIZE_008,
    SFX_SELECT_001, SFX_SELECT_002, SFX_MUSIC,    SFX_CLEARED,   SFX_GAMEOVER,
};

static mm_sfxhand music = MM_SFXHAND_INVALID;
static mm_sfxhand fanfare = MM_SFXHAND_INVALID;
static int quiet_frames;
static int fade_frames;

void sound_init(void)
{
    mmInitDefaultMem((mm_addr)soundbank_bin);
    for (unsigned i = 0; i < sizeof(effects) / sizeof(effects[0]); i++)
        mmLoadEffect(effects[i]);
    sound_update_music();
}

static void cancel(mm_sfxhand *handle)
{
    if (*handle != MM_SFXHAND_INVALID)
        mmEffectCancel(*handle);
    *handle = MM_SFXHAND_INVALID;
}

static void play(mm_word id, int volume)
{
    mm_sound_effect effect = {
        .id = id,
        .rate = 1024,
        .volume = volume,
        .panning = 128,
    };
    mmEffectRelease(mmEffectEx(&effect));
}

void sound_play(enum sound s)
{
    if (!save.sound_effects)
        return;

    switch (s) {
    case SOUND_CLICK:
        // One of two clicks at random, so typing doesn't sound mechanical.
        play(rand() % 2 ? SFX_CLICK_003 : SFX_CLICK_002, 255);
        break;
    case SOUND_SUBMIT:
        play(SFX_BONG_001, 255);
        break;
    case SOUND_ERROR:
        play(SFX_ERROR_004, 255);
        break;
    case SOUND_OPEN:
        play(SFX_MAXIMIZE_008, 255);
        break;
    case SOUND_CLOSE:
        play(SFX_MINIMIZE_008, 255);
        break;
    case SOUND_ON:
        play(SFX_SELECT_001, 255);
        break;
    case SOUND_OFF:
        play(SFX_SELECT_002, 255);
        break;
    }
}

void sound_fanfare(bool won)
{
    mm_sound_effect effect = {
        .id = won ? SFX_CLEARED : SFX_GAMEOVER,
        .rate = 1024,
        .volume = FANFARE_VOLUME,
        .panning = 128,
    };

    if (!save.music)
        return;

    cancel(&fanfare);
    fanfare = mmEffectEx(&effect);
    mmEffectVolume(music, 0);
    quiet_frames = won ? CLEARED_FRAMES : GAMEOVER_FRAMES;
    fade_frames = 0;
}

void sound_update_music(void)
{
    mm_sound_effect effect = {
        .id = SFX_MUSIC,
        .rate = 1024,
        .volume = MUSIC_VOLUME,
        .panning = 128,
    };

    if (save.music && music == MM_SFXHAND_INVALID) {
        music = mmEffectEx(&effect);
    } else if (!save.music && music != MM_SFXHAND_INVALID) {
        cancel(&music);
        cancel(&fanfare);
        quiet_frames = 0;
        fade_frames = 0;
    }
}

void sound_update(void)
{
    if (quiet_frames > 0 && --quiet_frames == 0) {
        fanfare = MM_SFXHAND_INVALID;
        fade_frames = FADE_FRAMES;
    }
    if (fade_frames > 0) {
        fade_frames--;
        mmEffectVolume(music, MUSIC_VOLUME * (FADE_FRAMES - fade_frames) / FADE_FRAMES);
    }
}
