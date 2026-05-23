// Stub for audio.h — provides no-op implementations so player.cpp links.
#ifndef AUDIO_STUB_H
#define AUDIO_STUB_H

#include "nds_stub.h"

enum SfxId {
    SFX_SHOOT   = 0,
    SFX_EXPLODE = 1,
    SFX_PICKUP  = 2,
    SFX_MERGE   = 3,
    SFX_HIT     = 4,
    SFX_SELECT  = 5,
    SFX_COUNT
};

inline void audioInit() {}
inline void audioPlaySfx(SfxId) {}
inline void audioPlayMusic(int) {}
inline void audioStopMusic() {}

#endif // AUDIO_STUB_H
