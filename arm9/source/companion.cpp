#include "companion.h"
#include "player.h"
#include "entities.h"
#include "perk.h"
#include "audio.h"
#include "render.h"
#include "rng.h"
#include "synergy.h"

Companion gCompanions[MAX_COMPANIONS];
int gCompanionCount = 0;

const ClassDef kClassDefs[36] = {
    // RED (indices 0-5)                                              rarity
    { 15,  3, 1,  0, 0,          0, 0, 0 },  // R1 Gunner: Common
    { 40,  4, 1,  0, BFLAG_EXPLODE, 32, 0, 0 }, // R2 Pyromaniac: Common (big boom)
    { 35,  3, 3, 30, 0,          0, 0, 1 },  // R3 Shotgunner: Uncommon
    { 25,  4, 1,  0, 0,          0, 0, 1 },  // R4 Berserker: Uncommon
    { 50,  6, 1,  0, BFLAG_EXPLODE, 24, 0, 2 }, // R5 Detonator: Rare
    { 60,  8, 1,  0, BFLAG_PIERCE, 0, 0, 2 }, // R6 Executioner: Rare

    // BLUE (indices 6-11)
    { 25,  3, 1,  0, 0,          0, 0, 0 },  // B1 Warden: Common
    { 12,  2, 1,  0, 0,          0, 0, 0 },  // B2 Channeler: Common (fast focused shot)
    { 35,  3, 1,  0, BFLAG_SLOW, 0, 60, 1},  // B3 Frost Archer: Uncommon
    { 20,  2, 2,  0, 0,          0, 0, 1 },  // B4 Orbiter: Uncommon
    { 75,  5, 1,  0, BFLAG_FREEZE,0,45, 2 }, // B5 Chronomancer: Rare
    { 40,  2, 1,  0, BFLAG_ERASE, 0, 0, 2 }, // B6 Nullifier: Rare

    // GREEN (indices 12-17)
    { 20,  2, 1,  0, 0,          0, 0, 0 },  // G1 Thornshot: Common
    { 30,  3, 1,  0, 0,          0, 0, 0 },  // G2 Seedling: Common
    { 40,  2, 1,  0, 0,          0, 0, 1 },  // G3 Apothecary: Uncommon
    { 45,  3, 1, 60, 0,          0, 0, 1 },  // G4 Vinecaller: Uncommon
    { 50,  3, 3,  0, 0,          0, 0, 2 },  // G5 Sporewitch: Rare
    { 60,  1, 1,  0, 0,          0, 0, 2 },  // G6 Lifeshaper: Rare

    // YELLOW (indices 18-23)
    { 15,  3, 1,  0, 0,          0, 0, 0 },  // Y1 Gambler: Common
    { 18,  2, 1,  0, 0,          0, 0, 0 },  // Y2 Scavenger: Common
    { 25,  2, 1,  0, 0,          0, 0, 1 },  // Y3 Jester: Uncommon
    { 30,  3, 1,  0, 0,          0, 0, 1 },  // Y4 Mimic: Uncommon
    { 90,  1, 1,  0, 0,          0, 0, 2 },  // Y5 Alchemist: Rare
    { 20,  3, 1,  0, 0,          0, 0, 2 },  // Y6 Trickster: Rare

    // PURPLE (indices 24-29)
    { 30,  2, 1,  0, BFLAG_SLOW, 0, 45, 0},  // P1 Hexer: Common
    { 15,  1, 3, 20, 0,          0, 0, 0 },  // P2 Blightling: Common
    { 45,  3, 1,  0, BFLAG_ZONE,   16,0, 1},  // P3 Plague Doctor: Uncommon
    { 40,  3, 1,  0, BFLAG_PIERCE, 0, 0, 1 }, // P4 Wraith: Uncommon
    { 50,  4, 1,  0, BFLAG_FREEZE,0,90, 2},  // P5 Voidcaller: Rare
    { 35,  3, 1,  0, 0,          0, 0, 2 },  // P6 Nightmare: Rare

    // CYAN (indices 30-35)
    { 20,  2, 3,  0, 0,          0, 0, 0 },  // C1 Drone Pilot: Common
    { 10,  1, 1,  0, 0,          0, 0, 0 },  // C2 Overclocker: Common
    { 30,  4, 1,  0, 0,          0, 0, 1 },  // C3 Tesla Coil: Uncommon
    { 40,  2, 1,  0, BFLAG_ERASE, 0, 0, 1 }, // C4 Signal Jammer: Uncommon
    { 60,  3, 1,  0, BFLAG_FREEZE,0,180, 2}, // C5 Circuit Hacker: Rare
    { 80,  8, 1,  0, BFLAG_EXPLODE,32,0, 2},  // C6 Mech Pilot: Rare
};

const PassiveDef kPassiveDefs[36] = {
    // RED (indices 0-5)
    { PASSIVE_TEAM_DAMAGE,      8,  0 },  // R1 Gunner
    { PASSIVE_ENEMY_SLOW_AURA, 48, 10 },  // R2 Pyromaniac (burn=slow proxy)
    { PASSIVE_CLOSE_DAMAGE,    10, 40 },  // R3 Shotgunner
    { PASSIVE_TEAM_DAMAGE,     15,  0 },  // R4 Berserker
    { PASSIVE_NONE,             0,  0 },  // R5 Detonator
    { PASSIVE_NONE,             0,  0 },  // R6 Executioner

    // BLUE (indices 6-11)
    { PASSIVE_ENEMY_SLOW_AURA, 64, 10 },  // B1 Warden
    { PASSIVE_TEAM_COOLDOWN,    5,  0 },  // B2 Channeler
    { PASSIVE_TEAM_DAMAGE,     10,  0 },  // B3 Frost Archer
    { PASSIVE_NONE,             0,  0 },  // B4 Orbiter
    { PASSIVE_NONE,             0,  0 },  // B5 Chronomancer
    { PASSIVE_DEBUFF_EXTEND,   15,  0 },  // B6 Nullifier

    // GREEN (indices 12-17)
    { PASSIVE_REGEN,            1, 300 }, // G1 Thornshot
    { PASSIVE_FORMATION_TIGHT,  3,   0 }, // G2 Seedling
    { PASSIVE_REGEN,            1,   0 }, // G3 Apothecary
    { PASSIVE_NONE,             0,   0 }, // G4 Vinecaller
    { PASSIVE_NONE,             0,   0 }, // G5 Sporewitch
    { PASSIVE_MAX_HP,           5,   0 }, // G6 Lifeshaper

    // YELLOW (indices 18-23)
    { PASSIVE_REROLL_DISCOUNT,  2,  0 },  // Y1 Gambler
    { PASSIVE_GOLD_BONUS,      10,  0 },  // Y2 Scavenger
    { PASSIVE_TEAM_SPEED,       2,  0 },  // Y3 Jester
    { PASSIVE_NONE,             0,  0 },  // Y4 Mimic
    { PASSIVE_EXTRA_SHOP,       1,  0 },  // Y5 Alchemist
    { PASSIVE_GOLD_BONUS,      15,  0 },  // Y6 Trickster

    // PURPLE (indices 24-29)
    { PASSIVE_DEBUFF_EXTEND,   25,  0 },  // P1 Hexer
    { PASSIVE_GOLD_BONUS,      10,  0 },  // P2 Blightling
    { PASSIVE_TEAM_DAMAGE,      8,  0 },  // P3 Plague Doctor
    { PASSIVE_NONE,             0,  0 },  // P4 Wraith
    { PASSIVE_NONE,             0,  0 },  // P5 Voidcaller
    { PASSIVE_DEBUFF_EXTEND,   25,  0 },  // P6 Nightmare

    // CYAN (indices 30-35)
    { PASSIVE_NONE,             0,  0 },  // C1 Drone Pilot
    { PASSIVE_TEAM_COOLDOWN,   15,  0 },  // C2 Overclocker
    { PASSIVE_NONE,             0,  0 },  // C3 Tesla Coil
    { PASSIVE_DEBUFF_EXTEND,   10,  0 },  // C4 Signal Jammer
    { PASSIVE_NONE,             0,  0 },  // C5 Circuit Hacker
    { PASSIVE_TEAM_DAMAGE,     15,  0 },  // C6 Mech Pilot
};

