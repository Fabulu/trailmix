#ifndef PERK_H
#define PERK_H

#include <nds.h>

constexpr int PERK_COUNT = 15;

enum PerkId : u8 {
    // ── OFFENSE (4) ──
    PERK_BULLET_HELL = 0,  // player fires in all 4 directions
    PERK_OVERCHARGE,       // all companions fire 50% faster
    PERK_GLASS_CANNON,     // player +5 damage, but max HP halved
    PERK_CHAIN_LIGHTNING,  // companion kills zap 1 nearby enemy for 50% dmg

    // ── DEFENSE (4) ──
    PERK_PHOENIX_DOWN,     // first companion to die revives once per wave
    PERK_FORTRESS,         // player +40 HP (25% slower), companions +20 HP
    PERK_SHIELD_BASH,      // dashing into enemies deals 8 damage
    PERK_SECOND_WIND,      // drop below 20% HP → full heal once per run

    // ── ECONOMY (4) ──
    PERK_GOLD_FEVER,       // 2x gold drops for 3 waves after purchase
    PERK_WAR_CHEST,        // 15g now, +3g each wave
    PERK_JACKPOT,          // interest cap raised to 10 (from 5)
    PERK_WHOLESALE,        // all companions cost 30% less

    // ── UTILITY (3) ──
    PERK_PACK_RAT,         // +1 companion slot
    PERK_WARP_DRIVE,       // dash goes 2x distance and leaves damage trail
    PERK_SOUL_SURGE,       // wave clear heals all units 20% maxHP
};

// Per-perk price table
static const u16 kPerkPrice[PERK_COUNT] = {
    60,  // Bullet Hell
    70,  // Overcharge
    75,  // Glass Cannon
    65,  // Chain Lightning
    60,  // Phoenix Down
    65,  // Fortress
    55,  // Shield Bash
    70,  // Second Wind
    50,  // Gold Fever
    65,  // War Chest
    60,  // Jackpot
    80,  // Wholesale
    55,  // Pack Rat
    65,  // Warp Drive
    70,  // Soul Surge
};

struct PerkState {
    bool active[PERK_COUNT];
    u8 phoenixUsedThisWave;  // for PHOENIX_DOWN: reset each wave
    u8 secondWindUsed;       // for SECOND_WIND: once per run
    u8 goldFeverWaves;       // for GOLD_FEVER: countdown
    bool warChestActive;     // for WAR_CHEST: +3g each wave
};

extern PerkState gPerks;

void perkInit();
void perkApplyOnBuy(PerkId id);
bool perkIsActive(PerkId id);

// Get a random perk that isn't already owned. Returns -1 if all owned.
int perkGetRandom();

// Get the max companion count (base 5 + Pack Rat)
int perkMaxCompanions();

// Get interest cap (base 5 + Jackpot)
int perkInterestCap();

// Called at wave start to reset per-wave perk state
void perkOnWaveStart();

// Called when player takes damage (for Second Wind check)
void perkOnPlayerHit();

// Called on wave clear: Soul Surge heals all units 20% maxHP
void perkOnWaveClear();

#endif
