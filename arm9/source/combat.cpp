#include "combat.h"
#include "player.h"
#include "entities.h"
#include "audio.h"
#include "perk.h"
#include "rng.h"
#include "companion.h"
#include "synergy.h"

// Update bullets: move, wall ricochet (Blue T2), remove out-of-arena
static void updateBullets() {
    bool ricochet = synergyBulletsRicochet();

    for (auto& b : gBullets) {
        if (!b.active) continue;
        b.pos += b.vel;
        int px = b.pos.pixelX();
        int py = b.pos.pixelY();

        // Blue T2: player bullets ricochet once off arena walls
        if (ricochet && b.color == 255 && b.lifetime == 0) {
            bool bounced = false;
            if (px <= WALL_THICK && b.vel.x < 0)      { b.vel.x = static_cast<Fixed>(-b.vel.x); bounced = true; }
            if (px >= ARENA_W - WALL_THICK && b.vel.x > 0) { b.vel.x = static_cast<Fixed>(-b.vel.x); bounced = true; }
            if (py <= WALL_THICK && b.vel.y < 0)       { b.vel.y = static_cast<Fixed>(-b.vel.y); bounced = true; }
            if (py >= ARENA_H - WALL_THICK && b.vel.y > 0)  { b.vel.y = static_cast<Fixed>(-b.vel.y); bounced = true; }
            if (bounced) {
                b.lifetime = 60; // after bounce, die after 60 frames (prevent infinite bounce)
                spawnParticle(b.pos, {0, 0}, 6, 1); // blue spark at bounce point
                continue; // skip OOB check this frame
            }
        }

        if (px < -8 || px > ARENA_W + 8 || py < -8 || py > ARENA_H + 8) {
            b.active = false;
            continue;
        }
        if (b.lifetime > 0) {
            b.lifetime--;
            if (b.lifetime == 0) b.active = false;
        }
    }
}

