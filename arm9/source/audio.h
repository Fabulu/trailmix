#ifndef AUDIO_H
#define AUDIO_H

#include <nds.h>

// Game SFX IDs — prefixed with GSFX_ to avoid collision with soundbank.h #defines
enum SfxId {
    GSFX_SHOOT   = 0,
    GSFX_EXPLODE = 1,
    GSFX_GOLD    = 2,
    GSFX_MERGE   = 3,
    GSFX_HIT     = 4,
    GSFX_SELECT  = 5,
    GSFX_DASH    = 6,
    GSFX_BUY     = 7,
    GSFX_SELL    = 8,
    GSFX_REROLL  = 9,
    GSFX_WAVE_CLEAR = 10,
    GSFX_WAVE_START = 11,
    GSFX_PLAYER_HIT = 12,
    GSFX_PERK    = 13,
    GSFX_BOSS    = 14,
    GSFX_TIMESTOP = 15,
    GSFX_HEAL    = 16,
    GSFX_SYNERGY = 17,
    GSFX_COUNT
};

void audioInit();
void audioPlaySfx(SfxId id);
void audioPlayMusic(int id);
void audioStopMusic();

#endif // AUDIO_H
