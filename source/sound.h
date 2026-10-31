#ifndef SOUND_H
#define SOUND_H

#include <stdbool.h>

enum sound {
    SOUND_CLICK,
    SOUND_SUBMIT,
    SOUND_ERROR,
    SOUND_OPEN,
    SOUND_CLOSE,
    SOUND_ON,
    SOUND_OFF,
};

void sound_init(void);
// Plays a sound effect, unless they're turned off in the options.
void sound_play(enum sound s);
// Plays the fanfare for a game won or lost over the music.
void sound_fanfare(bool won);
// Starts or stops the music to follow the options.
void sound_update_music(void);
// Call once per frame, to bring the music back after a fanfare.
void sound_update(void);

#endif
