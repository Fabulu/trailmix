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
        default:          return 800;  // boss base HP — must survive long enough to use abilities
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
// Wave templates for waves 1-15
// 16+ use procedural scaling
// --------------------------------------------------------------------------
static const WaveTemplate kWaveTemplates[15] = {
    // Wave 1: 6 small Grunts — line advance; learn to shoot
    { {{ ETYPE_GRUNT, SIZE_SMALL, 6, 0 }}, 1, false },

    // Wave 2: 8 small Grunts + 2 medium Grunts — surround pressure
    { {{ ETYPE_GRUNT, SIZE_SMALL,  8, 2 },
       { ETYPE_GRUNT, SIZE_MEDIUM, 2, 0 }}, 2, false },

    // Wave 3: Fish School of 8 small Grunts — first swarm pattern
    { {{ ETYPE_GRUNT, SIZE_SMALL, 8, 4 }}, 1, false },

    // Wave 4: 4 medium Grunts + 2 small Chargers — introduce flankers
    { {{ ETYPE_GRUNT,   SIZE_MEDIUM, 4, 0 },
       { ETYPE_CHARGER, SIZE_SMALL,  2, 1 }}, 2, false },

    // Wave 5: BOSS — Sentinel + 4 small Grunt adds
    { {{ ETYPE_BOSS_SENTINEL, SIZE_LARGE, 1, 5 },
       { ETYPE_GRUNT,         SIZE_SMALL, 4, 3 }}, 2, true },

    // Wave 6: Pincer formation — 2 groups of 3 Chargers
    { {{ ETYPE_CHARGER, SIZE_SMALL,  3, 1 },
       { ETYPE_CHARGER, SIZE_MEDIUM, 3, 1 }}, 2, false },

    // Wave 7: Spitters introduced — 4 medium Spitters + 6 small Grunts
    { {{ ETYPE_SPITTER, SIZE_MEDIUM, 4, 3 },
       { ETYPE_GRUNT,   SIZE_SMALL,  6, 2 }}, 2, false },

    // Wave 8: 1 large Brute + swarm of 8 small Grunts (fish school)
    { {{ ETYPE_BRUTE, SIZE_LARGE, 1, 3 },
       { ETYPE_GRUNT, SIZE_SMALL, 8, 4 }}, 2, false },

    // Wave 9: Mixed ranged — 3 Snipers + 3 Spitters + 4 Grunts
    { {{ ETYPE_SNIPER,  SIZE_MEDIUM, 3, 3 },
       { ETYPE_SPITTER, SIZE_MEDIUM, 3, 3 },
       { ETYPE_GRUNT,   SIZE_SMALL,  4, 0 }}, 3, false },

    // Wave 10: BOSS — Dreadnought + Spitter adds
    { {{ ETYPE_BOSS_DREADNOUGHT, SIZE_LARGE,  1, 5 },
       { ETYPE_SPITTER,          SIZE_SMALL,  4, 2 }}, 2, true },

    // Wave 11: Splitters — kill them and they multiply!
    { {{ ETYPE_SPLITTER, SIZE_MEDIUM, 6, 2 },
       { ETYPE_SPLITTER, SIZE_LARGE,  2, 3 }}, 2, false },

    // Wave 12: Shield enemies — must flank them
    { {{ ETYPE_SHIELD,  SIZE_MEDIUM, 4, 1 },
       { ETYPE_CHARGER, SIZE_SMALL,  4, 3 },
       { ETYPE_GRUNT,   SIZE_SMALL,  4, 2 }}, 3, false },

    // Wave 13: Spiral Close (Ghosts + Artillery) — teleport chaos + lobbed explosives
    { {{ ETYPE_GHOST,     SIZE_SMALL,  4, 3 },
       { ETYPE_ARTILLERY, SIZE_MEDIUM, 3, 2 },
       { ETYPE_GRUNT,     SIZE_SMALL,  4, 4 }}, 3, false },

    // Wave 14: Everything mixed — large Brutes + Swarm Drones + ranged
    { {{ ETYPE_BRUTE,       SIZE_LARGE,  2, 3 },
       { ETYPE_SWARM_DRONE, SIZE_SMALL,  8, 4 },
       { ETYPE_SNIPER,      SIZE_MEDIUM, 2, 3 },
       { ETYPE_NIGHTMARE,   SIZE_MEDIUM, 1, 3 }}, 4, false },

    // Wave 15: BOSS — Leviathan + all enemy types as adds
    { {{ ETYPE_BOSS_LEVIATHAN, SIZE_LARGE,  1, 5 },
       { ETYPE_SPLITTER,       SIZE_MEDIUM, 2, 3 },
       { ETYPE_SPITTER,        SIZE_SMALL,  3, 3 },
       { ETYPE_CHARGER,        SIZE_SMALL,  3, 3 }}, 4, true },
};

// Endless (16+): pool of enemy types weighted by wave depth, randomised counts
static void spawnEndlessWave(int wave) {
    // Wave 30: final boss — no regular enemies
    if (wave == 30) {
        Vec2 pos = farthestEdgeFromPlayer();
        s16 bossHp = scaledHp(3000, wave);  // final boss — must survive full 3-phase fight
        spawnEnemy(pos, {0,0}, bossHp, ETYPE_BOSS_APOTHECARY, SIZE_LARGE, SPRITE_SIZE_BOSS_LARGE);
        return;
    }

    // How many total enemy-slots to fill, capped at MAX_ENEMIES - 1
    int budget = 8 + (wave - 15) * 2;
    if (budget > 28) budget = 28;

    // Increase variety and difficulty every 5 waves past wave 15
    int tier = (wave - 16) / 5;  // 0,1,2,3...
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

    // Every 5 waves past 15: add a roaming boss
    if ((wave - 15) % 5 == 0) {
        static const u8 bossTypes[4] = {
            ETYPE_BOSS_SENTINEL, ETYPE_BOSS_DREADNOUGHT,
            ETYPE_BOSS_LEVIATHAN, ETYPE_BOSS_NIGHTMARE_B
        };
        u8 bossType = bossTypes[((wave - 15) / 5 - 1) % 4];
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

    if (waveNumber <= 15) {
        // Scripted wave: use template
        const WaveTemplate& tmpl = kWaveTemplates[waveNumber - 1];
        for (int g = 0; g < tmpl.groupCount; g++) {
            spawnGroup(tmpl.groups[g], waveNumber);
        }
    } else {
        // Endless: procedural scaling
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
        bool bossWave = (waveNumber <= 15)
                        ? kWaveTemplates[waveNumber - 1].isBossWave
                        : ((waveNumber - 15) % 5 == 0);
        if (bossWave) bossDefeatedTimer = 90;
    }

    // Tick announcement timer every frame
    waveAnnouncementTick();

    // Show "WAVE CLEAR!" banner, then transition to PerkChoice (boss) or Shop
    if (!waveActive && waveClearTimer > 0) {
        waveClearTimer--;
        if (waveClearTimer == 0) {
            bool bossWave = (waveNumber <= 15)
                        ? kWaveTemplates[waveNumber - 1].isBossWave
                        : ((waveNumber - 15) % 5 == 0);
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