// Update enemies: per-type AI steering
static void updateEnemies() {
    for (auto& e : gEnemies) {
        if (!e.active) continue;
        e.frame++;

        // Status effects — check before any AI/movement
        if (e.freezeTimer > 0) { e.freezeTimer--; continue; }
        if (e.slowTimer > 0) e.slowTimer--;
        if (e.fearTimer > 0) e.fearTimer--;

        // Direction toward player (cheap normalize via max component)
        Vec2 toPlayer = gPlayer.pos - e.pos;
        Fixed adx = (toPlayer.x < 0) ? -toPlayer.x : toPlayer.x;
        Fixed ady = (toPlayer.y < 0) ? -toPlayer.y : toPlayer.y;
        Fixed denom = (adx > ady) ? adx : ady;
        if (denom == 0) denom = 1;
        Vec2 dir = {fixDiv(toPlayer.x, denom), fixDiv(toPlayer.y, denom)};

        // Fear override: fleeing enemies run away from player
        if (e.fearTimer > 0) {
            e.vel.x -= fixMul(dir.x, FP_ONE);
            e.vel.y -= fixMul(dir.y, FP_ONE);
            // Skip normal AI while feared
            goto applySpeedCap;
        }

        // Per-type speed and behavior
        // Types 0-11 = regular enemies, 12-15 = bosses
        switch (e.type) {
            case ETYPE_GRUNT: // Grunt (0) — slow march straight at player
                e.vel.x += fixMul(dir.x, FP_ONE / 4);
                e.vel.y += fixMul(dir.y, FP_ONE / 4);
                break;
            case ETYPE_CHARGER: { // Charger (1) — flanker, approaches from side
                Fixed side = ((e.frame / 60) & 1) ? toFixed(80) : toFixed(-80);
                Vec2 flankTarget = {
                    static_cast<Fixed>(gPlayer.pos.x + fixMul(-dir.y, side)),
                    static_cast<Fixed>(gPlayer.pos.y + fixMul(dir.x, side))
                };
                Vec2 toFlank = flankTarget - e.pos;
                Fixed fd = (toFlank.x < 0 ? -toFlank.x : toFlank.x);
                Fixed fdy = (toFlank.y < 0 ? -toFlank.y : toFlank.y);
                if (fdy > fd) fd = fdy;
                if (fd > 0) {
                    e.vel.x += fixDiv(toFlank.x, fd);
                    e.vel.y += fixDiv(toFlank.y, fd);
                }
                break;
            }
            case ETYPE_BRUTE: // Brute (2) — heavy tank, slow straight push
                e.vel.x += fixMul(dir.x, FP_ONE / 8);
                e.vel.y += fixMul(dir.y, FP_ONE / 8);
                break;
            case ETYPE_SPITTER: { // Spitter (3) — ranged kiter, keeps ~160px distance
                s32 distSq = static_cast<s32>(toPlayer.x) * toPlayer.x +
                             static_cast<s32>(toPlayer.y) * toPlayer.y;
                s32 idealDist = toFixed(160);
                s32 idealSq = static_cast<s32>(idealDist) * idealDist;
                if (distSq < idealSq) {
                    e.vel.x -= fixMul(dir.x, FP_ONE / 3);
                    e.vel.y -= fixMul(dir.y, FP_ONE / 3);
                } else {
                    e.vel.x += fixMul(dir.x, FP_ONE / 6);
                    e.vel.y += fixMul(dir.y, FP_ONE / 6);
                }
                Fixed strafe = ((e.frame >> 5) & 1) ? FP_ONE / 2 : -(FP_ONE / 2);
                e.vel.x += fixMul(-dir.y, strafe);
                e.vel.y += fixMul(dir.x, strafe);
                // Fire projectile every 90 frames
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 90;
                    Vec2 shotVel = {
                        static_cast<Fixed>(fixMul(dir.x, toFixed(2))),
                        static_cast<Fixed>(fixMul(dir.y, toFixed(2)))
                    };
                    Bullet* sb = spawnBullet(e.pos, shotVel, 3, 2);
                    if (sb) sb->lifetime = 90;
                }
                break;
            }
            case ETYPE_SNIPER: { // Sniper (4) — long-range, almost still, fires accurate shot
                // Maintain very long distance
                s32 distSq = static_cast<s32>(toPlayer.x) * toPlayer.x +
                             static_cast<s32>(toPlayer.y) * toPlayer.y;
                s32 idealDist = toFixed(220);
                s32 idealSq = static_cast<s32>(idealDist) * idealDist;
                if (distSq < idealSq) {
                    e.vel.x -= fixMul(dir.x, FP_ONE / 4);
                    e.vel.y -= fixMul(dir.y, FP_ONE / 4);
                }
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 120;
                    Vec2 shotVel = {
                        static_cast<Fixed>(fixMul(dir.x, toFixed(3))),
                        static_cast<Fixed>(fixMul(dir.y, toFixed(3)))
                    };
                    Bullet* sb = spawnBullet(e.pos, shotVel, 3, 4);
                    if (sb) { sb->lifetime = 80; sb->flags = BFLAG_PIERCE; }
                }
                break;
            }
            case ETYPE_SPLITTER: { // Splitter (5) — on death splits into 2 small Grunts (handled in killEnemy)
                e.vel.x += fixMul(dir.x, FP_ONE / 5);
                e.vel.y += fixMul(dir.y, FP_ONE / 5);
                break;
            }
            case ETYPE_SHIELD: { // Shield (6) — faces player; blocks frontal shots by backing away
                // Always try to approach from the side
                Fixed side = ((e.frame / 90) & 1) ? toFixed(40) : toFixed(-40);
                Vec2 sideTarget = {
                    static_cast<Fixed>(gPlayer.pos.x + fixMul(-dir.y, side)),
                    static_cast<Fixed>(gPlayer.pos.y + fixMul(dir.x, side))
                };
                Vec2 toSide = sideTarget - e.pos;
                Fixed sd = (toSide.x < 0 ? -toSide.x : toSide.x);
                Fixed sdy = (toSide.y < 0 ? -toSide.y : toSide.y);
                if (sdy > sd) sd = sdy;
                if (sd > 0) {
                    e.vel.x += fixDiv(toSide.x, sd) / 2;
                    e.vel.y += fixDiv(toSide.y, sd) / 2;
                }
                break;
            }
            case ETYPE_ARTILLERY: { // Artillery (7) — stationary, lobs explosive shot in an arc
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 150;
                    Vec2 shotVel = {
                        static_cast<Fixed>(fixMul(dir.x, toFixed(2))),
                        static_cast<Fixed>(fixMul(dir.y, toFixed(2)))
                    };
                    Bullet* sb = spawnBullet(e.pos, shotVel, 4, 5);
                    if (sb) { sb->flags = BFLAG_EXPLODE; sb->aoeRadius = 24; sb->lifetime = 70; }
                }
                // Barely moves
                e.vel.x = static_cast<Fixed>(e.vel.x * 12 / 16);
                e.vel.y = static_cast<Fixed>(e.vel.y * 12 / 16);
                break;
            }
            case ETYPE_NIGHTMARE: { // Nightmare (8) — fear pulse every 180 frames
                e.vel.x += fixMul(dir.x, FP_ONE / 6);
                e.vel.y += fixMul(dir.y, FP_ONE / 6);
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 180;
                    // Fear-pulse: set fearTimer on all nearby enemies (friendly fire)
                    for (auto& other : gEnemies) {
                        if (!other.active || &other == &e) continue;
                        Vec2 d = other.pos - e.pos;
                        s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                        if (dist < static_cast<s32>(toFixed(100)) * toFixed(100))
                            other.fearTimer = 90;
                    }
                    spawnParticleBurst(e.pos, 16, 20, 4); // purple burst
                }
                break;
            }
            case ETYPE_GHOST: { // Ghost (9) — teleports to random position near player every 120 frames
                if (e.shootTimer > 0) {
                    e.shootTimer--;
                    // Drift toward player while visible
                    e.vel.x += fixMul(dir.x, FP_ONE / 5);
                    e.vel.y += fixMul(dir.y, FP_ONE / 5);
                } else {
                    e.shootTimer = 120;
                    // Teleport to a random position ~80px from player
                    Fixed ox = toFixed((rngRange(160)) - 80);
                    Fixed oy = toFixed((rngRange(160)) - 80);
                    e.pos.x = static_cast<Fixed>(gPlayer.pos.x + ox);
                    e.pos.y = static_cast<Fixed>(gPlayer.pos.y + oy);
                    // Clamp into arena
                    if (e.pos.pixelX() < WALL_THICK + 8) e.pos.x = toFixed(WALL_THICK + 8);
                    if (e.pos.pixelX() > ARENA_W - WALL_THICK - 8) e.pos.x = toFixed(ARENA_W - WALL_THICK - 8);
                    if (e.pos.pixelY() < WALL_THICK + 8) e.pos.y = toFixed(WALL_THICK + 8);
                    if (e.pos.pixelY() > ARENA_H - WALL_THICK - 8) e.pos.y = toFixed(ARENA_H - WALL_THICK - 8);
                }
                break;
            }
            case ETYPE_SWARM_DRONE: { // Swarm Drone (10) — very fast, follows swarm leader
                // Simple fast rush at player
                e.vel.x += fixMul(dir.x, FP_ONE / 2);
                e.vel.y += fixMul(dir.y, FP_ONE / 2);
                break;
            }
            case ETYPE_BOMBER: { // Bomber (11) — charges in, explodes on player contact (handled in collision)
                e.vel.x += fixMul(dir.x, FP_ONE * 3 / 8);
                e.vel.y += fixMul(dir.y, FP_ONE * 3 / 8);
                break;
            }
            // Bosses (12-15): all charge + special abilities
            case ETYPE_BOSS_SENTINEL: { // Sentinel (12) — rotating turret: fires 4-way spread every 60f
                e.vel.x += fixMul(dir.x, FP_ONE / 4);
                e.vel.y += fixMul(dir.y, FP_ONE / 4);
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 60;
                    const Fixed spd = toFixed(2);
                    Bullet* b0 = spawnBullet(e.pos, {spd, 0}, 3, 6);
                    Bullet* b1 = spawnBullet(e.pos, {static_cast<Fixed>(-spd), 0}, 3, 6);
                    Bullet* b2 = spawnBullet(e.pos, {0, spd}, 3, 6);
                    Bullet* b3 = spawnBullet(e.pos, {0, static_cast<Fixed>(-spd)}, 3, 6);
                    if (b0) b0->lifetime = 60;
                    if (b1) b1->lifetime = 60;
                    if (b2) b2->lifetime = 60;
                    if (b3) b3->lifetime = 60;
                }
                break;
            }
            case ETYPE_BOSS_DREADNOUGHT: { // Dreadnought (13) — heavy charge + shockwave slam
                e.vel.x += fixMul(dir.x, FP_ONE / 3);
                e.vel.y += fixMul(dir.y, FP_ONE / 3);
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 90;
                    // Shockwave: 8-way burst
                    for (int a = 0; a < 8; a++) {
                        Fixed cos8[8] = { toFixed(1), toFixed(1)/2, 0, static_cast<Fixed>(-toFixed(1)/2),
                                         static_cast<Fixed>(-toFixed(1)), static_cast<Fixed>(-toFixed(1)/2), 0, toFixed(1)/2 };
                        Fixed sin8[8] = { 0, toFixed(1)/2, toFixed(1), toFixed(1)/2,
                                         0, static_cast<Fixed>(-toFixed(1)/2), static_cast<Fixed>(-toFixed(1)), static_cast<Fixed>(-toFixed(1)/2) };
                        Vec2 bv = {static_cast<Fixed>(cos8[a]*2), static_cast<Fixed>(sin8[a]*2)};
                        Bullet* sb = spawnBullet(e.pos, bv, 3, 4);
                        if (sb) sb->lifetime = 50;
                    }
                }
                break;
            }
            case ETYPE_BOSS_LEVIATHAN: { // Leviathan (14) — spawns Grunt adds every 120 frames
                e.vel.x += fixMul(dir.x, FP_ONE / 5);
                e.vel.y += fixMul(dir.y, FP_ONE / 5);
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 120;
                    // Summon 2 small Grunts near self
                    for (int s = 0; s < 2; s++) {
                        Fixed ox = toFixed((rngRange(48)) - 24);
                        Fixed oy = toFixed((rngRange(48)) - 24);
                        Vec2 spos = {static_cast<Fixed>(e.pos.x + ox), static_cast<Fixed>(e.pos.y + oy)};
                        spawnEnemy(spos, {0,0}, static_cast<s16>(e.maxHp / 8), ETYPE_GRUNT, SIZE_SMALL, 8);
                    }
                }
                break;
            }
            default: { // ETYPE_BOSS_NIGHTMARE (15) or unknown — circles player, fear pulses
                e.vel.x += fixMul(dir.x, FP_ONE / 6);
                e.vel.y += fixMul(dir.y, FP_ONE / 6);
                Fixed circle = ((e.frame >> 4) & 1) ? FP_ONE : static_cast<Fixed>(-FP_ONE);
                e.vel.x += fixMul(-dir.y, circle);
                e.vel.y += fixMul(dir.x, circle);
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 150;
                    spawnParticleBurst(e.pos, 20, 25, 4);
                }
                break;
            }
        }

        applySpeedCap:
        // Speed cap table indexed by type (types 12-15 share boss speed)
        static const Fixed maxSpeedTable[] = {
            toFixed(1),                           //  0 Grunt
            toFixed(2),                           //  1 Charger
            static_cast<Fixed>(FP_ONE * 3 / 4),  //  2 Brute
            static_cast<Fixed>(FP_ONE * 3 / 2),  //  3 Spitter
            toFixed(1),                           //  4 Sniper
            static_cast<Fixed>(FP_ONE * 5 / 4),  //  5 Splitter
            toFixed(1),                           //  6 Shield
            static_cast<Fixed>(FP_ONE / 2),      //  7 Artillery
            static_cast<Fixed>(FP_ONE * 7 / 8),  //  8 Nightmare
            toFixed(2),                           //  9 Ghost
            static_cast<Fixed>(FP_ONE * 5 / 2),  // 10 Swarm Drone
            static_cast<Fixed>(FP_ONE * 3 / 2),  // 11 Bomber
            static_cast<Fixed>(FP_ONE * 5 / 4),  // 12 Boss: Sentinel
            static_cast<Fixed>(FP_ONE * 3 / 2),  // 13 Boss: Dreadnought
            toFixed(1),                           // 14 Boss: Leviathan
            toFixed(1),                           // 15 Boss: Nightmare
        };
        int typeIdx = (e.type < 16) ? e.type : 15;
        Fixed maxSpd = maxSpeedTable[typeIdx];
        Fixed mag = (e.vel.x < 0 ? -e.vel.x : e.vel.x);
        Fixed magy = (e.vel.y < 0 ? -e.vel.y : e.vel.y);
        if (magy > mag) mag = magy;
        if (mag > maxSpd && mag > 0) {
            e.vel.x = fixDiv(fixMul(e.vel.x, maxSpd), mag);
            e.vel.y = fixDiv(fixMul(e.vel.y, maxSpd), mag);
        }

        // Wall avoidance steering (soft push away from walls)
        int px = e.pos.pixelX(), py = e.pos.pixelY();
        constexpr Fixed WALL_STEER = 6;
        if (px < WALL_THICK + 24) e.vel.x += WALL_STEER;
        if (px > ARENA_W - WALL_THICK - 24) e.vel.x -= WALL_STEER;
        if (py < WALL_THICK + 24) e.vel.y += WALL_STEER;
        if (py > ARENA_H - WALL_THICK - 24) e.vel.y -= WALL_STEER;

        // Slow: halve velocity this frame (status applied at top of loop)
        if (e.slowTimer > 0) {
            e.vel.x = static_cast<Fixed>(e.vel.x / 2);
            e.vel.y = static_cast<Fixed>(e.vel.y / 2);
        }

        // Blue T3 (Arcane): enemies permanently 25% slower
        {
            int spdPct = synergyEnemySpeedPct();
            if (spdPct < 100) {
                e.vel.x = static_cast<Fixed>(e.vel.x * spdPct / 100);
                e.vel.y = static_cast<Fixed>(e.vel.y * spdPct / 100);
            }
        }

        // Apply velocity
        e.pos += e.vel;

        // Hard clamp as backstop
        if (e.pos.pixelX() < WALL_THICK) e.pos.x = toFixed(WALL_THICK);
        if (e.pos.pixelX() > ARENA_W - WALL_THICK) e.pos.x = toFixed(ARENA_W - WALL_THICK);
        if (e.pos.pixelY() < WALL_THICK) e.pos.y = toFixed(WALL_THICK);
        if (e.pos.pixelY() > ARENA_H - WALL_THICK) e.pos.y = toFixed(ARENA_H - WALL_THICK);

        // Friction to prevent infinite acceleration
        e.vel.x = static_cast<Fixed>(e.vel.x * 14 / 16);
        e.vel.y = static_cast<Fixed>(e.vel.y * 14 / 16);
    }
}

