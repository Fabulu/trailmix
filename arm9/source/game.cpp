#include "game.h"
#include "player.h"
#include "entities.h"
#include "combat.h"
#include "camera.h"
#include "render.h"
#include "ui.h"
#include "audio.h"
#include "rng.h"
#include "companion.h"
#include "shop.h"
#include "perk.h"
#include "perk_choice.h"
#include "synergy.h"
#include <nds.h>

static GameState state = GameState::Menu;
DTCM_BSS static u32 frameCounter;
static u8 waveNumber;
static bool waveActive = false;

// Wave-clear banner: counts down 60 frames before entering Shop/PerkChoice
static u8 waveClearTimer = 0;

// "BOSS DEFEATED!" banner: counts down 90 frames before entering PerkChoice
static u8 bossDefeatedTimer = 0;

static u8 shopInterestEarned = 0; // displayed in shop screen

// Wave timer
static u32 gWaveTimer;          // counts down in frames
static u32 gWaveTimerMax;       // set at wave start
static u8  waveAnnounceTimer;   // 90 frames countdown for "WAVE X" display
static bool overflowSpawned;

// Forward declaration
static void spawnWave();

static u32 waveTimerFrames(int wave) {
    int seconds = 15 + (300 / (10 + (wave - 1)));
    return static_cast<u32>(seconds * 60);
}

// --- State transitions ---

static void enterMenu() {
    state = GameState::Menu;
    renderClearSub();
    audioPlayMusic(0); // MOD_MENU
}

static void enterPlay() {
    state = GameState::Play;
    entitiesClear();
    playerInit();
    companionInit();  // reset companions on new game
    perkInit();       // reset perks on new game
    synergyInit();    // reset synergy state on new game
    cameraInit();
    frameCounter = 0;
    waveNumber = 0;
    waveActive = false;
    // Reset shop state so locks/selections don't carry over between runs
    gShop.lockTarget = -1;
    gShop.selectedCard = -1;
    gShop.selectedCompanion = -1;
    gShop.rerollCount = 0;
    gShop.errorCard = -1;
    gShop.errorTimer = 0;
    for (int i = 0; i < SHOP_CARDS; i++) gShop.cards[i].locked = false;
    // Start wave 1 immediately so there's no empty frame
    spawnWave();
    waveAnnouncementStart(waveNumber);
    waveActive = true;
    uiForceRedraw();
    audioPlayMusic(1); // MOD_BATTLE
}

static void applyInterest() {
    int interest = gPlayer.gold / 4;  // 25% interest rate (breakpoints at 4/8/12/16/20g)
    int cap = perkInterestCap();
    // Late-game interest growth
    int wave = gameGetWave();
    if (wave > 20) cap += (wave - 20) / 5;
    if (interest > cap) interest = cap;
    shopInterestEarned = static_cast<u8>(interest);
    gPlayer.gold = static_cast<u16>(gPlayer.gold + interest);
}

static void enterPerkChoice() {
    state = GameState::PerkChoice;
    bossDefeatedTimer = 0;
    perkChoiceEnter();
    // Force immediate draw of both screens to avoid flicker
    perkChoiceRender();
    // Keep battle music going while player chooses — feels snappier
}

static void enterShop() {
    state = GameState::Shop;
    renderClearSub();
    applyInterest();

    // Pension: +2g per living companion at wave end
    if (perkIsActive(PERK_PENSION)) {
        gPlayer.gold = static_cast<u16>(gPlayer.gold + companionCount() * 2);
    }

    // Soul Surge: heal all units 20% maxHP on wave clear
    perkOnWaveClear();

    // Wave-end heal: restore 50% of maxHp to every active companion (capped at maxHp)
    for (auto& c : gCompanions) {
        if (!c.active) continue;
        c.hp += c.maxHp / 2;
        if (c.hp > c.maxHp) c.hp = c.maxHp;
    }

    shopGenerate(waveNumber);
    audioPlayMusic(4); // MOD_SHOP
}

static void startNextWave() {
    renderClearSub();
    uiForceRedraw();
    audioPlayMusic(1); // MOD_BATTLE
    perkOnWaveStart();
    entitiesClear();
    spawnWave();
    waveAnnouncementStart(waveNumber);
    waveActive = true;
    state = GameState::Play;
}

static void enterGameOver() {
    state = GameState::GameOver;
    renderClearSub();
    audioPlayMusic(0); // MOD_MENU
}

static void enterVictory() {
    state = GameState::Victory;
    renderClearSub();
    audioPlaySfx(GSFX_VICTORY);
    audioPlayMusic(0); // MOD_MENU
}

// --- Wave-clear detection ---

static bool allEnemiesDead() {
    for (auto& e : gEnemies) if (e.active) return false;
    return true;
}

// --- Wave spawning ---

// Helper: generate a spawn position on a given edge, with spacing index for formations
static Vec2 edgePos(int edge, int index, int total) {
    int span;
    int offset;
    switch (edge) {
        case 0: // top
            span = ARENA_W - WALL_THICK * 2;
            offset = (total > 1) ? (WALL_THICK + (span * index) / (total - 1)) : (ARENA_W / 2);
            return {toFixed(offset), toFixed(WALL_THICK)};
        case 1: // bottom
            span = ARENA_W - WALL_THICK * 2;
            offset = (total > 1) ? (WALL_THICK + (span * index) / (total - 1)) : (ARENA_W / 2);
            return {toFixed(offset), toFixed(ARENA_H - WALL_THICK)};
        case 2: // left
            span = ARENA_H - WALL_THICK * 2;
            offset = (total > 1) ? (WALL_THICK + (span * index) / (total - 1)) : (ARENA_H / 2);
            return {toFixed(WALL_THICK), toFixed(offset)};
        default: // right
            span = ARENA_H - WALL_THICK * 2;
            offset = (total > 1) ? (WALL_THICK + (span * index) / (total - 1)) : (ARENA_H / 2);
            return {toFixed(ARENA_W - WALL_THICK), toFixed(offset)};
    }
}

// Helper: force position to farthest arena edge from player
static Vec2 farthestEdgeFromPlayer() {
    int px = toInt(gPlayer.pos.x);
    int py = toInt(gPlayer.pos.y);
    int dTop   = py - WALL_THICK;
    int dBot   = (ARENA_H - WALL_THICK) - py;
    int dLeft  = px - WALL_THICK;
    int dRight = (ARENA_W - WALL_THICK) - px;

    int best = dTop;
    Vec2 pos = {toFixed(ARENA_W / 2), toFixed(WALL_THICK)};
    if (dBot   > best) { best = dBot;   pos = {toFixed(ARENA_W / 2), toFixed(ARENA_H - WALL_THICK)}; }
    if (dLeft  > best) { best = dLeft;  pos = {toFixed(WALL_THICK),  toFixed(ARENA_H / 2)}; }
    if (dRight > best) {                pos = {toFixed(ARENA_W - WALL_THICK), toFixed(ARENA_H / 2)}; }
    return pos;
}

static Vec2 randomEdgePos() {
    Vec2 pos;
    int edge = rngRange(4);
    switch (edge) {
        case 0: pos = {toFixed(rngRange(ARENA_W - WALL_THICK * 2) + WALL_THICK), toFixed(WALL_THICK)}; break;
        case 1: pos = {toFixed(rngRange(ARENA_W - WALL_THICK * 2) + WALL_THICK), toFixed(ARENA_H - WALL_THICK)}; break;
        case 2: pos = {toFixed(WALL_THICK), toFixed(rngRange(ARENA_H - WALL_THICK * 2) + WALL_THICK)}; break;
        default: pos = {toFixed(ARENA_W - WALL_THICK), toFixed(rngRange(ARENA_H - WALL_THICK * 2) + WALL_THICK)}; break;
    }
    return pos;
}

