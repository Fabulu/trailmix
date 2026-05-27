#ifndef PERK_H
#define PERK_H

#include <nds.h>

constexpr int PERK_COUNT = 30;

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
    PERK_WAR_CHEST,        // 30g now, +6g each wave
    PERK_JACKPOT,          // interest cap raised to 20 (from 10)
    PERK_WHOLESALE,        // all companions cost 30% less

    // ── UTILITY (3) ──
    PERK_PACK_RAT,         // +1 companion slot
    PERK_WARP_DRIVE,       // dash goes 2x distance and leaves damage trail
    PERK_SOUL_SURGE,       // wave clear heals all units 20% maxHP

    // ── TRAIL MIX: OFFENSE (3) ──
    PERK_RICOCHET,         // player bullets bounce off arena edges once
    PERK_BLOODLUST,        // +3% fire rate per kill this wave, caps +90%
    PERK_ECHO_CHAMBER,     // every 5th player shot fires a bonus ghost bullet

    // ── TRAIL MIX: DEFENSE (3) ──
    PERK_THORNS,           // companions deal 5 damage back when hit
    PERK_LAST_STAND,       // last companion alive gets 3x damage
    PERK_JUGGERNAUT,       // player takes 1 less damage from all sources (min 1)

    // ── TRAIL MIX: ECONOMY (4) ──
    PERK_DOUBLE_OR_NOTHING,// 50/50: double wave gold or zero
    PERK_SHORTCUT,         // merging needs only 2 instead of 3
    PERK_BLACK_MARKET,     // uncommon/rare odds improved
    PERK_MAGNET,           // double gold pickup range
    PERK_PENSION,          // +10g bonus at wave end
    PERK_LOAN_SHARK,       // get 160g now, lose 20g/wave for 10 waves

    // ── TRAIL MIX: UTILITY (3) ──
    PERK_REWIND,           // enemies flee for 2s at wave start
    PERK_BOUNTY_BOARD,     // one random enemy per wave drops 3x gold
    PERK_WILDCARD,         // merge any color together (same classId)
};

// Per-perk price table
static const u16 kPerkPrice[PERK_COUNT] = {
    120, // Bullet Hell
    140, // Overcharge
    150, // Glass Cannon
    130, // Chain Lightning
    120, // Phoenix Down
    130, // Fortress
    110, // Shield Bash
    140, // Second Wind
    100, // Gold Fever
    130, // War Chest
    120, // Jackpot
    160, // Wholesale
    110, // Pack Rat
    130, // Warp Drive
    140, // Soul Surge
    // ── Trail Mix ──
    140, // Ricochet
    120, // Bloodlust
    170, // Echo Chamber
    120, // Thorns
    150, // Last Stand
    180, // Juggernaut
    100, // Double or Nothing
    180, // Shortcut
    130, // Black Market
    110, // Magnet
    160, // Pension
    100, // Loan Shark
    150, // Rewind
    140, // Bounty Board
    200, // Wildcard
};

struct PerkState {
    bool active[PERK_COUNT];
    u8 phoenixUsedThisWave;  // for PHOENIX_DOWN: reset each wave
    u8 secondWindUsed;       // for SECOND_WIND: once per run
    u8 goldFeverWaves;       // for GOLD_FEVER: countdown
    bool warChestActive;     // for WAR_CHEST: +3g each wave
    // ── Trail Mix state ──
    u8 killCount;            // BLOODLUST: kills this wave (caps at 30)
    u8 loanWavesLeft;        // LOAN_SHARK: waves of 10g debt remaining
    bool doubleGoldWave;     // DOUBLE_OR_NOTHING: gold doubled this wave
    bool zeroGoldWave;       // DOUBLE_OR_NOTHING: gold zeroed this wave
    s8 bountyEnemyIdx;       // BOUNTY_BOARD: index of marked enemy (-1 = none)
    u8 shotCounter;          // ECHO_CHAMBER: player shots fired count
};

extern PerkState gPerks;

void perkInit();
void perkApplyOnBuy(PerkId id);
bool perkIsActive(PerkId id);

// Get a random perk that isn't already owned. Returns -1 if all owned.
int perkGetRandom();

// Get the max companion count (base 5 + Pack Rat)
int perkMaxCompanions();

// Get interest cap (base 10 + Jackpot)
int perkInterestCap();

// Called at wave start to reset per-wave perk state
void perkOnWaveStart();

// Called when player takes damage (for Second Wind check)
void perkOnPlayerHit();

// Called on wave clear: Soul Surge heals all units 20% maxHP
void perkOnWaveClear();

// Called when any enemy is killed (for Bloodlust, Bounty Board)
void perkOnEnemyKill(int enemyIdx);

// Get Bloodlust cooldown reduction percentage (0-90)
int perkBloodlustCooldownPct();

#endif
