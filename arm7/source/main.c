// Default ARM7 stub — handles sound, wifi, RTC, touchscreen
// For most DS homebrew, the default ARM7 binary from devkitPro/BlocksDS
// is sufficient. This file is a minimal placeholder.
//
// If using devkitPro: link against -lnds7 and the default_arm7 template
// If using BlocksDS: the default ARM7 is provided automatically

#include <nds.h>

void VblankHandler(void) {
    inputGetAndSend();
}

void VcountHandler() {
    inputGetAndSend();
}

int main() {
    irqInit();
    fifoInit();

    readUserSettings();

    SetYtrigger(80);

    installSoundFIFO();
    installSystemFIFO();

    irqSet(IRQ_VCOUNT, VcountHandler);
    irqSet(IRQ_VBLANK, VblankHandler);

    irqEnable(IRQ_VBLANK | IRQ_VCOUNT);

    // Keep the ARM7 idle — wake on interrupts
    while (1) {
        swiWaitForVBlank();
    }

    return 0;
}