static Vec2 safeSpawnPos() {
    Vec2 pos;
    bool found = false;
    for (int attempt = 0; attempt < 5; ++attempt) {
        pos = randomEdgePos();
        Vec2 diff = pos - gPlayer.pos;
        s32 dx = static_cast<s32>(diff.x);
        s32 dy = static_cast<s32>(diff.y);
        s32 distSq = dx * dx + dy * dy;
        if (distSq > 2560000) { found = true; break; }
    }
    if (!found) pos = farthestEdgeFromPlayer();
    return pos;
}

// --------------------------------------------------------------------------
// Wave composition system
// --------------------------------------------------------------------------

// Wave entry: one "group" of enemies to spawn in a wave
struct WaveEntry {
    u8  type;       // ETYPE_* constant
    u8  sizeClass;  // SIZE_SMALL / SIZE_MEDIUM / SIZE_LARGE
    u8  count;      // how many to spawn
    u8  formation;  // 0=line, 1=pincer, 2=surround, 3=scatter, 4=fish-school, 5=boss-center
};

// Maximum groups per wave template
constexpr int MAX_WAVE_GROUPS = 4;

struct WaveTemplate {
    WaveEntry groups[MAX_WAVE_GROUPS];
    u8 groupCount;
    bool isBossWave;
};

// HP scaling: base * (100 + (wave-1)*35) / 100
// 35% per wave keeps enemies threatening against exponential player DPS
static s16 scaledHp(int baseHp, int wave) {
    s16 hp = static_cast<s16>(baseHp * (100 + (wave - 1) * 35) / 100);
    return (hp < 1) ? 1 : hp;
}

// Pixel size from size class
static u8 spriteSzFromClass(u8 sc) {
    switch (sc) {
        case SIZE_SMALL:  return SPRITE_SIZE_SMALL;
        case SIZE_MEDIUM: return SPRITE_SIZE_MEDIUM;
        case SIZE_LARGE:  return SPRITE_SIZE_LARGE;
        default:          return SPRITE_SIZE_BOSS;
    }
}

// Base HP for a regular enemy at size sc before wave scaling
static int baseHpForSize(u8 sc, int /*wave*/) {
    switch (sc) {
        case SIZE_SMALL:  return 4;
        case SIZE_MEDIUM: return 8;
        case SIZE_LARGE:  return 14;
        default:          return 480;  // boss base HP
    }
}

// Spawn a group of enemies using the given WaveEntry
static void spawnGroup(const WaveEntry& g, int wave) {
    int n = g.count;
    u8 sprSz = spriteSzFromClass(g.sizeClass);
    s16 hp   = scaledHp(baseHpForSize(g.sizeClass, wave), wave);

    switch (g.formation) {
        case 0: { // line advance — one edge, evenly spaced
            int edge = rngRange(4);
            for (int i = 0; i < n; i++) {
                Vec2 pos = edgePos(edge, i, n);
                Vec2 vel = {0, 0};
                spawnEnemy(pos, vel, hp, g.type, g.sizeClass, sprSz);
            }
            break;
        }
        case 1: { // pincer — two opposite edges
            int edge1 = rngRange(2) * 2;
            int edge2 = edge1 + 1;
            int half = n / 2;
            for (int i = 0; i < half; i++) {
                Vec2 pos = edgePos(edge1, i, half);
                spawnEnemy(pos, {0,0}, hp, g.type, g.sizeClass, sprSz);
            }
            for (int i = 0; i < n - half; i++) {
                Vec2 pos = edgePos(edge2, i, n - half);
                spawnEnemy(pos, {0,0}, hp, g.type, g.sizeClass, sprSz);
            }
            break;
        }
        case 2: { // surround — all 4 edges
            int perEdge = n / 4;
            int rem = n - perEdge * 4;
            for (int edge = 0; edge < 4; edge++) {
                int en = perEdge + (edge < rem ? 1 : 0);
                for (int i = 0; i < en; i++) {
                    Vec2 pos = edgePos(edge, i, en);
                    spawnEnemy(pos, {0,0}, hp, g.type, g.sizeClass, sprSz);
                }
            }
            break;
        }
        case 3: { // scatter — safe random positions
            for (int i = 0; i < n; i++) {
                Vec2 pos = safeSpawnPos();
                spawnEnemy(pos, {0,0}, hp, g.type, g.sizeClass, sprSz);
            }
            break;
        }
        case 4: { // fish school — tight cluster from one edge corner
            int edge = rngRange(4);
            int startI = rngRange(n > 1 ? n / 2 : 1);
            for (int i = 0; i < n; i++) {
                Vec2 base = edgePos(edge, startI, n + 2);
                // Add small jitter within a 20px school
                Fixed jx = toFixed((rngRange(20)) - 10);
                Fixed jy = toFixed((rngRange(20)) - 10);
                Vec2 pos = {static_cast<Fixed>(base.x + jx), static_cast<Fixed>(base.y + jy)};
                spawnEnemy(pos, {0,0}, hp, g.type, g.sizeClass, sprSz);
            }
            break;
        }
        case 5: { // boss-center — spawn at farthest edge, single unit
            Vec2 pos = farthestEdgeFromPlayer();
            // Boss uses large sprite size
            s16 bossHp = scaledHp(baseHpForSize(3, wave), wave);  // size 3 = boss tier
            spawnEnemy(pos, {0,0}, bossHp, g.type, SIZE_LARGE, SPRITE_SIZE_BOSS);
            break;
        }
        default: {
            for (int i = 0; i < n; i++) {
                Vec2 pos = safeSpawnPos();
                spawnEnemy(pos, {0,0}, hp, g.type, g.sizeClass, sprSz);
            }
            break;
        }
    }
}

// --------------------------------------------------------------------------
// Wave table: 30 waves x 5 hand-designed variants
// Formations: 0=line, 1=pincer, 2=surround, 3=scatter, 4=fish-school, 5=boss-center
// After wave 30, procedural endless mode kicks in.
// --------------------------------------------------------------------------
constexpr int WAVE_VARIANTS = 5;
constexpr int TOTAL_WAVES   = 30;

