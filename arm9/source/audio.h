#ifndef AUDIO_H
#define AUDIO_H

#include <nds.h>

// SFX IDs — will map to soundbank entries once assets are added
enum SfxId {
    SFX_SHOOT   = 0,
    SFX_EXPLODE = 1,
    SFX_PICKUP  = 2,
    SFX_MERGE   = 3,
    SFX_HIT     = 4,
    SFX_SELECT  = 5,
    SFX_COUNT
};

void audioInit();
void audioPlaySfx(SfxId id);
void audioPlayMusic(int id);
void audioStopMusic();

#endif // AUDIO_H