// Update gold drops: countdown lifetime
static void updateGoldDrops() {
    for (auto& g : gGold) {
        if (!g.active) continue;
        if (g.lifetime > 0) {
            g.lifetime--;
        } else {
            g.active = false;
        }
    }
}

// Update particles: move and decay
static void updateParticles() {
    for (auto& p : gParticles) {
        if (!p.active) continue;
        p.pos += p.vel;
        if (p.life > 0) {
            p.life--;
        } else {
            p.active = false;
        }
    }
}

// Helper: kill or drop gold for an enemy that just reached 0 hp
static void killEnemy(Enemy& e, u8 bulletColor) {
    e.active = false;
    spawnParticleBurst(e.pos, 12, 15, bulletColor);
    audioPlaySfx(GSFX_EXPLODE);

    // Splitter on-death: spawn 2 small Grunts at same position (if not already small)
    if (e.type == ETYPE_SPLITTER && e.sizeClass > SIZE_SMALL) {
        for (int s = 0; s < 2; s++) {
            Fixed ox = toFixed((s == 0) ? -8 : 8);
            Vec2 spos = {static_cast<Fixed>(e.pos.x + ox), e.pos.y};
            spawnEnemy(spos, {0,0}, static_cast<s16>(e.maxHp / 3),
                       ETYPE_GRUNT, SIZE_SMALL, SPRITE_SIZE_SMALL);
        }
    }

    // Bomber on-death: AoE explosion
    if (e.type == ETYPE_BOMBER) {
        int bx = e.pos.pixelX(), by = e.pos.pixelY();
        for (auto& other : gEnemies) {
            if (!other.active) continue;
            int dx = other.pos.pixelX() - bx;
            int dy = other.pos.pixelY() - by;
            if (dx * dx + dy * dy <= 28 * 28) {
                other.hp -= e.maxHp / 2;
                if (other.hp <= 0) other.active = false;
            }
        }
        // Damage player if in blast radius
        int dpx = gPlayer.pos.pixelX() - bx;
        int dpy = gPlayer.pos.pixelY() - by;
        if (dpx * dpx + dpy * dpy <= 32 * 32) {
            s16 dmg = synergyOnPlayerHit(4);
            if (dmg > 0) { gPlayer.hp -= dmg; perkOnPlayerHit(); }
        }
        spawnParticleBurst(e.pos, 24, 18, 1); // large red burst
        audioPlaySfx(GSFX_EXPLODE);
    }

    u8 goldValue;
    switch (e.type) {
        case ETYPE_GRUNT:       goldValue = 1;  break;
        case ETYPE_CHARGER:     goldValue = 2;  break;
        case ETYPE_BRUTE:       goldValue = 3;  break;
        case ETYPE_SPITTER:     goldValue = 2;  break;
        case ETYPE_SNIPER:      goldValue = 3;  break;
        case ETYPE_SPLITTER:    goldValue = 2;  break;
        case ETYPE_SHIELD:      goldValue = 3;  break;
        case ETYPE_ARTILLERY:   goldValue = 4;  break;
        case ETYPE_NIGHTMARE:   goldValue = 3;  break;
        case ETYPE_GHOST:       goldValue = 3;  break;
        case ETYPE_SWARM_DRONE: goldValue = 1;  break;
        case ETYPE_BOMBER:      goldValue = 2;  break;
        default:                goldValue = 15; break; // bosses
    }
    // Gold Fever: 2x gold for duration
    if (gPerks.goldFeverWaves > 0) {
        goldValue = static_cast<u8>(goldValue * 2);
    }
    // Apply gold bonus from passives
    if (gPassive.goldBonusPct > 0) {
        goldValue = static_cast<u8>(goldValue + goldValue * gPassive.goldBonusPct / 100);
    }
    // Yellow T0 (Fortune): +20% gold drops
    {
        int goldPct = synergyGoldDropPct();
        if (goldPct != 100) goldValue = static_cast<u8>(goldValue * goldPct / 100);
    }
    spawnGold(e.pos, goldValue);

    // Synergy on-kill hook: handles Red kill-explosion, Green orb drop,
    // Purple poison zone, Cyan chain lightning arc visual.
    synergyOnEnemyKill(e.pos, bulletColor, e.maxHp);

    // PERK Chain Lightning (stacks on top of Cyan synergy chain)
    if (perkIsActive(PERK_CHAIN_LIGHTNING)) {
        for (auto& other : gEnemies) {
            if (!other.active) continue;
            Vec2 d = other.pos - e.pos;
            s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
            if (dist < static_cast<s32>(toFixed(64)) * toFixed(64)) {
                other.hp -= e.maxHp / 2;
                spawnParticleBurst(other.pos, 4, 8, 5); // cyan particles
                synergySpawnChainArc(e.pos, other.pos); // arc visual
                if (other.hp <= 0) { other.active = false; }
                break; // only zap one
            }
        }
    }
}

