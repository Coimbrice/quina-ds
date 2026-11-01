#include <stdlib.h>
#include <time.h>

#include <fat.h>
#include <nds.h>

#include "gfx.h"
#include "keyboard.h"
#include "logo.h"
#include "menu.h"
#include "play.h"
#include "save.h"
#include "sound.h"

#define SAVE_PATH "/data/quina/save.dat"

// The license notices that ship in the ROM. See tools/licenses.py.
extern const char licenses[];

int main(void)
{
    bool playing = false;

    // Nothing reads the notices, so the linker would drop them without this.
    __asm__ volatile("" : : "r"(licenses));

    gfx_init();
    keyboard_init();
    logo_init();
    keysSetRepeat(20, 6);
    srand(time(NULL));

    // Without an SD card or flashcart the game still runs, it just can't save.
    save_load(fatInitDefault() ? SAVE_PATH : NULL);
    sound_init();
    gfx_set_high_contrast(save.high_contrast);

    setBrightness(3, -16);
    menu_start(true);
    gfx_fade_in(false);

    while (1) {
        swiWaitForVBlank();
        gfx_present();
        scanKeys();
        sound_update();

        if (playing) {
            if (play_update()) {
                gfx_fade_out(false);
                menu_start(false);
                gfx_fade_in(false);
                playing = false;
            }
        } else {
            enum menu_choice choice = menu_update();
            if (choice == MENU_QUIT)
                break;
            if (choice != MENU_NONE) {
                gfx_fade_out(false);
                menu_close();
                play_open(choice == MENU_DAILY);
                gfx_fade_in(false);
                playing = true;
            }
        }
    }

    // Returning goes back to the loader that started the game, or turns the
    // console off when there's none.
    gfx_fade_out(false);
    return 0;
}
