// synergy.cpp — Synergy system: recalc tiers, per-frame effects, event handlers.
// See synergy.h for the full design.

#include "synergy.h"
#include "entities.h"
#include "companion.h"
#include "player.h"
#include "audio.h"
#include "perk.h"
#include "rng.h"

SynergyState gSynergy;

// ---------------------------------------------------------------------------
// Init / recalc
// ---------------------------------------------------------------------------

void synergyInit() {
    for (int i = 0; i < PILL_COLOR_COUNT; ++i) gSynergy.tier[i] = -1;
    gSynergy.redKillCounter      = 0;
    gSynergy.redAuraTimer        = 0;
    gSynergy.blueFreezeFlash     = 0;
    gSynergy.greenHealTimer      = 0;
    gSynergy.greenShieldTimer    = 0;
    gSynergy.greenShieldReady    = false;
    gSynergy.greenCompRegenTimer = 0;
    gSynergy.healFlashTimer      = 0;
    gSynergy.yellowKillCounter   = 0;
    gSynergy.yellowFreeRerollUsed = false;
    gSynergy.cyanEmpTimer        = 0;
    for (int i = 0; i < MAX_CHAIN_ARCS; ++i) gSynergy.chainArcs[i].timer = 0;
}

void synergyRecalc() {
    for (int c = 0; c < PILL_COLOR_COUNT; ++c) {
        int count = synergyColorCount(c);
        s8 tier = -1;
        for (int t = SYNERGY_TIER_COUNT - 1; t >= 0; --t) {
            if (count >= SYNERGY_THRESHOLDS[t]) { tier = static_cast<s8>(t); break; }
        }
        gSynergy.tier[c] = tier;

        // When Green shield tier is newly gained, start the recharge timer
        if (c == static_cast<int>(PillColor::Green)) {
            if (tier >= 2 && !gSynergy.greenShieldReady && gSynergy.greenShieldTimer == 0) {
                gSynergy.greenShieldTimer = 60; // short initial charge
            }
            if (tier < 2) {
                gSynergy.greenShieldReady = false;
                gSynergy.greenShieldTimer = 0;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Query helpers
// ---------------------------------------------------------------------------

int synergyColorCount(int colorIndex) {
    // Wildcard: every companion counts for ALL colors
    if (perkIsActive(PERK_WILDCARD)) {
        int n = 0;
        for (int i = 0; i < MAX_COMPANIONS; ++i)
            if (gCompanions[i].active) n++;
        return n;
    }
    int n = 0;
    for (int i = 0; i < MAX_COMPANIONS; ++i)
        if (gCompanions[i].active &&
            static_cast<int>(gCompanions[i].color) == colorIndex) n++;
    return n;
}

int synergyLevel(int colorIndex) {
    return gSynergy.tier[colorIndex];
}

bool synergyFireTrails()         { return gSynergy.tier[static_cast<int>(PillColor::Red)]    >= 0; }
bool synergyBulletsPierce()      { return gSynergy.tier[static_cast<int>(PillColor::Red)]    >= 2; }
bool synergyBulletsRicochet()    { return gSynergy.tier[static_cast<int>(PillColor::Blue)]   >= 2; }
int  synergyEnemySpeedPct()      { return gSynergy.tier[static_cast<int>(PillColor::Blue)]   >= 3 ? 75 : 100; }
bool synergyHitsSlow()           { return gSynergy.tier[static_cast<int>(PillColor::Purple)] >= 0; }
bool synergyPoisonOnKill()       { return gSynergy.tier[static_cast<int>(PillColor::Purple)] >= 1; }
int  synergyEnemyDamageTakenPct(){ return gSynergy.tier[static_cast<int>(PillColor::Purple)] >= 3 ? 125 : 100; }
int  synergyCyanCooldownPct()    { return gSynergy.tier[static_cast<int>(PillColor::Cyan)]   >= 0 ? 85  : 100; }
int  synergyGoldDropPct()        { return gSynergy.tier[static_cast<int>(PillColor::Yellow)] >= 0 ? 120 : 100; }
bool synergyFreeReroll()         { return gSynergy.tier[static_cast<int>(PillColor::Yellow)] >= 1 && !gSynergy.yellowFreeRerollUsed; }
void synergyMarkFreeRerollUsed() { gSynergy.yellowFreeRerollUsed = true; }
int  synergyInterestCapBonus()   { return gSynergy.tier[static_cast<int>(PillColor::Yellow)] >= 2 ? 6  : 0; }
bool synergyDropHealOrbs()       { return gSynergy.tier[static_cast<int>(PillColor::Green)]  >= 1; }
bool synergyShieldActive()       { return gSynergy.tier[static_cast<int>(PillColor::Green)]  >= 2 && gSynergy.greenShieldReady; }
bool synergyCyanChainOnKill()    { return gSynergy.tier[static_cast<int>(PillColor::Cyan)]   >= 1; }
bool synergyCyanChainOnHit()     { return gSynergy.tier[static_cast<int>(PillColor::Cyan)]   >= 3; }

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void synergyUpdate() {
    // ── Chain-arc visual timers ─────────────────────────────────────────
    for (int i = 0; i < MAX_CHAIN_ARCS; ++i)
        if (gSynergy.chainArcs[i].timer > 0) gSynergy.chainArcs[i].timer--;

    // ── RED T3: fire aura around player (8px, 1 damage every 30 frames) ─
    if (gSynergy.tier[static_cast<int>(PillColor::Red)] >= 3) {
        gSynergy.redAuraTimer++;
        if (gSynergy.redAuraTimer >= 30) {
            gSynergy.redAuraTimer = 0;
            int px = gPlayer.pos.pixelX(), py = gPlayer.pos.pixelY();
            for (auto& e : gEnemies) {
                if (!e.active) continue;
                int dx = e.pos.pixelX() - px, dy = e.pos.pixelY() - py;
                if (dx * dx + dy * dy <= 8 * 8) {
                    e.hp -= 1;
                    spawnParticle(e.pos, {0, static_cast<Fixed>(-FP_ONE)}, 8,
                                 static_cast<u8>(PillColor::Red));
                    if (e.hp <= 0) {
                        e.active = false;
                        spawnParticleBurst(e.pos, 8, 12,
                                           static_cast<u8>(PillColor::Red));
                        audioPlaySfx(GSFX_EXPLODE);
                    }
                }
            }
        }
        // Fire aura particles (every 4 frames)
        if ((gSynergy.redAuraTimer & 3) == 0) {
            Vec2 ppos = gPlayer.pos;
            ppos.x = static_cast<Fixed>(ppos.x + toFixed(rngRange(16) - 8));
            ppos.y = static_cast<Fixed>(ppos.y + toFixed(rngRange(16) - 8));
            spawnParticle(ppos, {0, static_cast<Fixed>(-FP_ONE)}, 10,
                          static_cast<u8>(PillColor::Red));
        }
    } else {
        gSynergy.redAuraTimer = 0;
    }

    // ── GREEN T0: player HP regen (1 HP every 300 frames = 5 s) ────────
    if (gSynergy.tier[static_cast<int>(PillColor::Green)] >= 0) {
        gSynergy.greenHealTimer++;
        if (gSynergy.greenHealTimer >= 300) {
            gSynergy.greenHealTimer = 0;
            if (gPlayer.hp < gPlayer.maxHp) {
                gPlayer.hp++;
                gSynergy.healFlashTimer = 8;
            }
        }
    } else {
        gSynergy.greenHealTimer = 0;
    }
    if (gSynergy.healFlashTimer > 0) gSynergy.healFlashTimer--;

    // ── GREEN T2: shield recharge (600 frames = 10s after breaking) ────
    if (gSynergy.tier[static_cast<int>(PillColor::Green)] >= 2 && !gSynergy.greenShieldReady) {
        if (gSynergy.greenShieldTimer > 0) {
            gSynergy.greenShieldTimer--;
        } else {
            gSynergy.greenShieldReady = true;
        }
    }

    // ── GREEN T3: companion regen (1 HP every 180 frames = 3s) ─────────
    if (gSynergy.tier[static_cast<int>(PillColor::Green)] >= 3) {
        gSynergy.greenCompRegenTimer++;
        if (gSynergy.greenCompRegenTimer >= 180) {
            gSynergy.greenCompRegenTimer = 0;
            for (int i = 0; i < MAX_COMPANIONS; ++i) {
                Companion& c = gCompanions[i];
                if (!c.active) continue;
                if (c.hp < c.maxHp) {
                    c.hp++;
                    c.healFlashTimer = 8;
                }
            }
        }
    } else {
        gSynergy.greenCompRegenTimer = 0;
    }

    // ── CYAN T2: EMP pulse every 60 frames — stun nearest enemy 30f ────
    if (gSynergy.tier[static_cast<int>(PillColor::Cyan)] >= 2) {
        gSynergy.cyanEmpTimer++;
        if (gSynergy.cyanEmpTimer >= 60) {
            gSynergy.cyanEmpTimer = 0;
            Enemy* nearest = nullptr;
            s32 bestDist = 0x7FFFFFFF;
            for (auto& e : gEnemies) {
                if (!e.active) continue;
                Vec2 d = e.pos - gPlayer.pos;
                s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                if (dist < bestDist) { bestDist = dist; nearest = &e; }
            }
            if (nearest) {
                nearest->freezeTimer = 30;
                spawnParticleBurst(nearest->pos, 6, 10,
                                   static_cast<u8>(PillColor::Cyan));
            }
            // EMP ring visual particles
            spawnParticleBurst(gPlayer.pos, 8, 14,
                               static_cast<u8>(PillColor::Cyan));
        }
    } else {
        gSynergy.cyanEmpTimer = 0;
    }

    // ── BLUE T1: freeze-flash visual decay ──────────────────────────────
    if (gSynergy.blueFreezeFlash > 0) gSynergy.blueFreezeFlash--;
}

// ---------------------------------------------------------------------------
// Enemy kill event
// ---------------------------------------------------------------------------

void synergyOnEnemyKill(Vec2 deathPos, u8 bulletColor, s16 enemyMaxHp) {
    // Red T1: explosion every 5th kill (16px AoE, 5 damage)
    if (gSynergy.tier[static_cast<int>(PillColor::Red)] >= 1) {
        gSynergy.redKillCounter++;
        if (gSynergy.redKillCounter >= 5) {
            gSynergy.redKillCounter = 0;
            int bx = toInt(deathPos.x), by = toInt(deathPos.y);
            for (auto& e : gEnemies) {
                if (!e.active) continue;
                int dx = e.pos.pixelX() - bx, dy = e.pos.pixelY() - by;
                if (dx * dx + dy * dy <= 16 * 16) {
                    e.hp -= 5;
                    if (e.hp <= 0) e.active = false;
                }
            }
            spawnParticleBurst(deathPos, 16, 18,
                               static_cast<u8>(PillColor::Red));
            audioPlaySfx(GSFX_EXPLODE);
        }
    }

    // Green T1: drop a healing orb at the kill location
    if (gSynergy.tier[static_cast<int>(PillColor::Green)] >= 1) {
        spawnHealOrb(deathPos, 2);
    }

    // Yellow T3: every 10th kill drops a mega coin worth 10g
    if (gSynergy.tier[static_cast<int>(PillColor::Yellow)] >= 3) {
        gSynergy.yellowKillCounter++;
        if (gSynergy.yellowKillCounter >= 10) {
            gSynergy.yellowKillCounter = 0;
            spawnGold(deathPos, 10);
            spawnParticleBurst(deathPos, 10, 14,
                               static_cast<u8>(PillColor::Yellow));
        }
    }

    // Purple T1: poison zone at death point (8px, 1 dmg/20f, 90f duration)
    if (gSynergy.tier[static_cast<int>(PillColor::Purple)] >= 1) {
        spawnZone(deathPos, 8, 1, 20, 90,
                  static_cast<u8>(PillColor::Purple), ZONE_POISON);
    }

    // Cyan T1: chain lightning to nearest enemy within 64px
    if (gSynergy.tier[static_cast<int>(PillColor::Cyan)] >= 1) {
        Enemy* nearest = nullptr;
        s32 bestDist = static_cast<s32>(toFixed(64)) * toFixed(64);
        for (auto& e : gEnemies) {
            if (!e.active) continue;
            Vec2 d = e.pos - deathPos;
            s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
            if (dist < bestDist) { bestDist = dist; nearest = &e; }
        }
        if (nearest) {
            s16 chainDmg = (enemyMaxHp > 1) ? static_cast<s16>(enemyMaxHp / 2) : 1;
            nearest->hp -= chainDmg;
            synergySpawnChainArc(deathPos, nearest->pos);
            spawnParticleBurst(nearest->pos, 4, 8,
                               static_cast<u8>(PillColor::Cyan));
            if (nearest->hp <= 0) {
                nearest->active = false;
                spawnParticleBurst(nearest->pos, 8, 12,
                                   static_cast<u8>(PillColor::Cyan));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Player-hit event
// ---------------------------------------------------------------------------

s16 synergyOnPlayerHit(s16 rawDamage) {
    // Green T2: shield absorbs one hit entirely
    if (synergyShieldActive()) {
        gSynergy.greenShieldReady = false;
        gSynergy.greenShieldTimer = 600; // 10s recharge
        spawnParticleBurst(gPlayer.pos, 10, 12,
                           static_cast<u8>(PillColor::Blue));
        return 0;
    }

    // Blue T1: freeze all enemies for 30 frames on player hit
    if (gSynergy.tier[static_cast<int>(PillColor::Blue)] >= 1) {
        gSynergy.blueFreezeFlash = 8;
        for (auto& e : gEnemies) {
            if (e.active && e.freezeTimer < 30) e.freezeTimer = 30;
        }
    }

    // Purple T2: reduce incoming damage by 30%
    if (gSynergy.tier[static_cast<int>(PillColor::Purple)] >= 2) {
        rawDamage = static_cast<s16>(rawDamage * 70 / 100);
        if (rawDamage < 1) rawDamage = 1;
    }

    return rawDamage;
}

// ---------------------------------------------------------------------------
// Chain-arc spawn
// ---------------------------------------------------------------------------

void synergySpawnChainArc(Vec2 from, Vec2 to) {
    for (int i = 0; i < MAX_CHAIN_ARCS; ++i) {
        if (gSynergy.chainArcs[i].timer == 0) {
            gSynergy.chainArcs[i].from  = from;
            gSynergy.chainArcs[i].to    = to;
            gSynergy.chainArcs[i].timer = 12;
            return;
        }
    }
    // All slots full; overwrite slot 0
    gSynergy.chainArcs[0] = { from, to, 12 };
}
