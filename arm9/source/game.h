#ifndef GAME_H
#define GAME_H

#include <nds/ndstypes.h>

enum class GameState : int {
    Menu       = 0,
    Play       = 1,
    PerkChoice = 2,
    Shop       = 3,
    GameOver   = 4,
    COUNT
};

void gameInit();
void gameUpdate();
void gameRender();
GameState gameGetState();
int gameGetWave();
int gameGetWaveClearTimer();  // frames remaining in "WAVE CLEAR!" banner (0 = done)
int gameGetShopInterest();    // interest earned on this shop entry
int gameGetWaveTimerSecs();   // seconds remaining in wave timer
u8 gameGetWaveTimerPct();     // 0-255 wave timer progress
u8 gameGetAnnounceTimer();    // frames remaining in wave announce
u8 gameGetBossDefeatedTimer(); // frames remaining in "BOSS DEFEATED!" banner (0 = done)

#endif // GAME_H
