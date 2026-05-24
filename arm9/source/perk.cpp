#include "perk.h"
#include "player.h"
#include "companion.h"
#include "synergy.h"
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