static const WaveTemplate kWaveTable[TOTAL_WAVES][WAVE_VARIANTS] = {
    // ====================================================================
    // WAVE 1 — Tutorial: grunts (8-10 enemies, enough gold for 1 buy)
    // ====================================================================
    {
        { {{ ETYPE_GRUNT, SIZE_SMALL, 6, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 0 }},                        2, false }, // A: 10
        { {{ ETYPE_GRUNT, SIZE_SMALL, 8, 0 }},                                                           1, false }, // B: 8 line
        { {{ ETYPE_GRUNT, SIZE_SMALL, 5, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 2 }},                        2, false }, // C: 8
        { {{ ETYPE_GRUNT, SIZE_SMALL, 4, 0 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 1 }},                        2, false }, // D: 8 pincer
        { {{ ETYPE_GRUNT, SIZE_SMALL, 5, 4 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 3 }},                        2, false }, // E: 9
    },
    // ====================================================================
    // WAVE 2 — Introduce charger (8-10 enemies)
    // ====================================================================
    {
        { {{ ETYPE_CHARGER, SIZE_SMALL, 3, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 3 }},                      2, false }, // A: 8
        { {{ ETYPE_CHARGER, SIZE_SMALL, 4, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 0 }},                      2, false }, // B: 8
        { {{ ETYPE_CHARGER, SIZE_SMALL, 3, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 6, 2 }},                      2, false }, // C: 9
        { {{ ETYPE_GRUNT,   SIZE_SMALL, 8, 0 }, { ETYPE_CHARGER, SIZE_SMALL, 2, 1 }},                    2, false }, // D: 10
        { {{ ETYPE_GRUNT,   SIZE_SMALL, 4, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 4, 1 }},                    2, false }, // E: 8
    },
    // ====================================================================
    // WAVE 3 — Introduce ranged (8-10 enemies)
    // ====================================================================
    {
        { {{ ETYPE_SPITTER, SIZE_SMALL, 3, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 0 }},                      2, false }, // A: 8
        { {{ ETYPE_SPLITTER, SIZE_SMALL, 3, 0 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 3 }},                     2, false }, // B: 8
        { {{ ETYPE_GRUNT, SIZE_SMALL, 6, 0 }, { ETYPE_SPITTER, SIZE_SMALL, 2, 3 }},                      2, false }, // C: 8
        { {{ ETYPE_SPLITTER, SIZE_SMALL, 4, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 3, 1 }},                   2, false }, // D: 7
        { {{ ETYPE_SPITTER, SIZE_SMALL, 3, 1 }, { ETYPE_CHARGER, SIZE_SMALL, 3, 0 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 3 }}, 3, false }, // E: 9
    },
    // ====================================================================
    // WAVE 4 — Introduce shield/sniper (8-10 enemies)
    // ====================================================================
    {
        { {{ ETYPE_SHIELD, SIZE_SMALL, 3, 0 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 3 }},                        2, false }, // A: 8
        { {{ ETYPE_SNIPER, SIZE_SMALL, 2, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 6, 0 }},                        2, false }, // B: 8
        { {{ ETYPE_SHIELD, SIZE_SMALL, 2, 0 }, { ETYPE_SPITTER, SIZE_SMALL, 3, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 1 }}, 3, false }, // C: 8
        { {{ ETYPE_CHARGER, SIZE_SMALL, 4, 1 }, { ETYPE_SNIPER, SIZE_SMALL, 2, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 0 }}, 3, false }, // D: 9
        { {{ ETYPE_SHIELD, SIZE_SMALL, 3, 0 }, { ETYPE_SPLITTER, SIZE_SMALL, 3, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 2 }}, 3, false }, // E: 9
    },
    // ====================================================================
    // WAVE 5 — BOSS: Sentinel
    // ====================================================================
    {
        { {{ ETYPE_BOSS_SENTINEL, SIZE_LARGE, 1, 5 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 3 }},                 2, true  }, // A: 6
        { {{ ETYPE_BOSS_SENTINEL, SIZE_LARGE, 1, 5 }, { ETYPE_SPITTER, SIZE_SMALL, 3, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 0 }}, 3, true }, // B: 7
        { {{ ETYPE_BOSS_SENTINEL, SIZE_LARGE, 1, 5 }, { ETYPE_CHARGER, SIZE_SMALL, 5, 4 }},               2, true  }, // C: 6
        { {{ ETYPE_BOSS_SENTINEL, SIZE_LARGE, 1, 5 }, { ETYPE_SHIELD, SIZE_SMALL, 3, 0 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 3 }}, 3, true }, // D: 7
        { {{ ETYPE_BOSS_SENTINEL, SIZE_LARGE, 1, 5 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 2, 1 }}, 3, true }, // E: 6
    },
    // ====================================================================
    // WAVE 6 — Introduce bomber, medium enemies appear
    // ====================================================================
    {
        { {{ ETYPE_BOMBER, SIZE_SMALL, 4, 3 }, { ETYPE_GRUNT, SIZE_MEDIUM, 4, 0 }},                       2, false }, // A: 8
        { {{ ETYPE_CHARGER, SIZE_MEDIUM, 4, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 3 }},                      2, false }, // B: 9
        { {{ ETYPE_BOMBER, SIZE_SMALL, 5, 3 }, { ETYPE_SPITTER, SIZE_SMALL, 4, 1 }},                      2, false }, // C: 9
        { {{ ETYPE_GRUNT, SIZE_MEDIUM, 4, 0 }, { ETYPE_CHARGER, SIZE_SMALL, 4, 1 }},                      2, false }, // D: 8
        { {{ ETYPE_BOMBER, SIZE_SMALL, 4, 1 }, { ETYPE_SHIELD, SIZE_SMALL, 3, 0 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 3 }}, 3, false }, // E: 10
    },
    // ====================================================================
    // WAVE 7 — Introduce ghost
    // ====================================================================
    {
        { {{ ETYPE_GHOST, SIZE_SMALL, 4, 3 }, { ETYPE_GRUNT, SIZE_MEDIUM, 4, 0 }},                        2, false }, // A: 8
        { {{ ETYPE_GHOST, SIZE_SMALL, 4, 3 }, { ETYPE_SPITTER, SIZE_SMALL, 5, 1 }},                       2, false }, // B: 9
        { {{ ETYPE_GHOST, SIZE_SMALL, 3, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 3, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 0 }}, 3, false }, // C: 10
        { {{ ETYPE_SPITTER, SIZE_MEDIUM, 4, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 2 }},                      2, false }, // D: 9
        { {{ ETYPE_GHOST, SIZE_SMALL, 3, 3 }, { ETYPE_BOMBER, SIZE_SMALL, 3, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 4 }}, 3, false }, // E: 10
    },
    // ====================================================================
    // WAVE 8 — Introduce artillery
    // ====================================================================
    {
        { {{ ETYPE_ARTILLERY, SIZE_MEDIUM, 3, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 6, 0 }},                    2, false }, // A: 9
        { {{ ETYPE_ARTILLERY, SIZE_MEDIUM, 3, 2 }, { ETYPE_SHIELD, SIZE_SMALL, 3, 0 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 3 }}, 3, false }, // B: 10
        { {{ ETYPE_BRUTE, SIZE_LARGE, 1, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 7, 4 }},                         2, false }, // C: 8
        { {{ ETYPE_ARTILLERY, SIZE_MEDIUM, 3, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 5, 1 }},                  2, false }, // D: 8
        { {{ ETYPE_ARTILLERY, SIZE_SMALL, 4, 2 }, { ETYPE_SPITTER, SIZE_MEDIUM, 4, 3 }},                  2, false }, // E: 8
    },
    // ====================================================================
    // WAVE 9 — Introduce swarm drone
    // ====================================================================
    {
        { {{ ETYPE_SWARM_DRONE, SIZE_SMALL, 6, 4 }, { ETYPE_SNIPER, SIZE_MEDIUM, 2, 3 }},                 2, false }, // A
        { {{ ETYPE_SWARM_DRONE, SIZE_SMALL, 5, 4 }, { ETYPE_SPITTER, SIZE_MEDIUM, 2, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 2, 3 }}, 3, false }, // B
        { {{ ETYPE_SNIPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_SPITTER, SIZE_MEDIUM, 3, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 0 }}, 3, false }, // C
        { {{ ETYPE_SWARM_DRONE, SIZE_SMALL, 8, 4 }, { ETYPE_BRUTE, SIZE_MEDIUM, 1, 3 }},                  2, false }, // D
        { {{ ETYPE_SWARM_DRONE, SIZE_SMALL, 4, 4 }, { ETYPE_GHOST, SIZE_SMALL, 2, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 3, 1 }}, 3, false }, // E
    },
    // ====================================================================
    // WAVE 10 — BOSS: Dreadnought
    // ====================================================================
    {
        { {{ ETYPE_BOSS_DREADNOUGHT, SIZE_LARGE, 1, 5 }, { ETYPE_SPITTER, SIZE_SMALL, 6, 2 }},            2, true  }, // A: 7
        { {{ ETYPE_BOSS_DREADNOUGHT, SIZE_LARGE, 1, 5 }, { ETYPE_CHARGER, SIZE_SMALL, 4, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 3 }}, 3, true }, // B: 9
        { {{ ETYPE_BOSS_DREADNOUGHT, SIZE_LARGE, 1, 5 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 7, 4 }},        2, true  }, // C: 8
        { {{ ETYPE_BOSS_DREADNOUGHT, SIZE_LARGE, 1, 5 }, { ETYPE_SHIELD, SIZE_SMALL, 3, 0 }, { ETYPE_SPITTER, SIZE_SMALL, 4, 3 }}, 3, true }, // D: 8
        { {{ ETYPE_BOSS_DREADNOUGHT, SIZE_LARGE, 1, 5 }, { ETYPE_BOMBER, SIZE_SMALL, 4, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 0 }}, 3, true }, // E: 8
    },
    // ====================================================================
    // WAVE 11 — Introduce medic; splitter-heavy
    // ====================================================================
    {
        { {{ ETYPE_SPLITTER, SIZE_MEDIUM, 5, 2 }, { ETYPE_MEDIC, SIZE_SMALL, 2, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 0 }}, 3, false }, // A: 11
        { {{ ETYPE_SPLITTER, SIZE_MEDIUM, 4, 3 }, { ETYPE_SPLITTER, SIZE_LARGE, 1, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 5, 1 }}, 3, false }, // B: 10
        { {{ ETYPE_MEDIC, SIZE_SMALL, 2, 3 }, { ETYPE_BRUTE, SIZE_MEDIUM, 3, 0 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 4 }}, 3, false }, // C: 10
        { {{ ETYPE_SPLITTER, SIZE_MEDIUM, 6, 2 }, { ETYPE_MEDIC, SIZE_SMALL, 2, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 0 }}, 3, false }, // D: 12
        { {{ ETYPE_SPLITTER, SIZE_LARGE, 2, 3 }, { ETYPE_SPITTER, SIZE_MEDIUM, 3, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 3 }}, 3, false }, // E: 10
    },
    // ====================================================================
    // WAVE 12 — Introduce anchor; shield combos
    // ====================================================================
    {
        { {{ ETYPE_SHIELD, SIZE_MEDIUM, 4, 1 }, { ETYPE_ANCHOR, SIZE_SMALL, 2, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 5, 3 }}, 3, false }, // A: 11
        { {{ ETYPE_ANCHOR, SIZE_MEDIUM, 3, 3 }, { ETYPE_GRUNT, SIZE_MEDIUM, 4, 0 }, { ETYPE_SPITTER, SIZE_SMALL, 3, 1 }}, 3, false }, // B: 10
        { {{ ETYPE_SHIELD, SIZE_MEDIUM, 5, 1 }, { ETYPE_SNIPER, SIZE_SMALL, 5, 3 }},                      2, false }, // C: 10
        { {{ ETYPE_ANCHOR, SIZE_SMALL, 3, 3 }, { ETYPE_BRUTE, SIZE_MEDIUM, 3, 0 }, { ETYPE_CHARGER, SIZE_SMALL, 5, 1 }}, 3, false }, // D: 11
        { {{ ETYPE_SHIELD, SIZE_MEDIUM, 3, 0 }, { ETYPE_ANCHOR, SIZE_SMALL, 2, 3 }, { ETYPE_GHOST, SIZE_SMALL, 3, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 4 }}, 4, false }, // E: 12
    },
    // ====================================================================
    // WAVE 13 — Introduce trapper; ghost + artillery combos
    // ====================================================================
    {
        { {{ ETYPE_TRAPPER, SIZE_SMALL, 3, 3 }, { ETYPE_GHOST, SIZE_SMALL, 4, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 4 }}, 3, false }, // A: 12
        { {{ ETYPE_ARTILLERY, SIZE_MEDIUM, 3, 2 }, { ETYPE_TRAPPER, SIZE_SMALL, 3, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 5, 1 }}, 3, false }, // B: 11
        { {{ ETYPE_GHOST, SIZE_SMALL, 5, 3 }, { ETYPE_ARTILLERY, SIZE_MEDIUM, 2, 2 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 0 }}, 3, false }, // C: 11
        { {{ ETYPE_TRAPPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_SHIELD, SIZE_SMALL, 4, 0 }}, 3, false }, // D: 10
        { {{ ETYPE_TRAPPER, SIZE_SMALL, 4, 3 }, { ETYPE_BOMBER, SIZE_SMALL, 3, 1 }, { ETYPE_SPITTER, SIZE_MEDIUM, 3, 3 }}, 3, false }, // E: 10
    },
    // ====================================================================
    // WAVE 14 — Introduce hexer + hive; everything mixed
    // ====================================================================
    {
        { {{ ETYPE_HEXER, SIZE_MEDIUM, 2, 3 }, { ETYPE_BRUTE, SIZE_LARGE, 1, 3 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 8, 4 }}, 3, false }, // A: 11
        { {{ ETYPE_HIVE, SIZE_MEDIUM, 1, 3 }, { ETYPE_NIGHTMARE, SIZE_MEDIUM, 2, 3 }, { ETYPE_GRUNT, SIZE_MEDIUM, 4, 0 }, { ETYPE_CHARGER, SIZE_SMALL, 4, 1 }}, 4, false }, // B: 11
        { {{ ETYPE_HEXER, SIZE_SMALL, 4, 3 }, { ETYPE_SHIELD, SIZE_MEDIUM, 3, 0 }, { ETYPE_SPITTER, SIZE_MEDIUM, 3, 1 }}, 3, false }, // C: 10
        { {{ ETYPE_HIVE, SIZE_MEDIUM, 2, 3 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 6, 4 }, { ETYPE_SNIPER, SIZE_MEDIUM, 2, 3 }}, 3, false }, // D: 10
        { {{ ETYPE_HEXER, SIZE_MEDIUM, 2, 3 }, { ETYPE_HIVE, SIZE_MEDIUM, 1, 3 }, { ETYPE_BRUTE, SIZE_MEDIUM, 3, 0 }, { ETYPE_GHOST, SIZE_SMALL, 4, 3 }}, 4, false }, // E: 10
    },
    // ====================================================================
    // WAVE 15 — BOSS: Leviathan
    // ====================================================================
    {
        { {{ ETYPE_BOSS_LEVIATHAN, SIZE_LARGE, 1, 5 }, { ETYPE_SPLITTER, SIZE_MEDIUM, 3, 3 }, { ETYPE_SPITTER, SIZE_SMALL, 5, 3 }}, 3, true }, // A: 9
        { {{ ETYPE_BOSS_LEVIATHAN, SIZE_LARGE, 1, 5 }, { ETYPE_CHARGER, SIZE_SMALL, 5, 1 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 3 }}, 3, true }, // B: 10
        { {{ ETYPE_BOSS_LEVIATHAN, SIZE_LARGE, 1, 5 }, { ETYPE_MEDIC, SIZE_SMALL, 3, 3 }, { ETYPE_SHIELD, SIZE_MEDIUM, 4, 0 }}, 3, true }, // C: 8
        { {{ ETYPE_BOSS_LEVIATHAN, SIZE_LARGE, 1, 5 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 7, 4 }, { ETYPE_SNIPER, SIZE_SMALL, 2, 3 }}, 3, true }, // D: 10
        { {{ ETYPE_BOSS_LEVIATHAN, SIZE_LARGE, 1, 5 }, { ETYPE_BOMBER, SIZE_SMALL, 4, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 4, 1 }}, 3, true }, // E: 9
    },
    // ====================================================================
    // WAVE 16 — Late game: shield + sniper combos
    // ====================================================================
    {
        { {{ ETYPE_SHIELD, SIZE_MEDIUM, 4, 0 }, { ETYPE_SNIPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 5, 1 }}, 3, false }, // A: 12
        { {{ ETYPE_SHIELD, SIZE_MEDIUM, 3, 0 }, { ETYPE_SNIPER, SIZE_MEDIUM, 4, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 3, 1 }}, 3, false }, // B: 10
        { {{ ETYPE_SHIELD, SIZE_LARGE, 1, 0 }, { ETYPE_SNIPER, SIZE_SMALL, 5, 3 }, { ETYPE_SPITTER, SIZE_MEDIUM, 4, 1 }}, 3, false }, // C: 10
        { {{ ETYPE_ANCHOR, SIZE_MEDIUM, 3, 3 }, { ETYPE_SHIELD, SIZE_MEDIUM, 3, 0 }, { ETYPE_SNIPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 3, 3 }}, 4, false }, // D: 12
        { {{ ETYPE_SHIELD, SIZE_MEDIUM, 4, 1 }, { ETYPE_ARTILLERY, SIZE_MEDIUM, 3, 3 }, { ETYPE_SNIPER, SIZE_SMALL, 4, 3 }}, 3, false }, // E: 11
    },
    // ====================================================================
    // WAVE 17 — Nightmare + charger pressure
    // ====================================================================
    {
        { {{ ETYPE_NIGHTMARE, SIZE_MEDIUM, 3, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 7, 1 }},                  2, false }, // A: 10
        { {{ ETYPE_NIGHTMARE, SIZE_MEDIUM, 2, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 5, 1 }, { ETYPE_BOMBER, SIZE_SMALL, 5, 3 }}, 3, false }, // B: 12
        { {{ ETYPE_NIGHTMARE, SIZE_LARGE, 1, 3 }, { ETYPE_GHOST, SIZE_SMALL, 5, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 6, 4 }}, 3, false }, // C: 12
        { {{ ETYPE_NIGHTMARE, SIZE_MEDIUM, 3, 3 }, { ETYPE_HEXER, SIZE_SMALL, 3, 3 }, { ETYPE_GRUNT, SIZE_MEDIUM, 5, 0 }}, 3, false }, // D: 11
        { {{ ETYPE_NIGHTMARE, SIZE_MEDIUM, 3, 1 }, { ETYPE_CHARGER, SIZE_SMALL, 7, 4 }, { ETYPE_MEDIC, SIZE_SMALL, 2, 3 }}, 3, false }, // E: 12
    },
    // ====================================================================
    // WAVE 18 — Artillery + shield fortress
    // ====================================================================
    {
        { {{ ETYPE_ARTILLERY, SIZE_MEDIUM, 4, 2 }, { ETYPE_SHIELD, SIZE_MEDIUM, 4, 0 }, { ETYPE_GRUNT, SIZE_MEDIUM, 4, 3 }}, 3, false }, // A: 12
        { {{ ETYPE_ARTILLERY, SIZE_LARGE, 1, 3 }, { ETYPE_SHIELD, SIZE_MEDIUM, 4, 0 }, { ETYPE_TRAPPER, SIZE_SMALL, 5, 3 }}, 3, false }, // B: 10
        { {{ ETYPE_ARTILLERY, SIZE_MEDIUM, 4, 2 }, { ETYPE_ANCHOR, SIZE_MEDIUM, 2, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 5, 1 }}, 3, false }, // C: 11
        { {{ ETYPE_ARTILLERY, SIZE_MEDIUM, 3, 3 }, { ETYPE_SHIELD, SIZE_MEDIUM, 3, 0 }, { ETYPE_SNIPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 1 }}, 4, false }, // D: 13
        { {{ ETYPE_ARTILLERY, SIZE_MEDIUM, 3, 2 }, { ETYPE_HIVE, SIZE_MEDIUM, 2, 3 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 6, 4 }}, 3, false }, // E: 11
    },
    // ====================================================================
    // WAVE 19 — Hive + drone swarm
    // ====================================================================
    {
        { {{ ETYPE_HIVE, SIZE_MEDIUM, 2, 3 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 8, 4 }, { ETYPE_SPITTER, SIZE_MEDIUM, 3, 1 }}, 3, false }, // A: 13
        { {{ ETYPE_HIVE, SIZE_LARGE, 1, 3 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 10, 4 }},                   2, false }, // B: 11
        { {{ ETYPE_HIVE, SIZE_MEDIUM, 2, 3 }, { ETYPE_BOMBER, SIZE_SMALL, 5, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 4, 1 }}, 3, false }, // C: 11
        { {{ ETYPE_HIVE, SIZE_MEDIUM, 2, 3 }, { ETYPE_TRAPPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_GHOST, SIZE_SMALL, 4, 3 }, { ETYPE_GRUNT, SIZE_SMALL, 4, 4 }}, 4, false }, // D: 13
        { {{ ETYPE_HIVE, SIZE_MEDIUM, 2, 3 }, { ETYPE_HEXER, SIZE_MEDIUM, 2, 3 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 7, 4 }}, 3, false }, // E: 11
    },
    // ====================================================================
    // WAVE 20 — BOSS: Nightmare King
    // ====================================================================
    {
        { {{ ETYPE_BOSS_NIGHTMARE_B, SIZE_LARGE, 1, 5 }, { ETYPE_NIGHTMARE, SIZE_MEDIUM, 3, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 5, 1 }}, 3, true }, // A: 9
        { {{ ETYPE_BOSS_NIGHTMARE_B, SIZE_LARGE, 1, 5 }, { ETYPE_GHOST, SIZE_SMALL, 5, 3 }, { ETYPE_BOMBER, SIZE_SMALL, 4, 3 }}, 3, true }, // B: 10
        { {{ ETYPE_BOSS_NIGHTMARE_B, SIZE_LARGE, 1, 5 }, { ETYPE_HEXER, SIZE_MEDIUM, 3, 3 }, { ETYPE_SHIELD, SIZE_MEDIUM, 3, 0 }}, 3, true }, // C: 7 (heavy mediums)
        { {{ ETYPE_BOSS_NIGHTMARE_B, SIZE_LARGE, 1, 5 }, { ETYPE_MEDIC, SIZE_SMALL, 2, 3 }, { ETYPE_BRUTE, SIZE_MEDIUM, 3, 0 }, { ETYPE_GRUNT, SIZE_MEDIUM, 3, 3 }}, 4, true }, // D: 9
        { {{ ETYPE_BOSS_NIGHTMARE_B, SIZE_LARGE, 1, 5 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 8, 4 }, { ETYPE_SNIPER, SIZE_MEDIUM, 2, 3 }}, 3, true }, // E: 11
    },
    // ====================================================================
    // WAVE 21 — Endgame: medic + brute combos
    // ====================================================================
    {
        { {{ ETYPE_MEDIC, SIZE_MEDIUM, 3, 3 }, { ETYPE_BRUTE, SIZE_LARGE, 2, 0 }, { ETYPE_GRUNT, SIZE_MEDIUM, 5, 1 }}, 3, false }, // A: 10 (heavy, large enemies)
        { {{ ETYPE_MEDIC, SIZE_MEDIUM, 3, 3 }, { ETYPE_BRUTE, SIZE_MEDIUM, 5, 0 }, { ETYPE_SHIELD, SIZE_MEDIUM, 4, 1 }}, 3, false }, // B: 12
        { {{ ETYPE_MEDIC, SIZE_SMALL, 4, 3 }, { ETYPE_ANCHOR, SIZE_MEDIUM, 3, 3 }, { ETYPE_BRUTE, SIZE_LARGE, 1, 0 }, { ETYPE_CHARGER, SIZE_MEDIUM, 4, 1 }}, 4, false }, // C: 12
        { {{ ETYPE_BRUTE, SIZE_LARGE, 2, 0 }, { ETYPE_SPITTER, SIZE_MEDIUM, 5, 1 }, { ETYPE_MEDIC, SIZE_SMALL, 3, 3 }}, 3, false }, // D: 10 (heavy, large enemies)
        { {{ ETYPE_MEDIC, SIZE_MEDIUM, 3, 3 }, { ETYPE_NIGHTMARE, SIZE_MEDIUM, 3, 3 }, { ETYPE_BRUTE, SIZE_MEDIUM, 5, 0 }}, 3, false }, // E: 11 (all medium)
    },
    // ====================================================================
    // WAVE 22 — Trapper + charger kill zone
    // ====================================================================
    {
        { {{ ETYPE_TRAPPER, SIZE_MEDIUM, 4, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 5, 1 }, { ETYPE_GRUNT, SIZE_MEDIUM, 4, 0 }}, 3, false }, // A: 13
        { {{ ETYPE_TRAPPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_BOMBER, SIZE_MEDIUM, 4, 1 }, { ETYPE_GHOST, SIZE_SMALL, 5, 3 }}, 3, false }, // B: 12
        { {{ ETYPE_TRAPPER, SIZE_LARGE, 1, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 6, 4 }, { ETYPE_SHIELD, SIZE_MEDIUM, 4, 0 }}, 3, false }, // C: 11 (large+mediums)
        { {{ ETYPE_TRAPPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 5, 1 }, { ETYPE_MEDIC, SIZE_SMALL, 2, 3 }}, 4, false }, // D: 13
        { {{ ETYPE_TRAPPER, SIZE_MEDIUM, 4, 2 }, { ETYPE_HEXER, SIZE_MEDIUM, 3, 3 }, { ETYPE_CHARGER, SIZE_SMALL, 6, 4 }}, 3, false }, // E: 13
    },
    // ====================================================================
    // WAVE 23 — Hexer + sniper precision hell
    // ====================================================================
    {
        { {{ ETYPE_HEXER, SIZE_MEDIUM, 4, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 4, 3 }, { ETYPE_SHIELD, SIZE_MEDIUM, 4, 0 }}, 3, false }, // A: 12
        { {{ ETYPE_HEXER, SIZE_LARGE, 1, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 4, 3 }, { ETYPE_ARTILLERY, SIZE_MEDIUM, 3, 2 }, { ETYPE_GRUNT, SIZE_MEDIUM, 4, 0 }}, 4, false }, // B: 12
        { {{ ETYPE_HEXER, SIZE_MEDIUM, 4, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 4, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 5, 1 }}, 3, false }, // C: 13
        { {{ ETYPE_HEXER, SIZE_MEDIUM, 3, 3 }, { ETYPE_GHOST, SIZE_MEDIUM, 4, 3 }, { ETYPE_SNIPER, SIZE_SMALL, 5, 3 }}, 3, false }, // D: 12
        { {{ ETYPE_HEXER, SIZE_MEDIUM, 3, 1 }, { ETYPE_ANCHOR, SIZE_MEDIUM, 3, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_BRUTE, SIZE_MEDIUM, 3, 0 }}, 4, false }, // E: 12
    },
    // ====================================================================
    // WAVE 24 — All-type gauntlet
    // ====================================================================
    {
        { {{ ETYPE_HIVE, SIZE_MEDIUM, 2, 3 }, { ETYPE_ANCHOR, SIZE_MEDIUM, 3, 3 }, { ETYPE_BRUTE, SIZE_LARGE, 1, 0 }, { ETYPE_CHARGER, SIZE_MEDIUM, 6, 4 }}, 4, false }, // A: 12
        { {{ ETYPE_NIGHTMARE, SIZE_MEDIUM, 3, 3 }, { ETYPE_SPLITTER, SIZE_LARGE, 1, 3 }, { ETYPE_MEDIC, SIZE_MEDIUM, 2, 3 }, { ETYPE_BOMBER, SIZE_MEDIUM, 5, 1 }}, 4, false }, // B: 11 (large+mediums)
        { {{ ETYPE_TRAPPER, SIZE_MEDIUM, 4, 3 }, { ETYPE_ARTILLERY, SIZE_MEDIUM, 4, 2 }, { ETYPE_SHIELD, SIZE_MEDIUM, 5, 0 }}, 3, false }, // C: 13
        { {{ ETYPE_GHOST, SIZE_MEDIUM, 4, 3 }, { ETYPE_HEXER, SIZE_MEDIUM, 3, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 3, 3 }, { ETYPE_GRUNT, SIZE_MEDIUM, 3, 0 }}, 4, false }, // D: 13
        { {{ ETYPE_SPLITTER, SIZE_LARGE, 2, 3 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 8, 4 }, { ETYPE_MEDIC, SIZE_MEDIUM, 2, 3 }}, 3, false }, // E: 12
    },
    // ====================================================================
    // WAVE 25 — BOSS: Dual-threat boss wave
    // ====================================================================
    {
        { {{ ETYPE_BOSS_SENTINEL, SIZE_LARGE, 1, 5 }, { ETYPE_BOSS_DREADNOUGHT, SIZE_LARGE, 1, 5 }, { ETYPE_CHARGER, SIZE_MEDIUM, 5, 1 }}, 3, true }, // A: 7 (+bosses)
        { {{ ETYPE_BOSS_LEVIATHAN, SIZE_LARGE, 1, 5 }, { ETYPE_MEDIC, SIZE_MEDIUM, 3, 3 }, { ETYPE_SHIELD, SIZE_MEDIUM, 4, 0 }}, 3, true }, // B: 8
        { {{ ETYPE_BOSS_SENTINEL, SIZE_LARGE, 1, 5 }, { ETYPE_BOSS_NIGHTMARE_B, SIZE_LARGE, 1, 5 }, { ETYPE_GRUNT, SIZE_MEDIUM, 6, 3 }}, 3, true }, // C: 8 (+bosses)
        { {{ ETYPE_BOSS_DREADNOUGHT, SIZE_LARGE, 1, 5 }, { ETYPE_HEXER, SIZE_MEDIUM, 3, 3 }, { ETYPE_BRUTE, SIZE_LARGE, 1, 0 }, { ETYPE_SNIPER, SIZE_MEDIUM, 3, 3 }}, 4, true }, // D: 8
        { {{ ETYPE_BOSS_LEVIATHAN, SIZE_LARGE, 1, 5 }, { ETYPE_ANCHOR, SIZE_MEDIUM, 3, 3 }, { ETYPE_ARTILLERY, SIZE_MEDIUM, 4, 2 }}, 3, true }, // E: 8
    },
    // ====================================================================
    // WAVE 26 — Final gauntlet: maximum danger combos
    // ====================================================================
    {
        { {{ ETYPE_SHIELD, SIZE_LARGE, 2, 0 }, { ETYPE_SNIPER, SIZE_MEDIUM, 6, 3 }, { ETYPE_MEDIC, SIZE_MEDIUM, 6, 3 }}, 3, false }, // A: 14 (2 large + mediums)
        { {{ ETYPE_NIGHTMARE, SIZE_LARGE, 1, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 8, 1 }, { ETYPE_TRAPPER, SIZE_MEDIUM, 5, 3 }}, 3, false }, // B: 14
        { {{ ETYPE_HIVE, SIZE_LARGE, 1, 3 }, { ETYPE_HEXER, SIZE_MEDIUM, 3, 3 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 8, 4 }, { ETYPE_SHIELD, SIZE_MEDIUM, 3, 0 }}, 4, false }, // C: 15
        { {{ ETYPE_ARTILLERY, SIZE_LARGE, 1, 3 }, { ETYPE_ANCHOR, SIZE_MEDIUM, 4, 3 }, { ETYPE_BRUTE, SIZE_LARGE, 1, 0 }, { ETYPE_BOMBER, SIZE_MEDIUM, 6, 1 }}, 4, false }, // D: 12 (2 large + mediums)
        { {{ ETYPE_NIGHTMARE, SIZE_MEDIUM, 5, 3 }, { ETYPE_GHOST, SIZE_MEDIUM, 5, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 6, 3 }}, 3, false }, // E: 16
    },
    // ====================================================================
    // WAVE 27 — Fortress breaker: tanky + support
    // ====================================================================
    {
        { {{ ETYPE_BRUTE, SIZE_LARGE, 2, 0 }, { ETYPE_MEDIC, SIZE_MEDIUM, 4, 3 }, { ETYPE_ANCHOR, SIZE_MEDIUM, 4, 3 }}, 3, false }, // A: 10 (2 large + mediums)
        { {{ ETYPE_SHIELD, SIZE_LARGE, 2, 0 }, { ETYPE_ARTILLERY, SIZE_MEDIUM, 5, 2 }, { ETYPE_MEDIC, SIZE_SMALL, 4, 3 }}, 3, false }, // B: 11 (2 large + mix)
        { {{ ETYPE_BRUTE, SIZE_LARGE, 1, 0 }, { ETYPE_SHIELD, SIZE_LARGE, 1, 0 }, { ETYPE_HEXER, SIZE_MEDIUM, 4, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 6, 1 }}, 4, false }, // C: 12 (2 large + mediums)
        { {{ ETYPE_ANCHOR, SIZE_LARGE, 1, 3 }, { ETYPE_BRUTE, SIZE_MEDIUM, 5, 0 }, { ETYPE_TRAPPER, SIZE_MEDIUM, 5, 3 }}, 3, false }, // D: 11 (large + mediums)
        { {{ ETYPE_BRUTE, SIZE_LARGE, 2, 0 }, { ETYPE_HIVE, SIZE_MEDIUM, 2, 3 }, { ETYPE_SWARM_DRONE, SIZE_SMALL, 8, 4 }, { ETYPE_MEDIC, SIZE_SMALL, 3, 3 }}, 4, false }, // E: 15
    },
    // ====================================================================
    // WAVE 28 — Chaos swarm: fast + teleport + fear
    // ====================================================================
    {
        { {{ ETYPE_GHOST, SIZE_MEDIUM, 5, 3 }, { ETYPE_NIGHTMARE, SIZE_MEDIUM, 4, 3 }, { ETYPE_BOMBER, SIZE_MEDIUM, 5, 1 }}, 3, false }, // A: 14
        { {{ ETYPE_GHOST, SIZE_LARGE, 1, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 7, 4 }, { ETYPE_HEXER, SIZE_MEDIUM, 4, 3 }}, 3, false }, // B: 12 (large + mediums)
        { {{ ETYPE_NIGHTMARE, SIZE_LARGE, 1, 3 }, { ETYPE_GHOST, SIZE_MEDIUM, 5, 3 }, { ETYPE_BOMBER, SIZE_MEDIUM, 4, 1 }, { ETYPE_CHARGER, SIZE_MEDIUM, 4, 4 }}, 4, false }, // C: 14
        { {{ ETYPE_GHOST, SIZE_MEDIUM, 5, 3 }, { ETYPE_SPLITTER, SIZE_LARGE, 2, 3 }, { ETYPE_MEDIC, SIZE_MEDIUM, 3, 3 }}, 3, false }, // D: 10 (2 large + mediums)
        { {{ ETYPE_NIGHTMARE, SIZE_MEDIUM, 5, 2 }, { ETYPE_TRAPPER, SIZE_MEDIUM, 4, 3 }, { ETYPE_GHOST, SIZE_MEDIUM, 5, 3 }}, 3, false }, // E: 14
    },
    // ====================================================================
    // WAVE 29 — The gauntlet: everything at once
    // ====================================================================
    {
        { {{ ETYPE_BRUTE, SIZE_LARGE, 2, 0 }, { ETYPE_HEXER, SIZE_MEDIUM, 4, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 5, 3 }, { ETYPE_MEDIC, SIZE_MEDIUM, 3, 3 }}, 4, false }, // A: 14
        { {{ ETYPE_HIVE, SIZE_LARGE, 1, 3 }, { ETYPE_ANCHOR, SIZE_MEDIUM, 4, 3 }, { ETYPE_NIGHTMARE, SIZE_MEDIUM, 4, 3 }, { ETYPE_CHARGER, SIZE_MEDIUM, 5, 1 }}, 4, false }, // B: 14
        { {{ ETYPE_SHIELD, SIZE_LARGE, 2, 0 }, { ETYPE_ARTILLERY, SIZE_LARGE, 1, 3 }, { ETYPE_TRAPPER, SIZE_MEDIUM, 4, 3 }, { ETYPE_BOMBER, SIZE_MEDIUM, 5, 1 }}, 4, false }, // C: 12 (3 large + mediums)
        { {{ ETYPE_SPLITTER, SIZE_LARGE, 2, 3 }, { ETYPE_GHOST, SIZE_MEDIUM, 6, 3 }, { ETYPE_HEXER, SIZE_MEDIUM, 5, 3 }}, 3, false }, // D: 13 (2 large + mediums)
        { {{ ETYPE_NIGHTMARE, SIZE_LARGE, 1, 3 }, { ETYPE_BRUTE, SIZE_LARGE, 1, 0 }, { ETYPE_MEDIC, SIZE_MEDIUM, 4, 3 }, { ETYPE_SNIPER, SIZE_MEDIUM, 4, 3 }}, 4, false }, // E: 10 (2 large + mediums)
    },
    // ====================================================================
    // WAVE 30 — Apothecary (handled separately; placeholder entries)
    // ====================================================================
    {
        { {{}}, 0, true },
        { {{}}, 0, true },
        { {{}}, 0, true },
        { {{}}, 0, true },
        { {{}}, 0, true },
    },
};

