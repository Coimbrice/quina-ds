#include <stdlib.h>
#include <time.h>

#include <fat.h>
#include <nds.h>

#include "gfx.h"
#include "keyboard.h"
#include "menu.h"
#include "play.h"
#include "save.h"

#define SAVE_PATH "/data/quina/save.dat"

int main(void)
{
    bool playing = false;

    gfx_init();
    keyboard_init();
    keysSetRepeat(20, 6);
    srand(time(NULL));

    // Without an SD card or flashcart the game still runs, it just can't save.
    save_load(fatInitDefault() ? SAVE_PATH : NULL);

    setBrightness(3, -16);
    menu_start(true);
    gfx_fade_in(false);

    while (1) {
        swiWaitForVBlank();
        gfx_present();
        scanKeys();

        if (playing) {
            if (play_update()) {
                gfx_fade_out(false);
                menu_start(false);
                gfx_fade_in(false);
                playing = false;
            }
        } else {
            enum menu_choice choice = menu_update();
            if (choice != MENU_NONE) {
                gfx_fade_out(false);
                play_open(choice == MENU_DAILY);
                gfx_fade_in(false);
                playing = true;
            }
        }
    }

    return 0;
}
