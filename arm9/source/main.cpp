#include <nds.h>
#include "game.h"

int main() {
    // Hardware power-on
    powerOn(POWER_ALL_2D);

    // Map screens: top = main engine, bottom = sub engine
    lcdMainOnTop();

    // Initialize game systems
    gameInit();

    // Main loop — fixed timestep, VBlank-gated
    while (true) {
        scanKeys();
        gameUpdate();
        gameRender();
        swiWaitForVBlank();
    }

    return 0;
}