PassiveState gPassive;
static u16 regenTimer = 0;

void companionApplyPassives() {
    // Reset accumulators
    gPassive = {};
    regenTimer++;

    for (int i = 0; i < MAX_COMPANIONS; i++) {
        if (!gCompanions[i].active) continue;
        int fci = companionFullClassId(gCompanions[i]);
        const PassiveDef& pd = kPassiveDefs[fci];
        u8 tierMult = (gCompanions[i].tier == 0) ? 1 : (gCompanions[i].tier == 1) ? 2 : 5;
        u8 mag = pd.param1 * tierMult;

        switch (pd.type) {
            case PASSIVE_TEAM_DAMAGE: gPassive.damageBoostPct += mag; break;
            case PASSIVE_TEAM_SPEED: gPassive.speedBonus += mag; break;
            case PASSIVE_TEAM_COOLDOWN: gPassive.cooldownReducePct += mag; break;
            case PASSIVE_GOLD_BONUS: gPassive.goldBonusPct += mag; break;
            case PASSIVE_PICKUP_RANGE: gPassive.pickupRangeBonus += mag; break;
            case PASSIVE_REROLL_DISCOUNT: gPassive.rerollDiscount += mag; break;
            case PASSIVE_EXTRA_SHOP: gPassive.extraShopCards += mag; break;
            case PASSIVE_DEBUFF_EXTEND:
                gPassive.debuffExtendPct += mag;
                break;
            case PASSIVE_FORMATION_TIGHT:
                // Reduce follow spacing — applied in companionUpdate()
                break;
            case PASSIVE_MAX_HP:
                // One-shot on acquire — not accumulated per frame
                break;
            case PASSIVE_CLOSE_DAMAGE:
                gPassive.closeDmgBoostPct += mag;
                if (pd.param2 > gPassive.closeDmgRange)
                    gPassive.closeDmgRange = static_cast<u8>(pd.param2);
                break;
            case PASSIVE_REGEN: {
                u16 interval = pd.param2 > 0 ? pd.param2 : 300;
                if (regenTimer % interval == 0) {
                    if (gPlayer.hp < gPlayer.maxHp) {
                        gPlayer.hp += mag;
                        if (gPlayer.hp > gPlayer.maxHp) gPlayer.hp = gPlayer.maxHp;
                    }
                    // Heal the lowest-HP active companion at 50% rate
                    Companion* lowestC = nullptr;
                    for (int j = 0; j < MAX_COMPANIONS; j++) {
                        if (!gCompanions[j].active) continue;
                        if (!lowestC || gCompanions[j].hp < lowestC->hp)
                            lowestC = &gCompanions[j];
                    }
                    if (lowestC && lowestC->hp < lowestC->maxHp) {
                        u8 halfMag = (mag / 2 > 0) ? mag / 2 : 1;
                        lowestC->hp += halfMag;
                        if (lowestC->hp > lowestC->maxHp) lowestC->hp = lowestC->maxHp;
                    }
                }
                break;
            }
            case PASSIVE_ENEMY_SLOW_AURA: {
                for (auto& e : gEnemies) {
                    if (!e.active) continue;
                    Vec2 d = e.pos - gCompanions[i].pos;
                    s32 dx = toInt(d.x), dy = toInt(d.y);
                    if (dx*dx + dy*dy < static_cast<s32>(pd.param1) * pd.param1) {
                        if (e.slowTimer < 4) e.slowTimer = 4;
                    }
                }
                break;
            }
            default: break;
        }
    }

    if (gPassive.damageBoostPct > 40) gPassive.damageBoostPct = 40;
    gPassive.companionDmgBoostPct = gPassive.damageBoostPct / 2;

    // Apply to player base stats
    gPlayer.bulletDamage = 3 + gPassive.damageBoostPct / 10; // +1 per 10%
    gPlayer.speed = toFixed(3) + static_cast<Fixed>(gPassive.speedBonus);
    int cd = 24 - (24 * gPassive.cooldownReducePct / 100);
    if (cd < 4) cd = 4;
    gPlayer.shootCooldown = static_cast<u8>(cd);
}

static Vec2 gTrailHistory[TRAIL_HIST_SIZE];
static int gTrailHead = 0;

void companionInit() {
    for (auto& c : gCompanions) {
        c.active = false;
    }
    gCompanionCount = 0;
    gTrailHead = 0;

    // Fill trail with player's current position
    for (int i = 0; i < TRAIL_HIST_SIZE; ++i) {
        gTrailHistory[i] = gPlayer.pos;
    }
}