// Helper: apply AoE damage from a bullet that just exploded
static void applyAoe(const Bullet& b) {
    int bx = b.pos.pixelX(), by = b.pos.pixelY();
    int r  = b.aoeRadius;
    for (auto& e : gEnemies) {
        if (!e.active) continue;
        int dx = e.pos.pixelX() - bx;
        int dy = e.pos.pixelY() - by;
        if (dx * dx + dy * dy <= r * r) {
            e.hp -= b.damage;
            spawnParticleBurst(e.pos, 4, 8, b.color);
            if (e.hp <= 0) killEnemy(e, b.color);
        }
    }
    // Add visible explosion at bullet impact point
    spawnParticleBurst({toFixed(bx), toFixed(by)}, 8, 12, b.color);
}

// Check bullet-enemy collisions
static void checkBulletEnemyCollisions() {
    for (auto& b : gBullets) {
        if (!b.active) continue;

        // Lifetime countdown (0 = unlimited)
        if (b.lifetime > 0) {
            // lifetime is decremented in updateBullets; treat reaching 0 as expiry
            // (field is decremented there — nothing extra needed here)
        }

        // BFLAG_ERASE: deactivate enemy bullets that overlap this bullet
        if (b.flags & BFLAG_ERASE) {
            Rect br = {
                static_cast<s16>(b.pos.pixelX() - 4),
                static_cast<s16>(b.pos.pixelY() - 4),
                8, 8
            };
            for (auto& ob : gBullets) {
                if (&ob == &b || !ob.active) continue;
                // Enemy bullets have no flags; player bullets always have a shooter
                // identity. Simplest heuristic: erase bullets with color != b.color.
                if (ob.color == b.color) continue;
                Rect obr = {
                    static_cast<s16>(ob.pos.pixelX() - 2),
                    static_cast<s16>(ob.pos.pixelY() - 2),
                    4, 4
                };
                if (br.overlaps(obr)) ob.active = false;
            }
        }

        Rect br = {
            static_cast<s16>(b.pos.pixelX() - 2),
            static_cast<s16>(b.pos.pixelY() - 2),
            4, 4
        };

        for (auto& e : gEnemies) {
            if (!e.active) continue;
            int half = e.spriteSize / 2;
            Rect er = {
                static_cast<s16>(e.pos.pixelX() - half),
                static_cast<s16>(e.pos.pixelY() - half),
                static_cast<s16>(e.spriteSize),
                static_cast<s16>(e.spriteSize)
            };

            if (!br.overlaps(er)) continue;

            // --- apply status effects before damage ---
            if ((b.flags & BFLAG_FREEZE) && b.effectDuration > 0)
                e.freezeTimer = b.effectDuration;
            else if ((b.flags & BFLAG_SLOW) && b.effectDuration > 0)
                e.slowTimer = b.effectDuration;

            // Purple T0 (Hex): all bullet hits slow enemies 30f
            if (synergyHitsSlow() && e.slowTimer < 30)
                e.slowTimer = 30;

            // --- apply damage (or AoE / zone) ---
            if (b.flags & BFLAG_ZONE) {
                // Spawn a poison cloud at impact point; radius reuses aoeRadius
                u8 zoneRadius = b.aoeRadius > 0 ? b.aoeRadius : 16;
                spawnZone(b.pos, zoneRadius, b.damage, 15, 120, b.color, ZONE_POISON);
                spawnParticleBurst(b.pos, 8, 14, b.color);
                b.active = false;
                break;
            }
            if (b.flags & BFLAG_EXPLODE) {
                applyAoe(b);
                b.active = false;
                break; // bullet is gone; stop checking enemies
            }

            // Purple T3 (Hex): enemies take +25% damage
            u8 dmg = b.damage;
            {
                int mult = synergyEnemyDamageTakenPct();
                if (mult != 100) dmg = static_cast<u8>(dmg * mult / 100);
            }

            e.hp -= dmg;
            spawnParticleBurst(e.pos, 4, 8, b.color);

            // Cyan T3 (Volt): every bullet hit chains to 1 nearby enemy
            if (synergyCyanChainOnHit() && e.active) {
                Enemy* chainTarget = nullptr;
                s32 bestDist = static_cast<s32>(toFixed(48)) * toFixed(48);
                for (auto& other : gEnemies) {
                    if (!other.active || &other == &e) continue;
                    Vec2 d = other.pos - e.pos;
                    s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                    if (dist < bestDist) { bestDist = dist; chainTarget = &other; }
                }
                if (chainTarget) {
                    chainTarget->hp -= dmg / 2;
                    synergySpawnChainArc(e.pos, chainTarget->pos);
                    spawnParticleBurst(chainTarget->pos, 3, 6, 5);
                    if (chainTarget->hp <= 0) killEnemy(*chainTarget, b.color);
                }
            }

            if (e.hp <= 0) killEnemy(e, b.color);

            // Pierce: keep bullet alive if BFLAG_PIERCE or Red T2 synergy
            bool shouldPierce = (b.flags & BFLAG_PIERCE) || synergyBulletsPierce();
            if (!shouldPierce) {
                b.active = false;
                break;
            }
        }
    }
}

