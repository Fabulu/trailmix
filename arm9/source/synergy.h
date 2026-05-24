#ifndef SYNERGY_H
#define SYNERGY_H

#include "types.h"

// ---------------------------------------------------------------------------
// Synergy System
// 6 colors, 4 tiers each (activated at 2/3/4/5 companions of that color).
// Each tier adds a VISIBLE, IMPACTFUL effect.
//
// RED   - Wrath:   offense escalation (fire trails, kill explosions, pierce, aura)
// BLUE  - Arcane:  control (slow bullets, freeze-on-hit, ricochet, time warp)
// GREEN - Nature:  sustain (regen, heal orbs, shield, companion regen)
// YELLOW- Fortune: economy (gold bonus, free reroll, interest, mega coins)
// PURPLE- Hex:     debuffs (slow-on-hit, poison-on-kill, damage reduce, weakness)
// CYAN  - Volt:    tech (fire rate, chain-on-kill, EMP pulse, chain-on-hit)
// ---------------------------------------------------------------------------

constexpr int SYNERGY_TIER_COUNT = 4;
constexpr int SYNERGY_THRESHOLDS[SYNERGY_TIER_COUNT] = { 2, 3, 4, 5 };

// Chain-lightning arc visual
constexpr int MAX_CHAIN_ARCS = 8;
struct ChainArc {
    Vec2 from;
    Vec2 to;
    u8   timer;   // frames remaining (counts down, 0 = inactive)
};

struct SynergyState {
    s8 tier[PILL_COLOR_COUNT];  // -1 if inactive, 0..3 = active tier

    // --- RED (Wrath) ---
    u16 redKillCounter;         // T1: counts kills; every 5th triggers 16px AoE
    u8  redAuraTimer;           // T3: ticks 0..29 for fire aura damage

    // --- BLUE (Arcane) ---
    u8  blueFreezeFlash;        // T1: frames of freeze-flash remaining

    // --- GREEN (Nature) ---
    u16 greenHealTimer;         // T0: counts frames for 1HP/5s heal
    u16 greenShieldTimer;       // T2: counts frames for shield recharge (600f)
    bool greenShieldReady;      // T2: true when shield can absorb a hit
    u16 greenCompRegenTimer;    // T3: counts frames for companion regen (180f)
    u8  healFlashTimer;         // T0: visual green flash on player (8 frames)

    // --- YELLOW (Fortune) ---
    u16 yellowKillCounter;      // T3: counts kills; every 10th drops mega coin
    bool yellowFreeRerollUsed;  // T1: reset each shop entry

    // --- CYAN (Volt) ---
    u8  cyanEmpTimer;           // T2: counts 0..59 for EMP pulse

    // --- Visual arcs (Cyan chain lightning) ---
    ChainArc chainArcs[MAX_CHAIN_ARCS];
};

extern SynergyState gSynergy;

// Initialize synergy state (call at game start)
void synergyInit();

// Recalculate synergy tiers from current companion roster.
// Call after any companion add/remove/merge.
void synergyRecalc();

// Per-frame update: fire aura, EMP pulse, heal timers, shield recharge, etc.
// Call once per frame during gameplay.
void synergyUpdate();

// Called when an enemy is killed. Handles:
//   Red T1:    kill-explosion every 5th kill
//   Green T1:  spawn healing orb
//   Yellow T3: mega coin every 10th kill
//   Purple T1: poison zone on kill
//   Cyan T1:   chain to nearest enemy
void synergyOnEnemyKill(Vec2 deathPos, u8 bulletColor, s16 enemyMaxHp);

// Called when player takes damage. Handles:
//   Blue T1:   freeze all enemies for 30f
//   Green T2:  shield absorbs hit (returns 0 damage)
//   Purple T2: reduce damage by 30% if enemy is near player
// Returns modified damage amount.
s16 synergyOnPlayerHit(s16 rawDamage);

// Spawn a chain-arc visual (for Cyan chain lightning rendering)
void synergySpawnChainArc(Vec2 from, Vec2 to);

// --- Queries (used by combat, companion, shop, render) ---

// Red T0: bullets leave fire trails?
bool synergyFireTrails();

// Red T2: all bullets pierce?
bool synergyBulletsPierce();

// Blue T2: bullets ricochet off walls?
bool synergyBulletsRicochet();

// Blue T3: enemy speed multiplier. Returns 75 (=75%) or 100.
int synergyEnemySpeedPct();

// Purple T0: hits slow enemies?
bool synergyHitsSlow();

// Purple T3: enemies take +25% damage? Returns 125 or 100.
int synergyEnemyDamageTakenPct();

// Cyan T0: companion cooldown multiplier. Returns 85 (=85%) or 100.
int synergyCyanCooldownPct();

// Yellow T0: gold drop multiplier. Returns 120 (=+20%) or 100.
int synergyGoldDropPct();

// Yellow T1: free reroll available?
bool synergyFreeReroll();
void synergyMarkFreeRerollUsed();

// Yellow T2: interest cap bonus. Returns 0 or 3.
int synergyInterestCapBonus();

// Green T1: drop heal orbs on kill?
bool synergyDropHealOrbs();

// Green T2: shield currently absorbing?
bool synergyShieldActive();

// Purple T1: poison zone on kill?
bool synergyPoisonOnKill();

// Cyan T1: chain lightning on kill?
bool synergyCyanChainOnKill();

// Cyan T3: chain on every bullet hit?
bool synergyCyanChainOnHit();

// How many companions of a given color are currently active?
int synergyColorCount(int colorIndex);

// Highest active tier for a color (-1 if none)
int synergyLevel(int colorIndex);

#endif // SYNERGY_H