// Derive visual bullet type from class flags, pill colour, and companion tier.
// Flag upgrades take priority; color + tier provide the base default.
static u8 bulletVisualType(u8 flags, PillColor color, u8 tier = 0) {
    if (flags & BFLAG_ZONE)    return BVIS_POISON;
    if (flags & BFLAG_EXPLODE) return BVIS_EXPLOSIVE;
    if (flags & BFLAG_PIERCE)  return BVIS_PIERCE;
    if (flags & (BFLAG_SLOW | BFLAG_FREEZE)) return BVIS_ICE;
    switch (color) {
        case PillColor::Blue:   return BVIS_ICE;
        case PillColor::Green:  return (tier >= 2) ? BVIS_NORMAL_T3 : (tier == 1) ? BVIS_NORMAL_T2 : BVIS_NORMAL;
        case PillColor::Yellow: return (tier >= 2) ? BVIS_NORMAL_T3 : (tier == 1) ? BVIS_NORMAL_T2 : BVIS_NORMAL;
        case PillColor::Purple: return BVIS_POISON;
        case PillColor::Cyan:   return BVIS_ELECTRIC;
        default:                return BVIS_FIRE;   // Red and any future colours
    }
}

// Spawn a bullet and immediately assign its visual type.
static Bullet* spawnCompanionBullet(Vec2 pos, Vec2 vel, PillColor color, u8 damage,
                                    u8 flags = 0, u8 aoeRadius = 0,
                                    u8 effectDuration = 0, u8 lifetime = 0, u8 tier = 0) {
    Bullet* b = spawnBullet(pos, vel, static_cast<u8>(color), damage,
                            flags, aoeRadius, effectDuration, lifetime);
    if (b) b->visualType = bulletVisualType(flags, color, tier);
    return b;
}

// Find nearest active enemy to a given position
static Enemy* findNearestEnemyFrom(Vec2 origin) {
    Enemy* nearest = nullptr;
    s32 bestDist = 0x7FFFFFFF;

    for (auto& e : gEnemies) {
        if (!e.active) continue;
        Vec2 diff = e.pos - origin;
        s32 dist = static_cast<s32>(diff.x) * diff.x + static_cast<s32>(diff.y) * diff.y;
        if (dist < bestDist) {
            bestDist = dist;
            nearest = &e;
        }
    }
    return nearest;
}

