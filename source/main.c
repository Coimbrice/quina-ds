#include <stdlib.h>
#include <time.h>

#include <nds.h>

#include "gfx.h"
#include "keyboard.h"
#include "play.h"

int main(void)
{
    gfx_init();
    keyboard_init();
    keysSetRepeat(20, 6);
    srand(time(NULL));

    play_start(1);

    while (1) {
        swiWaitForVBlank();
        gfx_present();
        scanKeys();
        play_update();
    }

    return 0;
}
