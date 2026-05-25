#include "perk.h"
#include "player.h"
#include "companion.h"
#include "synergy.h"
#include "entities.h"
#include "rng.h"
#include <string.h>

PerkState gPerks;

void perkInit() {
    memset(&gPerks, 0, sizeof(gPerks));
}

void perkApplyOnBuy(PerkId id) {
    gPerks.active[id] = true;

    switch (id) {
    case PERK_GLASS_CANNON:
        gPlayer.bulletDamage += 5;
        gPlayer.maxHp /= 2;
        if (gPlayer.hp > gPlayer.maxHp)
            gPlayer.hp = gPlayer.maxHp;
        for (int i = 0; i < MAX_COMPANIONS; i++) {
            if (!gCompanions[i].active) continue;
            gCompanions[i].maxHp /= 2;
            if (gCompanions[i].maxHp < 1) gCompanions[i].maxHp = 1;
            if (gCompanions[i].hp > gCompanions[i].maxHp)
                gCompanions[i].hp = gCompanions[i].maxHp;
        }
        break;
    case PERK_FORTRESS:
        gPlayer.maxHp += 40;
        gPlayer.hp += 40;
        // Companions: +20 HP
        for (int i = 0; i < MAX_COMPANIONS; i++) {
            if (!gCompanions[i].active) continue;
            gCompanions[i].maxHp += 20;
            gCompanions[i].hp += 20;
        }
        // speed reduction applied in playerUpdate via perkIsActive check
        break;
    case PERK_GOLD_FEVER:
        gPerks.goldFeverWaves = 3;
        break;
    case PERK_WAR_CHEST:
        gPlayer.gold += 15;
        gPerks.warChestActive = true;
        break;
    case PERK_LOAN_SHARK:
        gPlayer.gold += 80;
        gPerks.loanWavesLeft = 10;
        break;
    default:
        break;
    }
}

bool perkIsActive(PerkId id) {
    return gPerks.active[id];
}

int perkGetRandom() {
    int candidates[PERK_COUNT];
    int count = 0;

    for (int i = 0; i < PERK_COUNT; i++) {
        if (!gPerks.active[i]) {
            // Wildcard is ultra-rare: only 20% chance to appear in the pool
            if (i == PERK_WILDCARD && rngRange(5) != 0) continue;
            candidates[count++] = i;
        }
    }

    if (count == 0) return -1;
    return candidates[rngRange(count)];
}

int perkMaxCompanions() {
    return 5 + (gPerks.active[PERK_PACK_RAT] ? 1 : 0);
}

int perkInterestCap() {
    int cap = gPerks.active[PERK_JACKPOT] ? 10 : 5;
    cap += synergyInterestCapBonus(); // Yellow T2: +3
    return cap;
}

void perkOnWaveStart() {
    gPerks.phoenixUsedThisWave = 0;

    // Gold Fever countdown
    if (gPerks.goldFeverWaves > 0) {
        gPerks.goldFeverWaves--;
    }

    // War Chest: +3g each wave
    if (gPerks.warChestActive) {
        gPlayer.gold += 3;
    }

    // Bloodlust: reset kill count each wave
    gPerks.killCount = 0;

    // Loan Shark: deduct 10g per wave
    if (gPerks.loanWavesLeft > 0) {
        if (gPlayer.gold >= 10)
            gPlayer.gold -= 10;
        else
            gPlayer.gold = 0;
        gPerks.loanWavesLeft--;
    }

    // Double or Nothing: coin flip for the wave
    gPerks.doubleGoldWave = false;
    gPerks.zeroGoldWave = false;
    if (gPerks.active[PERK_DOUBLE_OR_NOTHING]) {
        if (rngRange(2) == 0)
            gPerks.doubleGoldWave = true;
        else
            gPerks.zeroGoldWave = true;
    }

    // Bounty Board: pick a random active enemy
    gPerks.bountyEnemyIdx = -1;
    if (gPerks.active[PERK_BOUNTY_BOARD]) {
        // Will be assigned on first frame when enemies exist
        // (enemies may not be spawned yet at wave start call)
        gPerks.bountyEnemyIdx = -2; // sentinel: needs assignment
    }

    // Rewind: set fearTimer on all active enemies
    if (gPerks.active[PERK_REWIND]) {
        for (auto& e : gEnemies) {
            if (e.active) e.fearTimer = 120; // 2 seconds at 60fps
        }
    }
}

void perkOnWaveClear() {
    // Soul Surge: heal all units (player + companions) for 20% maxHP
    if (gPerks.active[PERK_SOUL_SURGE]) {
        s16 playerHeal = static_cast<s16>(gPlayer.maxHp * 20 / 100);
        if (playerHeal < 1) playerHeal = 1;
        gPlayer.hp += playerHeal;
        if (gPlayer.hp > gPlayer.maxHp) gPlayer.hp = gPlayer.maxHp;

        for (int i = 0; i < MAX_COMPANIONS; i++) {
            if (!gCompanions[i].active) continue;
            s16 heal = static_cast<s16>(gCompanions[i].maxHp * 20 / 100);
            if (heal < 1) heal = 1;
            gCompanions[i].hp += heal;
            if (gCompanions[i].hp > gCompanions[i].maxHp)
                gCompanions[i].hp = gCompanions[i].maxHp;
        }
    }
}

void perkOnPlayerHit() {
    // Second Wind: heal to full once per run when dropping below 20% HP
    if (gPerks.active[PERK_SECOND_WIND] && !gPerks.secondWindUsed) {
        if (gPlayer.hp > 0 && gPlayer.hp <= gPlayer.maxHp / 5) {
            gPlayer.hp = gPlayer.maxHp;
            gPerks.secondWindUsed = 1;
        }
    }
}

void perkOnEnemyKill(int enemyIdx) {
    // Bloodlust: track kills this wave
    if (gPerks.active[PERK_BLOODLUST]) {
        if (gPerks.killCount < 30) gPerks.killCount++;
    }
}

int perkBloodlustCooldownPct() {
    // Returns the cooldown percentage to apply (100 = normal, lower = faster)
    if (!gPerks.active[PERK_BLOODLUST]) return 100;
    int reduction = gPerks.killCount * 3; // 3% per kill
    if (reduction > 90) reduction = 90;
    return 100 - reduction;
}