// Spawn wave 30 final boss (Apothecary) — called from spawnWave()
static void spawnApothecaryBoss(int wave) {
    Vec2 pos = farthestEdgeFromPlayer();
    s16 bossHp = scaledHp(1800, wave);  // final boss
    spawnEnemy(pos, {0,0}, bossHp, ETYPE_BOSS_APOTHECARY, SIZE_LARGE, SPRITE_SIZE_BOSS_LARGE);
}

// Endless (31+): pool of enemy types weighted by wave depth, randomised counts
static void spawnEndlessWave(int wave) {
    // How many total enemy-slots to fill, capped at MAX_ENEMIES - 1
    int budget = 8 + (wave - 15) * 2;
    if (budget > 28) budget = 28;

    // Increase variety and difficulty every 5 waves past wave 30
    int tier = (wave - 31) / 5;  // 0,1,2,3...
    if (tier > 3) tier = 3;

    // Available type pool grows with tier
    static const u8 typePools[4][8] = {
        { ETYPE_GRUNT, ETYPE_CHARGER, ETYPE_SPITTER, ETYPE_BRUTE,
          ETYPE_GRUNT, ETYPE_CHARGER, ETYPE_GRUNT,   ETYPE_CHARGER },         // tier 0
        { ETYPE_CHARGER, ETYPE_SPITTER, ETYPE_SNIPER, ETYPE_SPLITTER,
          ETYPE_BRUTE,   ETYPE_SHIELD,  ETYPE_GRUNT,  ETYPE_CHARGER },        // tier 1
        { ETYPE_SPLITTER, ETYPE_SHIELD, ETYPE_ARTILLERY, ETYPE_NIGHTMARE,
          ETYPE_SPITTER,  ETYPE_SNIPER, ETYPE_GHOST,     ETYPE_BOMBER },      // tier 2
        { ETYPE_NIGHTMARE, ETYPE_GHOST, ETYPE_BOMBER, ETYPE_SWARM_DRONE,
          ETYPE_SPLITTER,  ETYPE_SHIELD,ETYPE_ARTILLERY, ETYPE_SPITTER },     // tier 3
    };
    static const u8 sizePools[4][2] = {
        { SIZE_SMALL, SIZE_SMALL  },
        { SIZE_SMALL, SIZE_MEDIUM },
        { SIZE_MEDIUM, SIZE_LARGE },
        { SIZE_MEDIUM, SIZE_LARGE },
    };

    while (budget > 0) {
        int poolIdx = rngRange(8);
        u8 etype = typePools[tier][poolIdx];
        u8 sc    = sizePools[tier][rngRange(2)];
        int cnt  = (sc == SIZE_LARGE) ? 1 : (sc == SIZE_MEDIUM ? rngRange(2) + 1 : rngRange(3) + 1);
        if (cnt > budget) cnt = budget;
        int form = rngRange(5);
        WaveEntry g = { etype, sc, static_cast<u8>(cnt), static_cast<u8>(form) };
        spawnGroup(g, wave);
        budget -= cnt;
    }

    // Every 5 waves past 30: add a roaming boss
    if ((wave - 30) % 5 == 0 && wave > 30) {
        static const u8 bossTypes[4] = {
            ETYPE_BOSS_SENTINEL, ETYPE_BOSS_DREADNOUGHT,
            ETYPE_BOSS_LEVIATHAN, ETYPE_BOSS_NIGHTMARE_B
        };
        u8 bossType = bossTypes[((wave - 30) / 5 - 1) % 4];
        Vec2 pos = farthestEdgeFromPlayer();
        s16 bossHp = scaledHp(baseHpForSize(3, wave), wave);
        spawnEnemy(pos, {0,0}, bossHp, bossType, SIZE_LARGE, SPRITE_SIZE_BOSS);
    }
}