// Pickup heal orbs (Green3 synergy)
static void checkPlayerHealOrbCollisions() {
    Rect pr = playerRect();
    for (auto& h : gHealOrbs) {
        if (!h.active) continue;
        // Despawn countdown
        if (h.lifetime > 0) h.lifetime--;
        else { h.active = false; continue; }

        Rect hr = {
            static_cast<s16>(h.pos.pixelX() - 5),
            static_cast<s16>(h.pos.pixelY() - 5),
            10, 10
        };
        if (pr.overlaps(hr)) {
            gPlayer.hp += h.healAmount;
            if (gPlayer.hp > gPlayer.maxHp) gPlayer.hp = gPlayer.maxHp;
            h.active = false;
            audioPlaySfx(GSFX_HEAL);
        }
    }
}

// Check player-gold collisions
static void checkPlayerGoldCollisions() {
    Rect pr = playerRect();

    for (auto& g : gGold) {
        if (!g.active) continue;
        int bonus = gPassive.pickupRangeBonus;
        Rect gr = {
            static_cast<s16>(g.pos.pixelX() - 4 - bonus),
            static_cast<s16>(g.pos.pixelY() - 4 - bonus),
            static_cast<s16>(8 + bonus * 2),
            static_cast<s16>(8 + bonus * 2)
        };

        if (pr.overlaps(gr)) {
            gPlayer.gold += g.value;
            g.active = false;
            audioPlaySfx(GSFX_GOLD);
        }
    }
}

