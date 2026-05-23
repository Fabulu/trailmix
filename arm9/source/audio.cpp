#include "audio.h"

// Audio is stubbed until soundbank assets are added.
// When assets exist, include <maxmod9.h> and implement properly.

void audioInit() {
    // TODO: mmInitDefaultMem((mm_addr)soundbank_bin);
}

void audioPlaySfx([[maybe_unused]] SfxId id) {
    // TODO: mmEffect(id);
}

void audioPlayMusic([[maybe_unused]] int id) {
    // TODO: mmStart(id, MM_PLAY_LOOP);
}

void audioStopMusic() {
    // TODO: mmStop();
}
