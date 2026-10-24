#ifndef MENU_H
#define MENU_H

#include <stdbool.h>

enum menu_choice {
    MENU_NONE,
    MENU_DAILY,
    MENU_FREE,
};

extern bool sound_effects_on;
extern bool music_on;

// Shows the menu, starting from the "press START" screen if title is set.
void menu_start(bool title);
// Handles one frame of input. Returns the mode the player picked, if any.
enum menu_choice menu_update(void);

#endif
