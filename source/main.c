#include <stdio.h>

#include <nds.h>

int main(void)
{
    PrintConsole top;
    PrintConsole bottom;

    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleInit(&top, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    consoleInit(&bottom, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

    consoleSelect(&top);
    printf("Quina\n");

    consoleSelect(&bottom);
    printf("Tela de baixo.\n\n");
    printf("Aperte START para sair.\n");

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        if (keysDown() & KEY_START)
            break;
    }

    return 0;
}