// Check enemy-companion collisions
static void checkEnemyCompanionCollisions() {
    for (auto& e : gEnemies) {
        if (!e.active) continue;
        int half = e.spriteSize / 2;
        Rect er = {
            static_cast<s16>(e.pos.pixelX() - half),
            static_cast<s16>(e.pos.pixelY() - half),
            static_cast<s16>(e.spriteSize),
            static_cast<s16>(e.spriteSize)
        };

        for (int i = 0; i < MAX_COMPANIONS; i++) {
            Companion& c = gCompanions[i];
            if (!c.active) continue;
            if (c.iframes > 0) { c.iframes--; continue; }

            Rect cr = {
                static_cast<s16>(c.pos.pixelX() - 6),
                static_cast<s16>(c.pos.pixelY() - 6),
                12, 12
            };

            if (!er.overlaps(cr)) continue;

            c.hp -= 3;
            c.iframes = 60;
            spawnParticleBurst(c.pos, 6, 8, 3);
            audioPlaySfx(GSFX_HIT);

            if (c.hp <= 0) {
                spawnParticleBurst(c.pos, 12, 15, static_cast<u8>(c.color));
                companionRemove(i);
            }
            // Enemy survives contact with a companion
        }
    }
}

// Check enemy-player collisions
static void checkEnemyPlayerCollisions() {
    Rect pr = playerRect();

    for (auto& e : gEnemies) {
        if (!e.active) continue;
        int half = e.spriteSize / 2;
        Rect er = {
            static_cast<s16>(e.pos.pixelX() - half),
            static_cast<s16>(e.pos.pixelY() - half),
            static_cast<s16>(e.spriteSize),
            static_cast<s16>(e.spriteSize)
        };

        if (pr.overlaps(er)) {
            if (gPlayer.dashInvincible) continue; // BUG-7: dash should not kill enemies
            // Route damage through synergy (shield absorption, damage reduction)
            u8 rawDmg = (e.type == ETYPE_BOMBER) ? 6 : 3;
            s16 dmg = synergyOnPlayerHit(rawDmg);
            if (dmg > 0) {
                gPlayer.hp -= dmg;
                perkOnPlayerHit();
                spawnParticleBurst(gPlayer.pos, 8, 10, 3);
            }
            audioPlaySfx(GSFX_HIT);
            // Bombers trigger their AoE explosion on contact via killEnemy
            killEnemy(e, 0);
        }
    }
}


