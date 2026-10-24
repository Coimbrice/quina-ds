#include <stdlib.h>
#include <time.h>

#include <nds.h>

#include "gfx.h"
#include "keyboard.h"
#include "menu.h"
#include "play.h"

int main(void)
{
    bool playing = false;

    gfx_init();
    keyboard_init();
    keysSetRepeat(20, 6);
    srand(time(NULL));

    setBrightness(3, -16);
    menu_start(true);
    gfx_fade_in();

    while (1) {
        swiWaitForVBlank();
        gfx_present();
        scanKeys();

        if (playing) {
            if (play_update()) {
                gfx_fade_out();
                menu_start(false);
                gfx_fade_in();
                playing = false;
            }
        } else if (menu_update() != MENU_NONE) {
            gfx_fade_out();
            play_start(1);
            gfx_fade_in();
            playing = true;
        }
    }

    return 0;
}