void companionUpdate() {
    // Push current player position into the ring buffer
    gTrailHistory[gTrailHead] = gPlayer.pos;
    gTrailHead = (gTrailHead + 1) & (TRAIL_HIST_SIZE - 1);

    int slot = 0;
    for (int i = 0; i < MAX_COMPANIONS; ++i) {
        Companion& c = gCompanions[i];
        if (!c.active) continue;

        // --- FOLLOW ---
        // Apply FORMATION_TIGHT: reduce spacing for each companion with that passive
        int tightBonus = 0;
        for (int j = 0; j < MAX_COMPANIONS; j++) {
            if (!gCompanions[j].active) continue;
            int jfci = companionFullClassId(gCompanions[j]);
            const PassiveDef& jpd = kPassiveDefs[jfci];
            if (jpd.type == PASSIVE_FORMATION_TIGHT) {
                u8 tierMult = (gCompanions[j].tier == 0) ? 1 : (gCompanions[j].tier == 1) ? 2 : 5;
                tightBonus += jpd.param1 * tierMult;
            }
        }
        int spacing = TRAIL_SPACING - tightBonus;
        if (spacing < 3) spacing = 3;

        // FORMATION_TIGHT regen: companions in tight formation heal 1HP/600f
        if (tightBonus > 0 && (regenTimer % 600 == 0) && c.hp < c.maxHp) {
            c.hp++;
        }

        int followOffset = (slot + 1) * spacing;
        int trailIdx = (gTrailHead - followOffset) & (TRAIL_HIST_SIZE - 1);
        Vec2 followTarget = gTrailHistory[trailIdx];

        // Lerp: pos += (followTarget - pos) / 4
        Vec2 diff = followTarget - c.pos;
        c.pos.x += static_cast<Fixed>(diff.x / 4);
        c.pos.y += static_cast<Fixed>(diff.y / 4);

        // Tick down Lifeshaper damage buff
        if (c.dmgBuffTimer > 0) c.dmgBuffTimer--;

        // --- SHOOT ---
        if (c.shootTimer > 0) {
            --c.shootTimer;
        } else {
            Enemy* target = findNearestEnemyFrom(c.pos);
            if (target) {
                int fci = companionFullClassId(c);
                const ClassDef& cd = kClassDefs[fci];

                // Tier-scaled damage (use int to avoid u8 overflow on high-damage classes)
                int baseDmg = cd.damage;
                int dmgInt = baseDmg * (c.tier == 0 ? 1 : c.tier == 1 ? 2 : 5);
                // Glass Cannon: +5 flat damage
                if (perkIsActive(PERK_GLASS_CANNON)) dmgInt += 5;
                // Lifeshaper buff: +20% damage
                if (c.dmgBuffTimer > 0) dmgInt = dmgInt * 12 / 10;
                // Last Stand: 3x damage when only 1 companion alive
                if (perkIsActive(PERK_LAST_STAND) && gCompanionCount == 1) dmgInt *= 3;
                // Cap at 255 for u8 bullet damage field
                u8 dmg = static_cast<u8>(dmgInt > 255 ? 255 : dmgInt);

                // Tier-scaled bullet count and pierce (universal, all classes)
                u8 bulletCount = cd.bulletCount + (c.tier >= 1 ? 1 : 0) + (c.tier >= 2 ? 1 : 0);
                u8 shotFlags   = cd.flags | (c.tier >= 2 ? BFLAG_PIERCE : 0);

                // Cooldown (Berserker special: scales with player HP)
                u8 cooldown = cd.cooldown;
                if (fci == 3) { // Berserker
                    int hpPct = (gPlayer.hp * 100) / gPlayer.maxHp;
                    cooldown = static_cast<u8>(cd.cooldown * hpPct / 100);
                    if (cooldown < 5) cooldown = 5;
                }
                // Overcharge: all companions fire 50% faster
                if (perkIsActive(PERK_OVERCHARGE)) {
                    cooldown = cooldown / 2;
                    if (cooldown < 3) cooldown = 3;
                }
                // Bloodlust: fire rate bonus from kills this wave
                {
                    int blPct = perkBloodlustCooldownPct();
                    if (blPct < 100) {
                        cooldown = static_cast<u8>(cooldown * blPct / 100);
                        if (cooldown < 3) cooldown = 3;
                    }
                }
                // Cyan T0 (Volt): companion fire rate +15%
                {
                    int cyanPct = synergyCyanCooldownPct();
                    if (cyanPct < 100) {
                        cooldown = static_cast<u8>(cooldown * cyanPct / 100);
                        if (cooldown < 3) cooldown = 3;
                    }
                }

                // Direction to nearest enemy
                Vec2 dir = target->pos - c.pos;
                Fixed adx = dir.x < 0 ? -dir.x : dir.x;
                Fixed ady = dir.y < 0 ? -dir.y : dir.y;
                Fixed denom = (adx > ady) ? adx : ady;
                if (denom == 0) denom = 1;
                Vec2 baseVel = {static_cast<Fixed>(fixDiv(dir.x, denom) * 3),
                                static_cast<Fixed>(fixDiv(dir.y, denom) * 3)};

                // Special class behaviors
                bool handled = false;
                switch (fci) {
                    case 6: { // B1 Warden — pulse wave: damage all enemies within range
                        // T1: 48px, T2: 64px + slow, T3: 96px + freeze
                        s32 pxRange = (c.tier == 0) ? 48 : (c.tier == 1) ? 64 : 96;
                        s32 range2 = static_cast<s32>(toFixed(pxRange)) * toFixed(pxRange);
                        for (auto& e : gEnemies) {
                            if (!e.active) continue;
                            Vec2 d = e.pos - c.pos;
                            s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                            if (dist < range2) {
                                e.hp -= dmg;
                                if (c.tier >= 2) { e.freezeTimer = 60; }
                                else if (c.tier >= 1) { if (e.slowTimer < 30) e.slowTimer = 30; }
                                if (e.hp <= 0) { e.active = false; spawnParticleBurst(e.pos, 8, 12, static_cast<u8>(c.color)); }
                            }
                        }
                        spawnParticleBurst(c.pos, 6, 8, static_cast<u8>(c.color));
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 14: { // G3 Apothecary — heal dart at lowest-HP companion; shoot enemies if all full
                        Companion* healTarget = nullptr;
                        for (int j = 0; j < MAX_COMPANIONS; j++) {
                            if (!gCompanions[j].active || &gCompanions[j] == &c) continue;
                            if (!healTarget || gCompanions[j].hp < healTarget->hp)
                                healTarget = &gCompanions[j];
                        }
                        if (healTarget && healTarget->hp < healTarget->maxHp) {
                            s16 heal = static_cast<s16>(1 * (c.tier == 0 ? 1 : c.tier == 1 ? 2 : 4));
                            healTarget->hp += heal;
                            if (healTarget->hp > healTarget->maxHp) healTarget->hp = healTarget->maxHp;
                            spawnParticleBurst(healTarget->pos, 4, 10, static_cast<u8>(c.color));
                            c.shootTimer = cooldown;
                            handled = true;
                        }
                        // If healTarget is null or all companions full HP, fall through to normal enemy shoot
                        break;
                    }
                    case 17: { // G6 Lifeshaper — pulse: heal all companions in 64px by 3HP, +20% dmg for 90f
                        s32 range2 = static_cast<s32>(toFixed(64)) * toFixed(64);
                        u8 healAmt = static_cast<u8>(3 * (c.tier == 0 ? 1 : c.tier == 1 ? 2 : 4));
                        u8 buffDur = 90;
                        for (int j = 0; j < MAX_COMPANIONS; j++) {
                            if (!gCompanions[j].active) continue;
                            Vec2 d = gCompanions[j].pos - c.pos;
                            s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                            if (dist < range2) {
                                gCompanions[j].hp += healAmt;
                                if (gCompanions[j].hp > gCompanions[j].maxHp)
                                    gCompanions[j].hp = gCompanions[j].maxHp;
                                gCompanions[j].dmgBuffTimer = buffDur;
                                spawnParticleBurst(gCompanions[j].pos, 5, 10, static_cast<u8>(c.color));
                            }
                        }
                        spawnParticleBurst(c.pos, 8, 12, static_cast<u8>(c.color));
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 18: { // Y1 Gambler — random damage: T1: 0-3x, T2: 0-4x, T3: 0-6x
                        u8 maxMult = (c.tier == 0) ? 3 : (c.tier == 1) ? 4 : 6;
                        u8 roll = static_cast<u8>(rngRange(maxMult + 1)); // 0..maxMult inclusive
                        u8 gambleDmg = static_cast<u8>(dmg * roll);
                        spawnCompanionBullet(c.pos, baseVel, c.color, gambleDmg,
                                            cd.flags, cd.aoeRadius, cd.effectDuration, 0, c.tier);
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 12: { // G1 Thornshot — spike wall: 5/7/9 thorns in wide perpendicular line
                        // T1: 5 thorns, T2: 7 thorns, T3: 9 thorns + poison
                        int thornCount = (c.tier == 0) ? 5 : (c.tier == 1) ? 7 : 9;
                        int halfN = thornCount / 2;
                        Fixed perpX = (baseVel.y > 0) ? toFixed(1) : (baseVel.y < 0) ? toFixed(-1) : 0;
                        Fixed perpY = (baseVel.x > 0) ? toFixed(-1) : (baseVel.x < 0) ? toFixed(1) : 0;
                        if (perpX == 0 && perpY == 0) perpX = toFixed(1);
                        u8 thornFlags = shotFlags | BFLAG_PIERCE | (c.tier >= 2 ? BFLAG_SLOW : 0);
                        for (int t = -halfN; t <= halfN; t++) {
                            Vec2 spos = c.pos;
                            spos.x += static_cast<Fixed>(perpX * t * 10);
                            spos.y += static_cast<Fixed>(perpY * t * 10);
                            Vec2 thornVel = {static_cast<Fixed>(baseVel.x / 3), static_cast<Fixed>(baseVel.y / 3)};
                            spawnCompanionBullet(spos, thornVel, c.color, dmg,
                                                thornFlags, 0, (c.tier >= 2 ? 90 : 0), 20, c.tier);
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 13: { // G2 Seedling — timed mine: stationary bullet, explodes after 90f
                        // T1: aoe=16, T2: aoe=32, T3: aoe=32 + chains (spawns 2 extra mines nearby)
                        u8 mineAoe = (c.tier == 0) ? 16 : 32;
                        Vec2 zero = {0, 0};
                        spawnCompanionBullet(c.pos, zero, c.color, static_cast<u8>(dmg * 2),
                                            shotFlags | BFLAG_EXPLODE, mineAoe, 0, 90, c.tier);
                        if (c.tier >= 2) {
                            // Chain mines: place 2 offset mines at 20px away
                            static const Vec2 kMineOffsets[2] = {{toFixed(20), 0}, {static_cast<Fixed>(-toFixed(20)), 0}};
                            for (int m = 0; m < 2; m++) {
                                Vec2 mpos = {c.pos.x + kMineOffsets[m].x, c.pos.y + kMineOffsets[m].y};
                                spawnCompanionBullet(mpos, zero, c.color, static_cast<u8>(dmg),
                                                    shotFlags | BFLAG_EXPLODE, mineAoe, 0, 120, c.tier);
                            }
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 15: { // G4 Vinecaller — 3 slow tendrils toward target
                        for (int t = -1; t <= 1; t++) {
                            Vec2 vel = baseVel;
                            vel.x = static_cast<Fixed>(vel.x * 2 / 3);
                            vel.y = static_cast<Fixed>(vel.y * 2 / 3);
                            vel.x += static_cast<Fixed>(t * 3);
                            spawnCompanionBullet(c.pos, vel, c.color, dmg,
                                                shotFlags | BFLAG_SLOW, 0, 45, 60, c.tier);
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 19: { // Y2 Scavenger — backward fan: 3 bullets away from target
                        Vec2 backVel = {static_cast<Fixed>(-baseVel.x), static_cast<Fixed>(-baseVel.y)};
                        for (int f = -1; f <= 1; f++) {
                            Vec2 vel = backVel;
                            vel.x += static_cast<Fixed>(f * 4);
                            vel.y += static_cast<Fixed>(f * 2);
                            spawnCompanionBullet(c.pos, vel, c.color, dmg, shotFlags, 0, 0, 0, c.tier);
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 20: { // Y3 Jester — ricochet pierce: bouncing bullet
                        spawnCompanionBullet(c.pos, baseVel, c.color, dmg,
                                            shotFlags | BFLAG_PIERCE, 0, 0, 90, c.tier);
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 21: { // Y4 Mimic — copy nearest ally's shot
                        int bestDist = 0x7FFFFFFF;
                        int bestIdx = -1;
                        for (int j = 0; j < MAX_COMPANIONS; j++) {
                            if (j == i || !gCompanions[j].active) continue;
                            Vec2 d = gCompanions[j].pos - c.pos;
                            s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                            if (dist < bestDist) { bestDist = dist; bestIdx = j; }
                        }
                        if (bestIdx >= 0) {
                            int allyFci = companionFullClassId(gCompanions[bestIdx]);
                            const ClassDef& acd = kClassDefs[allyFci];
                            spawnCompanionBullet(c.pos, baseVel, c.color, dmg,
                                                acd.flags | shotFlags, acd.aoeRadius, acd.effectDuration, 0, c.tier);
                        } else {
                            spawnCompanionBullet(c.pos, baseVel, c.color, dmg, shotFlags, 0, 0, 0, c.tier);
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 29: { // P6 Nightmare — fear pulse: push enemies away
                        // T1: 56px, T2: 72px, T3: 96px + damage-over-time while feared
                        s32 pxRange = (c.tier == 0) ? 56 : (c.tier == 1) ? 72 : 96;
                        s32 range2 = static_cast<s32>(toFixed(pxRange)) * toFixed(pxRange);
                        for (auto& e : gEnemies) {
                            if (!e.active) continue;
                            Vec2 d = e.pos - c.pos;
                            s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                            if (dist < range2) {
                                e.hp -= dmg;
                                if (dist > 0) {
                                    e.vel.x += static_cast<Fixed>(d.x / 4);
                                    e.vel.y += static_cast<Fixed>(d.y / 4);
                                }
                                e.fearTimer = static_cast<u8>(60 + c.tier * 30);
                                // T3: feared enemies also get slowed
                                if (c.tier >= 2) { if (e.slowTimer < e.fearTimer) e.slowTimer = e.fearTimer; }
                                if (e.hp <= 0) { e.active = false; spawnParticleBurst(e.pos, 8, 12, static_cast<u8>(c.color)); }
                            }
                        }
                        spawnParticleBurst(c.pos, 8, 10, static_cast<u8>(c.color));
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 30: { // C1 Drone Pilot — multi-target: T1: 3, T2: 4, T3: 5 + homing
                        int droneTargets = (c.tier == 0) ? 3 : (c.tier == 1) ? 4 : 5;
                        u8 droneFlags = shotFlags | (c.tier >= 2 ? BFLAG_PIERCE : 0);
                        Enemy* targets[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
                        s32 dists[5] = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
                        for (auto& e : gEnemies) {
                            if (!e.active) continue;
                            Vec2 d = e.pos - c.pos;
                            s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                            for (int t = 0; t < droneTargets; t++) {
                                if (dist < dists[t]) {
                                    for (int s = droneTargets - 1; s > t; s--) { targets[s] = targets[s-1]; dists[s] = dists[s-1]; }
                                    targets[t] = &e; dists[t] = dist;
                                    break;
                                }
                            }
                        }
                        for (int t = 0; t < droneTargets; t++) {
                            if (!targets[t]) break;
                            Vec2 tdir = targets[t]->pos - c.pos;
                            Fixed tadx = tdir.x < 0 ? -tdir.x : tdir.x;
                            Fixed tady = tdir.y < 0 ? -tdir.y : tdir.y;
                            Fixed tdenom = (tadx > tady) ? tadx : tady;
                            if (tdenom == 0) tdenom = 1;
                            Vec2 tvel = {static_cast<Fixed>(fixDiv(tdir.x, tdenom) * 3),
                                         static_cast<Fixed>(fixDiv(tdir.y, tdenom) * 3)};
                            spawnCompanionBullet(c.pos, tvel, c.color, dmg, droneFlags, 0, 0, 0, c.tier);
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 32: { // C3 Tesla Coil — chain lightning: T1: 2 chains, T2: 3, T3: 5
                        spawnCompanionBullet(c.pos, baseVel, c.color, dmg, shotFlags, 0, 0, 0, c.tier);
                        int maxChains = (c.tier == 0) ? 2 : (c.tier == 1) ? 3 : 5;
                        int chains = 0;
                        for (auto& e : gEnemies) {
                            if (!e.active || &e == target) continue;
                            Vec2 d = e.pos - target->pos;
                            s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                            if (dist < static_cast<s32>(toFixed(48)) * toFixed(48)) {
                                e.hp -= static_cast<s16>(dmg / 2);
                                spawnParticleBurst(e.pos, 3, 6, static_cast<u8>(c.color));
                                if (e.hp <= 0) { e.active = false; spawnParticleBurst(e.pos, 8, 12, static_cast<u8>(c.color)); }
                                if (++chains >= maxChains) break;
                            }
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 26: { // P3 Plague Doctor — lob a potion that creates a poison cloud zone
                        // T1: 16px / 180f, T2: 24px / 240f, T3: 32px / 240f + slow on tick
                        u8 cloudRadius = (c.tier == 0) ? 16 : (c.tier == 1) ? 24 : 32;
                        u8 cloudLife   = (c.tier == 0) ? 180 : 240;
                        spawnZone(target->pos, cloudRadius, dmg, 20, cloudLife,
                                  static_cast<u8>(c.color), ZONE_POISON);
                        // T3: immediately slow all enemies inside the fresh cloud
                        if (c.tier >= 2) {
                            int zx = target->pos.pixelX(), zy = target->pos.pixelY();
                            int r2 = cloudRadius * cloudRadius;
                            for (auto& e : gEnemies) {
                                if (!e.active) continue;
                                int dx = e.pos.pixelX() - zx, dy = e.pos.pixelY() - zy;
                                if (dx*dx + dy*dy <= r2 && e.slowTimer < 60)
                                    e.slowTimer = 60;
                            }
                        }
                        spawnParticleBurst(target->pos, 10, 16, static_cast<u8>(c.color));
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 23: { // Y6 Trickster — N-way burst: T1: 8, T2: 12, T3: 16 + pierce
                        // Each direction evenly divided around 360 degrees using integer ratios.
                        // We use a lookup table of (velX, velY) pairs scaled to spd.
                        // 8-way: cardinal+diagonal. 12-way: adds 30-deg offsets. 16-way: adds 22.5-deg.
                        // Approximated with integer fixed-point pairs (sum of axis components).
                        static const s8 kDirs8[8][2]  = {{3,0},{2,-2},{0,-3},{-2,-2},{-3,0},{-2,2},{0,3},{2,2}};
                        static const s8 kDirs12[12][2] = {{3,0},{3,-1},{2,-2},{1,-3},{0,-3},{-1,-3},{-2,-2},{-3,-1},{-3,0},{-3,1},{-2,2},{-1,3}};
                        static const s8 kDirs16[16][2] = {{3,0},{3,-1},{2,-2},{1,-3},{0,-3},{-1,-3},{-2,-2},{-3,-1},{-3,0},{-3,1},{-2,2},{-1,3},{0,3},{1,3},{2,2},{3,1}};
                        int ways = (c.tier == 0) ? 8 : (c.tier == 1) ? 12 : 16;
                        u8 burstFlags = (c.tier >= 2) ? (shotFlags | BFLAG_PIERCE) : shotFlags;
                        const s8 (*dirs)[2] = (ways == 8) ? kDirs8 : (ways == 12) ? kDirs12 : kDirs16;
                        for (int d = 0; d < ways; d++) {
                            Vec2 vel = {static_cast<Fixed>(toFixed(dirs[d][0])),
                                        static_cast<Fixed>(toFixed(dirs[d][1]))};
                            spawnCompanionBullet(c.pos, vel, c.color, dmg, burstFlags, 0, 0, 0, c.tier);
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 0: { // R1 Gunner — tracer rounds: every 5th shot does 3x damage
                        static u8 tracerCount = 0;
                        tracerCount++;
                        bool isTracer = (tracerCount % 5 == 0);
                        u8 tracerDmg = isTracer ? static_cast<u8>(dmgInt * 3 > 255 ? 255 : dmg * 3) : dmg;
                        spawnCompanionBullet(c.pos, baseVel, c.color, tracerDmg, shotFlags, 0, 0, 0, c.tier);
                        if (isTracer) {
                            spawnParticleBurst(c.pos, 3, 6, static_cast<u8>(c.color));
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 5: { // R6 Executioner — heavy execute bolt: slow piercing devastation
                        Vec2 slowVel = {static_cast<Fixed>(baseVel.x * 2 / 3), static_cast<Fixed>(baseVel.y * 2 / 3)};
                        u8 execFlags = shotFlags | BFLAG_PIERCE;
                        u8 execDmg = static_cast<u8>(dmgInt * 2 > 255 ? 255 : dmg * 2);
                        spawnCompanionBullet(c.pos, slowVel, c.color, execDmg, execFlags, 0, 0, 120, c.tier);
                        spawnParticle(c.pos, slowVel, 8, static_cast<u8>(c.color));
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 7: { // B2 Channeler — 3-round burst with slight spread
                        spawnCompanionBullet(c.pos, baseVel, c.color, dmg, shotFlags, 0, 0, 0, c.tier);
                        Vec2 v2 = {static_cast<Fixed>(baseVel.x + rngRange(8) - 4),
                                   static_cast<Fixed>(baseVel.y + rngRange(8) - 4)};
                        Vec2 v3 = {static_cast<Fixed>(baseVel.x + rngRange(8) - 4),
                                   static_cast<Fixed>(baseVel.y + rngRange(8) - 4)};
                        spawnCompanionBullet(c.pos, v2, c.color, dmg, shotFlags, 0, 0, 0, c.tier);
                        spawnCompanionBullet(c.pos, v3, c.color, dmg, shotFlags, 0, 0, 0, c.tier);
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 24: { // P1 Hexer — spreading curse: main slow bolt + 2 curse splinters
                        spawnCompanionBullet(c.pos, baseVel, c.color, dmg, shotFlags | BFLAG_SLOW, 0, 45, 0, c.tier);
                        for (int s = 0; s < 2; s++) {
                            Vec2 sv = {
                                static_cast<Fixed>(baseVel.x + (s ? baseVel.y/3 : -baseVel.y/3)),
                                static_cast<Fixed>(baseVel.y + (s ? -baseVel.x/3 : baseVel.x/3))
                            };
                            spawnCompanionBullet(c.pos, sv, c.color, static_cast<u8>(dmg/2), BFLAG_SLOW, 0, 30, 60, c.tier);
                        }
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 27: { // P4 Wraith — bidirectional ghost bolt: pierces forward and backward
                        spawnCompanionBullet(c.pos, baseVel, c.color, dmg, shotFlags | BFLAG_PIERCE, 0, 0, 90, c.tier);
                        Vec2 backVel = {static_cast<Fixed>(-baseVel.x * 2 / 3), static_cast<Fixed>(-baseVel.y * 2 / 3)};
                        spawnCompanionBullet(c.pos, backVel, c.color, static_cast<u8>(dmg * 3 / 4), BFLAG_PIERCE, 0, 0, 60, c.tier);
                        spawnParticle(c.pos, backVel, 10, 4);
                        c.shootTimer = cooldown;
                        handled = true;
                        break;
                    }
                    case 31: { // C2 Overclocker — overheat cycle: fires faster, then AoE burst
                        static u8 heatCounter = 0;
                        spawnCompanionBullet(c.pos, baseVel, c.color, dmg, shotFlags, 0, 0, 0, c.tier);
                        heatCounter++;
                        if (heatCounter >= 8) {
                            // OVERHEAT: AoE burst + forced cooldown
                            heatCounter = 0;
                            spawnParticleBurst(c.pos, 6, 10, 5);
                            for (auto& e : gEnemies) {
                                if (!e.active) continue;
                                Vec2 d = e.pos - c.pos;
                                s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                                if (dist < static_cast<s32>(toFixed(32)) * toFixed(32)) {
                                    e.hp -= dmg;
                                    e.hurtTimer = 4;
                                    if (e.hp <= 0) { e.active = false; spawnParticleBurst(e.pos, 6, 10, 5); }
                                }
                            }
                            c.shootTimer = 30; // forced overheat cooldown
                        } else {
                            // Each shot reduces next cooldown
                            c.shootTimer = static_cast<u8>(cooldown * (8 - heatCounter) / 8);
                            if (c.shootTimer < 3) c.shootTimer = 3;
                        }
                        handled = true;
                        break;
                    }
                    default: break;
                }

                if (!handled) {
                    // Data-driven bullet spawn
                    if (bulletCount == 1) {
                        spawnCompanionBullet(c.pos, baseVel, c.color, dmg,
                                            shotFlags, cd.aoeRadius, cd.effectDuration, 0, c.tier);
                    } else {
                        // Multi-bullet with spread
                        for (int n = 0; n < bulletCount; n++) {
                            // Simple spread: offset velocity
                            int offset = (n - bulletCount/2) * 4;
                            Vec2 vel = baseVel;
                            vel.x += static_cast<Fixed>(offset);
                            vel.y += static_cast<Fixed>(offset / 2);
                            spawnCompanionBullet(c.pos, vel, c.color, dmg,
                                                shotFlags, cd.aoeRadius, cd.effectDuration, 0, c.tier);
                        }
                    }

                    c.shootTimer = cooldown;
                }

                // Visual juice particles on shoot — per class
                switch (fci) {
                    case 2: // R3 Shotgunner: muzzle flash
                        spawnParticle(c.pos, baseVel, 4, static_cast<u8>(c.color));
                        break;
                    case 3: // R4 Berserker: rage sparks when player HP < 50%
                        if (gPlayer.hp < gPlayer.maxHp / 2)
                            spawnParticle(c.pos, {0, static_cast<Fixed>(-4)}, 8, 0);
                        break;
                    case 8: // B3 Frost Archer: frost particle trail
                        spawnParticle(c.pos, {static_cast<Fixed>(rngRange(6)-3), static_cast<Fixed>(rngRange(6)-3)}, 6, 1);
                        break;
                    case 9: { // B4 Orbiter: orbiting sparkle
                        Vec2 op = {static_cast<Fixed>(c.pos.x + toFixed(rngRange(8)-4)),
                                   static_cast<Fixed>(c.pos.y + toFixed(rngRange(8)-4))};
                        spawnParticle(op, {0, 0}, 3, 1);
                        break;
                    }
                    case 11: // B6 Nullifier: erase flash
                        spawnParticle(c.pos, baseVel, 6, 1);
                        break;
                    case 16: // G5 Sporewitch: green spore puff
                        for (int p = 0; p < 2; p++)
                            spawnParticle(c.pos, {static_cast<Fixed>(rngRange(10)-5), static_cast<Fixed>(rngRange(10)-5)}, 10, 2);
                        break;
                    case 22: // Y5 Alchemist: gold sparkle
                        spawnParticleBurst(c.pos, 3, 6, 3);
                        break;
                    case 25: // P2 Blightling: poison trail
                        spawnParticle(c.pos, baseVel, 8, 4);
                        break;
                    case 33: // C4 Signal Jammer: static burst
                        spawnParticle(c.pos, {0, 0}, 5, 5);
                        break;
                    case 34: // C5 Circuit Hacker: tech trail
                        spawnParticle(c.pos, baseVel, 8, 1);
                        break;
                    case 35: // C6 Mech Pilot: blast recoil
                        spawnParticleBurst(c.pos, 2, 4, 5);
                        break;
                    default: break;
                }
            }
        }

        ++slot;
    }
}

Companion* companionSpawn(PillColor color, u8 classId, u8 tier) {
    // Allow spawning up to MAX_COMPANIONS (array size) even if over soft cap
    // The shop checks merge eligibility before calling this
    if (gCompanionCount >= MAX_COMPANIONS) return nullptr;

    // Find first inactive slot
    for (auto& c : gCompanions) {
        if (c.active) continue;

        c.pos = gPlayer.pos;
        c.color = color;
        c.classId = classId;
        c.tier = tier;
        c.shootTimer = 0;
        c.iframes = 0;
        c.hitFlashTimer = 0;
        c.healFlashTimer = 0;
        c.dmgBuffTimer = 0;

        // Max HP by rarity: Common=6, Uncommon=10, Rare=16; doubles each tier step
        {
            int fci = static_cast<int>(color) * 6 + classId;
            static const s16 kBaseHpByRarity[3] = { 6, 10, 16 };
            s16 base = kBaseHpByRarity[kClassDefs[fci].rarity];
            s16 mult = (tier == 0) ? 1 : (tier == 1) ? 2 : 4;
            c.maxHp = static_cast<s16>(base * mult);
            if (perkIsActive(PERK_GLASS_CANNON)) {
                c.maxHp /= 2;
                if (c.maxHp < 1) c.maxHp = 1;
            }
        }
        c.hp = c.maxHp;  // newly bought companions start at full HP

        c.active = true;
        ++gCompanionCount;

        // PASSIVE_MAX_HP: boost player max HP when companion with this passive is acquired
        {
            int fci = static_cast<int>(color) * 6 + classId;
            const PassiveDef& pd = kPassiveDefs[fci];
            if (pd.type == PASSIVE_MAX_HP) {
                u8 tierMult = (tier == 0) ? 1 : (tier == 1) ? 2 : 5;
                s16 bonus = static_cast<s16>(pd.param1 * tierMult);
                gPlayer.maxHp += bonus;
                gPlayer.hp += bonus;
            }
        }

        synergyRecalc();  // update synergy tiers after roster change
        return &c;
    }

    return nullptr;
}

void companionRemove(int index) {
    if (index < 0 || index >= MAX_COMPANIONS) return;

    // PASSIVE_MAX_HP: remove player max HP bonus when companion is removed
    {
        const Companion& c = gCompanions[index];
        int fci = companionFullClassId(c);
        const PassiveDef& pd = kPassiveDefs[fci];
        if (pd.type == PASSIVE_MAX_HP) {
            u8 tierMult = (c.tier == 0) ? 1 : (c.tier == 1) ? 2 : 5;
            s16 bonus = static_cast<s16>(pd.param1 * tierMult);
            gPlayer.maxHp -= bonus;
            if (gPlayer.maxHp < 1) gPlayer.maxHp = 1;
            if (gPlayer.hp > gPlayer.maxHp) gPlayer.hp = gPlayer.maxHp;
            if (gPlayer.hp < 1) gPlayer.hp = 1;
        }
    }

    gCompanions[index].active = false;
    // Compact array
    int write = 0;
    for (int r = 0; r < MAX_COMPANIONS; ++r) {
        if (gCompanions[r].active) {
            if (r != write) gCompanions[write] = gCompanions[r];
            write++;
        }
    }
    for (int r = write; r < MAX_COMPANIONS; ++r) {
        gCompanions[r].active = false;
    }
    gCompanionCount = write;
    synergyRecalc();  // update synergy tiers after roster change
}

int companionCount() {
    return gCompanionCount;
}

bool companionCheckMerge() {
    // Scan for companions with same fullClassId and tier: T1->T2 needs 3, T2->T3 needs 2
    // Wildcard: match on classId only (ignore color)
    // Shortcut: T0->T1 also needs only 2
    bool wildcard = perkIsActive(PERK_WILDCARD);
    bool shortcut = perkIsActive(PERK_SHORTCUT);

    for (int i = 0; i < MAX_COMPANIONS; ++i) {
        if (!gCompanions[i].active) continue;
        int matchCount = 1;
        int matches[3] = {i, -1, -1};
        int fci = companionFullClassId(gCompanions[i]);
        u8 tier = gCompanions[i].tier;
        u8 classIdI = gCompanions[i].classId;

        for (int j = i + 1; j < MAX_COMPANIONS; ++j) {
            if (!gCompanions[j].active) continue;
            if (gCompanions[j].tier != tier) continue;
            bool match;
            if (wildcard) {
                // Wildcard: same classId regardless of color
                match = (gCompanions[j].classId == classIdI);
            } else {
                match = (companionFullClassId(gCompanions[j]) == fci);
            }
            if (match) {
                matches[matchCount] = j;
                matchCount++;
                if (matchCount >= 3) break;
            }
        }

        int required = (shortcut || tier > 0) ? 2 : 3;  // Shortcut: always 2; otherwise T0->T1 needs 3, T1->T2 needs 2
        if (matchCount >= required && tier < 2) {  // cap at T3 (tier 2)
            // Upgrade first match, remove the rest
            gCompanions[matches[0]].tier = tier + 1;
            int removed = 0;
            for (int m = 1; m < required; ++m) {
                gCompanions[matches[m]].active = false;
                removed++;
            }
            gCompanionCount -= removed;

            // Repack: shift active companions to fill gaps
            int write = 0;
            for (int r = 0; r < MAX_COMPANIONS; ++r) {
                if (gCompanions[r].active) {
                    if (r != write) gCompanions[write] = gCompanions[r];
                    write++;
                }
            }
            for (int r = write; r < MAX_COMPANIONS; ++r) {
                gCompanions[r].active = false;
            }
            gCompanionCount = write;

            synergyRecalc();  // tier might have changed after merge
            audioPlaySfx(GSFX_MERGE);
            renderTriggerMergeFlash();
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Visual event helpers
// ---------------------------------------------------------------------------

void companionOnDamage(int index, s16 dmg) {
    if (index < 0 || index >= MAX_COMPANIONS) return;
    Companion& c = gCompanions[index];
    if (!c.active || c.iframes > 0) return;
    c.hp -= dmg;
    c.hitFlashTimer = 3;
    c.iframes = 45;  // ~0.75 s at 60 fps
    if (c.hp <= 0) companionOnDeath(index);
}

void companionOnHeal(int index, s16 amount) {
    if (index < 0 || index >= MAX_COMPANIONS) return;
    Companion& c = gCompanions[index];
    if (!c.active) return;
    c.hp += amount;
    if (c.hp > c.maxHp) c.hp = c.maxHp;
    c.healFlashTimer = 12;  // 12 frames of rising green particles
}

void companionOnDeath(int index) {
    if (index < 0 || index >= MAX_COMPANIONS) return;
    Companion& c = gCompanions[index];
    if (!c.active) return;

    // Phoenix Down: revive first companion to die each wave
    if (perkIsActive(PERK_PHOENIX_DOWN) && !gPerks.phoenixUsedThisWave) {
        c.hp = c.maxHp / 2;
        c.iframes = 90;
        gPerks.phoenixUsedThisWave = true;
        audioPlaySfx(GSFX_HEAL);
        return; // don't actually die
    }

    // PASSIVE_MAX_HP: remove player max HP bonus on death
    {
        int fci = companionFullClassId(c);
        const PassiveDef& pd = kPassiveDefs[fci];
        if (pd.type == PASSIVE_MAX_HP) {
            u8 tierMult = (c.tier == 0) ? 1 : (c.tier == 1) ? 2 : 5;
            s16 bonus = static_cast<s16>(pd.param1 * tierMult);
            gPlayer.maxHp -= bonus;
            if (gPlayer.maxHp < 1) gPlayer.maxHp = 1;
            if (gPlayer.hp > gPlayer.maxHp) gPlayer.hp = gPlayer.maxHp;
            if (gPlayer.hp < 1) gPlayer.hp = 1;
        }
    }

    spawnParticleBurst(c.pos, 12, 20, static_cast<u8>(c.color));
    audioPlaySfx(GSFX_EXPLODE);
    c.active = false;
    int write = 0;
    for (int r = 0; r < MAX_COMPANIONS; ++r) {
        if (gCompanions[r].active) {
            if (r != write) gCompanions[write] = gCompanions[r];
            write++;
        }
    }
    for (int r = write; r < MAX_COMPANIONS; ++r) gCompanions[r].active = false;
    gCompanionCount = write;
}

void companionOnWaveHeal(int index) {
    if (index < 0 || index >= MAX_COMPANIONS) return;
    Companion& c = gCompanions[index];
    if (!c.active) return;
    spawnParticleBurst(c.pos, 5, 14, static_cast<u8>(PillColor::Green));
}