// Update damage zones: tick lifetime and deal periodic damage to enemies inside
static void updateZones() {
    for (auto& z : gZones) {
        if (!z.active) continue;

        // Lifetime countdown
        if (z.lifetime > 0) z.lifetime--;
        else { z.active = false; continue; }

        // Ambient cloud particles every 8 frames
        if (z.lifetime % 8 == 0) {
            for (int p = 0; p < 3; p++) {
                Vec2 ppos = z.pos;
                ppos.x = static_cast<Fixed>(ppos.x + toFixed(rngRange(z.radius * 2) - z.radius));
                ppos.y = static_cast<Fixed>(ppos.y + toFixed(rngRange(z.radius * 2) - z.radius));
                Vec2 pvel = {0, static_cast<Fixed>(-FP_ONE / 2)};
                spawnParticle(ppos, pvel, 25, z.color);
            }
        }

        // Damage tick countdown
        if (z.tickTimer > 0) { z.tickTimer--; continue; }
        z.tickTimer = z.tickInterval;

        // Apply damage to every enemy inside the radius
        int zx = z.pos.pixelX(), zy = z.pos.pixelY();
        for (auto& e : gEnemies) {
            if (!e.active) continue;
            int dx = e.pos.pixelX() - zx;
            int dy = e.pos.pixelY() - zy;
            if (dx * dx + dy * dy < z.radius * z.radius) {
                e.hp -= z.damage;
                spawnParticle(e.pos, {0, 0}, 10, z.color);
                if (e.hp <= 0) killEnemy(e, z.color);
            }
        }
    }
}

void combatUpdate() {
    synergyUpdate();          // per-frame timers (EMP, heal tick, shield regen, arcs)
    updateBullets();
    updateEnemies();
    updateGoldDrops();
    updateParticles();
    updateZones();
    checkBulletEnemyCollisions();
    checkPlayerGoldCollisions();
    checkPlayerHealOrbCollisions();
    checkEnemyPlayerCollisions();
    checkEnemyCompanionCollisions();
}
