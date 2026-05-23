#include "game.h"
#include "player.h"
#include "entities.h"
#include "combat.h"
#include "pills.h"
#include "render.h"
#include "ui.h"
#include "audio.h"
#include "rng.h"
#include <nds.h>

static GameState state = GameState::Menu;
DTCM_BSS static u32 frameCounter;
static u16 waveTimer;
static u8 waveNumber;

// --- State transitions ---

static void enterMenu() {
    state = GameState::Menu;
}

static void enterPlay() {
    state = GameState::Play;
    entitiesClear();
    playerInit();
    pillsReset();
    frameCounter = 0;
    waveTimer = 0;
    waveNumber = 0;
}

static void resumePlay() {
    state = GameState::Play;
}

static void enterUpgrade() {
    state = GameState::Upgrade;
    uiGenerateChoices();
}

static void enterGameOver() {
    state = GameState::GameOver;
}

// --- Wave spawning ---

static void spawnWave() {
    waveNumber++;
    int count = 3 + waveNumber;
    if (count > MAX_ENEMIES) count = MAX_ENEMIES;

    for (int i = 0; i < count; ++i) {
        // Spawn from screen edges
        Vec2 pos;
        int edge = rngRange(4);
        switch (edge) {
            case 0: pos = {toFixed(rngRange(SCREEN_W)), toFixed(-16)}; break;
            case 1: pos = {toFixed(rngRange(SCREEN_W)), toFixed(SCREEN_H + 16)}; break;
            case 2: pos = {toFixed(-16), toFixed(rngRange(SCREEN_H))}; break;
            default: pos = {toFixed(SCREEN_W + 16), toFixed(rngRange(SCREEN_H))}; break;
        }

        // Move toward center with some randomness
        Vec2 center = {toFixed(SCREEN_W / 2), toFixed(SCREEN_H / 2)};
        Vec2 dir = center - pos;

        // Simple normalize: just set a fixed speed toward center
        Fixed spd = toFixed(1) + static_cast<Fixed>(rngRange(FP_ONE));
        Vec2 vel = {0, 0};
        if (dir.x > 0) vel.x = spd; else if (dir.x < 0) vel.x = static_cast<Fixed>(-spd);
        if (dir.y > 0) vel.y = spd; else if (dir.y < 0) vel.y = static_cast<Fixed>(-spd);

        s16 hp = static_cast<s16>(2 + waveNumber);
        u8 type = static_cast<u8>(rngRange(4));
        u8 size = static_cast<u8>(12 + rngRange(8));

        spawnEnemy(pos, vel, hp, type, size);
    }
}

// --- Per-state update ---

static void updateMenu() {
    if (keysDown() & KEY_START) {
        enterPlay();
    }
}

static void updatePlay() {
    frameCounter++;

    playerUpdate(keysHeld());
    combatUpdate();
    pillsApplySynergies();

    // Spawn waves periodically
    waveTimer++;
    u16 waveInterval = 180; // 3 seconds at 60fps
    if (waveTimer >= waveInterval) {
        waveTimer = 0;
        spawnWave();
    }

    // Check for upgrade trigger (every 5 waves)
    if (waveNumber > 0 && waveNumber % 5 == 0 && waveTimer == 1) {
        enterUpgrade();
    }

    // Check death
    if (gPlayer.hp <= 0) {
        enterGameOver();
    }
}

static void updateUpgrade() {
    uiUpdate();
    int sel = uiGetSelection();
    if (sel >= 0) {
        // Apply upgrade — give a pill at the player's current slot tier
        PillColor color = static_cast<PillColor>(sel % PILL_COLOR_COUNT);
        PillTier currentTier = gPlayer.inventory[static_cast<int>(color)].tier;
        playerCollectPill(color, currentTier);
        resumePlay();
    }
}

static void updateGameOver() {
    if (keysDown() & KEY_START) {
        enterMenu();
    }
}

// --- Per-state render ---

static void renderPlayState() {
    renderGameplay();
    uiRenderHUD();
}

static void renderUpgradeState() {
    renderGameplay(); // Keep gameplay visible on top screen
    uiRenderUpgradeScreen();
}

// --- Function pointer tables ---

using StateFunc = void (*)();

static const StateFunc updateTable[static_cast<int>(GameState::COUNT)] = {
    updateMenu,
    updatePlay,
    updateUpgrade,
    updateGameOver,
};

static const StateFunc renderTable[static_cast<int>(GameState::COUNT)] = {
    renderMenu,
    renderPlayState,
    renderUpgradeState,
    renderGameOver,
};

// --- Public API ---

void gameInit() {
    rngSeed(42); // TODO: seed from RTC or user input
    entitiesInit();
    playerInit();
    renderInit();
    uiInit();
    audioInit();
    enterMenu();
}

void gameUpdate() {
    updateTable[static_cast<int>(state)]();
}

void gameRender() {
    renderTable[static_cast<int>(state)]();
}

GameState gameGetState() {
    return state;
}