static void spawnWave() {
    waveNumber++;

    // Set wave timer
    gWaveTimerMax = waveTimerFrames(waveNumber);
    gWaveTimer    = gWaveTimerMax;
    waveAnnounceTimer = 90;
    overflowSpawned   = false;

    if (waveNumber <= TOTAL_WAVES) {
        if (waveNumber == 30) {
            // Wave 30: final boss (Apothecary) — special spawn
            spawnApothecaryBoss(waveNumber);
        } else {
            // Waves 1-29: pick a random variant from the hand-designed table
            int variant = rngRange(WAVE_VARIANTS);
            const WaveTemplate& tmpl = kWaveTable[waveNumber - 1][variant];
            for (int g = 0; g < tmpl.groupCount; g++) {
                spawnGroup(tmpl.groups[g], waveNumber);
            }
        }
    } else {
        // Endless (31+): procedural scaling
        spawnEndlessWave(waveNumber);
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

    // Wave announce freeze: skip combat updates during announcement
    if (waveAnnounceTimer > 0) {
        waveAnnounceTimer--;
        cameraUpdate(gPlayer.pos);
        waveAnnouncementTick();
        return;
    }

    playerUpdate(keysHeld(), keysDown());
    companionUpdate();
    companionApplyPassives();
    combatUpdate();

    cameraUpdate(gPlayer.pos);

    // Tick wave timer
    if (waveActive && gWaveTimer > 0) {
        gWaveTimer--;
        if (gWaveTimer == 0 && !overflowSpawned) {
            // Overflow: half-count of fast Chargers to pressure the player
            int overflowCount = 3 + waveNumber / 3;
            if (overflowCount > 12) overflowCount = 12;
            for (int i = 0; i < overflowCount; ++i) {
                Vec2 pos = safeSpawnPos();
                s16 hp = scaledHp(baseHpForSize(SIZE_SMALL, waveNumber), waveNumber);
                spawnEnemy(pos, {0,0}, hp, ETYPE_CHARGER, SIZE_SMALL, SPRITE_SIZE_SMALL);
            }
            overflowSpawned = true;
        }
    }

    // Wave-clear detection
    if (waveActive && allEnemiesDead()) {
        waveActive = false;
        waveClearTimer = 60;
        waveAnnouncementClear();
        // Boss waves get the extra "BOSS DEFEATED!" banner timer
        bool bossWave = (waveNumber == 5 || waveNumber == 10 ||
                         waveNumber == 15 || waveNumber == 20 ||
                         waveNumber == 25 || waveNumber == 30 ||
                         (waveNumber > 30 && (waveNumber - 30) % 5 == 0));
        if (bossWave) bossDefeatedTimer = 90;
    }

    // Tick announcement timer every frame
    waveAnnouncementTick();

    // Show "WAVE CLEAR!" banner, then transition to PerkChoice (boss) or Shop
    if (!waveActive && waveClearTimer > 0) {
        waveClearTimer--;
        if (waveClearTimer == 0) {
            bool bossWave = (waveNumber == 5 || waveNumber == 10 ||
                         waveNumber == 15 || waveNumber == 20 ||
                         waveNumber == 25 || waveNumber == 30 ||
                         (waveNumber > 30 && (waveNumber - 30) % 5 == 0));
            if (bossWave) {
                // bossDefeatedTimer was already set above; stay in Play state
                // and let the banner tick finish before entering PerkChoice
            } else {
                enterShop();
                return;
            }
        }
    }

    // "BOSS DEFEATED!" banner countdown → Victory (wave 30) or PerkChoice
    if (!waveActive && waveClearTimer == 0 && bossDefeatedTimer > 0) {
        bossDefeatedTimer--;
        if (bossDefeatedTimer == 0) {
            if (waveNumber == 30) {
                enterVictory();
            } else {
                enterPerkChoice();
            }
            return;
        }
    }

    if (!waveActive && waveClearTimer == 0 && bossDefeatedTimer == 0
            && state == GameState::Play) {
        spawnWave();
        waveAnnouncementStart(waveNumber);
        waveActive = true;
    }

    // Check death
    if (gPlayer.hp <= 0) {
        enterGameOver();
    }
}

static void updatePerkChoice() {
    if (perkChoiceUpdate()) {
        enterShop();
    }
}

static void updateShop() {
    if (shopUpdate()) {
        startNextWave();
    }
}

static void updateGameOver() {
    if (keysDown() & KEY_START) {
        enterMenu();
    }
}

static void updateVictory() {
    u32 kd = keysDown();
    if (kd & KEY_START) {
        // Continue to endless mode
        startNextWave();
    }
    if (kd & KEY_A) {
        enterMenu();
    }
}

// --- Per-state render ---

static void renderPlayState() {
    renderGameplay();
    waveAnnouncementRender();
    uiRenderHUD();
}

static void renderPerkChoiceState() {
    perkChoiceRender();  // draws both top and bottom screens
}

static void renderShopState() {
    if (gShop.selectedCompanion >= 0) {
        renderCompanionDetail(gShop.selectedCompanion);
    } else {
        renderShopDetail(gShop.selectedCard);
    }
    uiRenderShop();
}

// --- Function pointer tables ---

using StateFunc = void (*)();

static const StateFunc updateTable[static_cast<int>(GameState::COUNT)] = {
    updateMenu,
    updatePlay,
    updatePerkChoice,
    updateShop,
    updateGameOver,
    updateVictory,
};

static const StateFunc renderTable[static_cast<int>(GameState::COUNT)] = {
    renderMenu,
    renderPlayState,
    renderPerkChoiceState,
    renderShopState,
    renderGameOver,
    renderVictory,
};

// --- Public API ---

void gameInit() {
    rngSeed(42); // TODO: seed from RTC or user input
    entitiesInit();
    playerInit();
    companionInit();
    perkInit();
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

int gameGetWave() {
    return waveNumber;
}

int gameGetWaveClearTimer() {
    return static_cast<int>(waveClearTimer);
}

int gameGetShopInterest() {
    return static_cast<int>(shopInterestEarned);
}

int gameGetWaveTimerSecs() {
    return static_cast<int>(gWaveTimer / 60);
}

u8 gameGetWaveTimerPct() {
    if (gWaveTimerMax == 0) return 0;
    return static_cast<u8>((gWaveTimer * 255) / gWaveTimerMax);
}

u8 gameGetAnnounceTimer() {
    return waveAnnounceTimer;
}

u8 gameGetBossDefeatedTimer() {
    return bossDefeatedTimer;
}
