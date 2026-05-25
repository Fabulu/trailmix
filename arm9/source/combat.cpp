#include "combat.h"
#include "player.h"
#include "entities.h"
#include "camera.h"
#include "audio.h"
#include "perk.h"
#include "rng.h"
#include "companion.h"
#include "synergy.h"
#include "game.h"

// Update bullets: move, wall ricochet (Blue T2 / Ricochet perk), remove out-of-arena
static void updateBullets() {
    bool ricochet = synergyBulletsRicochet() || perkIsActive(PERK_RICOCHET);

    for (auto& b : gBullets) {
        if (!b.active) continue;
        b.pos += b.vel;
        int px = b.pos.pixelX();
        int py = b.pos.pixelY();

        // Blue T2 / Ricochet perk: player bullets ricochet once off arena walls
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
        // Types 0-16 = regular enemies, 17+ = bosses
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
                    Bullet* sb = spawnBullet(e.pos, shotVel, BULLET_COLOR_ENEMY, 2);
                    if (sb) { sb->lifetime = 90; sb->visualType = BVIS_ENEMY; }
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
                    Bullet* sb = spawnBullet(e.pos, shotVel, BULLET_COLOR_ENEMY, 4);
                    if (sb) { sb->lifetime = 80; sb->flags = BFLAG_PIERCE; sb->visualType = BVIS_ENEMY; }
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
                    Bullet* sb = spawnBullet(e.pos, shotVel, BULLET_COLOR_ENEMY, 5);
                    if (sb) { sb->flags = BFLAG_EXPLODE; sb->aoeRadius = 24; sb->lifetime = 70; sb->visualType = BVIS_ENEMY; }
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
                    spawnParticleBurst(e.pos, 6, 12, 4); // purple burst
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
            case ETYPE_MEDIC: { // Medic (12) — flees player, heals nearby damaged allies
                // Flee: accelerate AWAY from player
                e.vel.x -= fixMul(dir.x, FP_ONE / 4);
                e.vel.y -= fixMul(dir.y, FP_ONE / 4);
                // Every 90 frames: heal nearest damaged ally within 48px
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 90;
                    Enemy* healTarget = nullptr;
                    s32 bestDist = static_cast<s32>(toFixed(48)) * toFixed(48);
                    for (auto& other : gEnemies) {
                        if (!other.active || &other == &e) continue;
                        if (other.hp >= other.maxHp) continue; // not damaged
                        Vec2 d = other.pos - e.pos;
                        s32 dist = static_cast<s32>(d.x) * d.x + static_cast<s32>(d.y) * d.y;
                        if (dist < bestDist) { bestDist = dist; healTarget = &other; }
                    }
                    if (healTarget) {
                        s16 heal = static_cast<s16>(healTarget->maxHp * 15 / 100);
                        if (heal < 1) heal = 1;
                        healTarget->hp += heal;
                        if (healTarget->hp > healTarget->maxHp) healTarget->hp = healTarget->maxHp;
                        spawnParticleBurst(healTarget->pos, 4, 8, 2); // green heal particles
                    }
                }
                break;
            }
            case ETYPE_ANCHOR: { // Anchor (13) — slow approach, damage-resist aura (checked in collision code)
                e.vel.x += fixMul(dir.x, FP_ONE / 6);
                e.vel.y += fixMul(dir.y, FP_ONE / 6);
                break;
            }
            case ETYPE_TRAPPER: { // Trapper (14) — maintains ~120px distance, plants slow zones
                s32 distSq = static_cast<s32>(toPlayer.x) * toPlayer.x +
                             static_cast<s32>(toPlayer.y) * toPlayer.y;
                s32 idealDist = toFixed(120);
                s32 idealSq = static_cast<s32>(idealDist) * idealDist;
                if (distSq < idealSq) {
                    e.vel.x -= fixMul(dir.x, FP_ONE / 3);
                    e.vel.y -= fixMul(dir.y, FP_ONE / 3);
                } else {
                    e.vel.x += fixMul(dir.x, FP_ONE / 6);
                    e.vel.y += fixMul(dir.y, FP_ONE / 6);
                }
                // Every 120 frames: spawn slow zone at player's current position (max 3 via aiState counter)
                if (e.shootTimer > 0) { e.shootTimer--; }
                else if (e.aiState < 3) {
                    e.shootTimer = 120;
                    e.aiState++;
                    // Spawn a zone at the player's current position
                    spawnZone(gPlayer.pos, 16, 1, 15, 180, 4, ZONE_POISON);
                    audioPlaySfx(GSFX_MINE_PLACE);
                }
                break;
            }
            case ETYPE_HEXER: { // Hexer (15) — maintains ~180px distance, fires slow projectile
                s32 distSq = static_cast<s32>(toPlayer.x) * toPlayer.x +
                             static_cast<s32>(toPlayer.y) * toPlayer.y;
                s32 idealDist = toFixed(180);
                s32 idealSq = static_cast<s32>(idealDist) * idealDist;
                if (distSq < idealSq) {
                    e.vel.x -= fixMul(dir.x, FP_ONE / 4);
                    e.vel.y -= fixMul(dir.y, FP_ONE / 4);
                } else {
                    e.vel.x += fixMul(dir.x, FP_ONE / 6);
                    e.vel.y += fixMul(dir.y, FP_ONE / 6);
                }
                // Every 150 frames: fire a slow projectile (speed 1, BFLAG_SLOW, effectDuration=120)
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 150;
                    Vec2 shotVel = {
                        static_cast<Fixed>(fixMul(dir.x, FP_ONE)),
                        static_cast<Fixed>(fixMul(dir.y, FP_ONE))
                    };
                    Bullet* sb = spawnBullet(e.pos, shotVel, BULLET_COLOR_ENEMY, 2, BFLAG_SLOW, 0, 120, 180);
                    if (sb) sb->visualType = BVIS_ENEMY;
                    (void)sb;
                    audioPlaySfx(GSFX_ENEMY_SHOOT);
                }
                break;
            }
            case ETYPE_HIVE: { // Hive (16) — stationary, spawns drones periodically
                // Does NOT move: apply heavy friction to stay still
                e.vel.x = static_cast<Fixed>(e.vel.x * 4 / 16);
                e.vel.y = static_cast<Fixed>(e.vel.y * 4 / 16);
                // Every 180 frames: spawn 1 Swarm Drone if fewer than 4 alive
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 180;
                    // Count active drones
                    int droneCount = 0;
                    for (auto& other : gEnemies) {
                        if (other.active && other.type == ETYPE_SWARM_DRONE) droneCount++;
                    }
                    if (droneCount < 4) {
                        Fixed ox = toFixed(rngRange(16) - 8);
                        Fixed oy = toFixed(rngRange(16) - 8);
                        Vec2 spos = {static_cast<Fixed>(e.pos.x + ox), static_cast<Fixed>(e.pos.y + oy)};
                        spawnEnemy(spos, {0, 0}, static_cast<s16>(e.maxHp / 4), ETYPE_SWARM_DRONE, SIZE_SMALL, SPRITE_SIZE_SMALL);
                    }
                }
                break;
            }
            // ============ BOSSES — real behavior, not just charge+shoot ============

            case ETYPE_BOSS_SENTINEL: { // Sentinel — orbiting turret with rotating bullet spiral
                e.frame++;
                // First 60 frames: charge toward player to get into range
                if (e.frame < 60) {
                    e.vel.x += fixMul(dir.x, FP_ONE / 3);
                    e.vel.y += fixMul(dir.y, FP_ONE / 3);
                } else {
                    // Orbit player slowly, reverse direction every 4 seconds
                    bool clockwise = ((e.frame / 240) & 1);
                    // Slower orbit — hittable but still mobile
                    if (clockwise) {
                        e.vel.x += fixMul(-dir.y, FP_ONE / 8) + fixMul(dir.x, FP_ONE / 6);
                        e.vel.y += fixMul(dir.x, FP_ONE / 8) + fixMul(dir.y, FP_ONE / 6);
                    } else {
                        e.vel.x += fixMul(dir.y, FP_ONE / 8) + fixMul(dir.x, FP_ONE / 6);
                        e.vel.y += fixMul(-dir.x, FP_ONE / 8) + fixMul(dir.y, FP_ONE / 6);
                    }
                    // Add friction so it doesn't accelerate forever
                    e.vel.x = static_cast<Fixed>(e.vel.x * 14 / 16);
                    e.vel.y = static_cast<Fixed>(e.vel.y * 14 / 16);
                }

                // Constant slow rotation fire — 1 bullet every 20 frames in a spiral
                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    e.shootTimer = 20;
                    // Rotating angle based on frame counter
                    static const Fixed cos8[] = { 16,11,0,-11,-16,-11,0,11 };
                    static const Fixed sin8[] = { 0,-11,-16,-11,0,11,16,11 };
                    int angle = (e.frame / 20) & 7;
                    Vec2 bv = {static_cast<Fixed>(cos8[angle] * 2), static_cast<Fixed>(sin8[angle] * 2)};
                    Bullet* b = spawnBullet(e.pos, bv, BULLET_COLOR_ENEMY, 4);
                    if (b) { b->lifetime = 80; b->visualType = BVIS_ENEMY; }
                    // Every 3rd shot also fires aimed at player
                    if ((e.aiState & 3) == 0) {
                        Fixed spd = toFixed(2);
                        Bullet* ab = spawnBullet(e.pos, {static_cast<Fixed>(fixMul(dir.x, spd)),
                                            static_cast<Fixed>(fixMul(dir.y, spd))}, BULLET_COLOR_ENEMY, 5);
                        if (ab) ab->visualType = BVIS_ENEMY;
                    }
                    e.aiState++;
                    // Telegraph particle on the bullet origin
                    spawnParticle(e.pos, bv, 8, 5);
                }
                break;
            }

            case ETYPE_BOSS_DREADNOUGHT: { // Dreadnought — 3-state: stalk, telegraph, SLAM
                e.frame++;
                if (e.shootTimer > 0) { e.shootTimer--; }
                // First spawn: start with a stalk timer so it approaches first
                if (e.frame == 1 && e.shootTimer == 0) e.shootTimer = 90;

                if (e.aiState == 0) {
                    // STALK: menacing approach toward player
                    e.vel.x += fixMul(dir.x, FP_ONE / 3);
                    e.vel.y += fixMul(dir.y, FP_ONE / 3);
                    // Fire aimed shot every 60 frames while stalking
                    if ((e.frame % 60) == 0) {
                        Fixed spd = static_cast<Fixed>(FP_ONE * 3 / 2);
                        Bullet* db = spawnBullet(e.pos, {static_cast<Fixed>(fixMul(dir.x, spd)),
                                            static_cast<Fixed>(fixMul(dir.y, spd))}, BULLET_COLOR_ENEMY, 4);
                        if (db) db->visualType = BVIS_ENEMY;
                    }
                    // After stalking for 120 frames, start telegraph
                    if (e.shootTimer == 0) {
                        e.shootTimer = 40; // telegraph duration
                        e.aiState = 1;
                        // Telegraph: stop moving, pulse particles inward
                        e.vel = {0, 0};
                    }
                } else if (e.aiState == 1) {
                    // TELEGRAPH: frozen, sucking particles inward (visual warning)
                    e.vel = {0, 0};
                    if ((e.frame & 3) == 0) {
                        Vec2 pv = {static_cast<Fixed>(rngRange(20) - 10), static_cast<Fixed>(rngRange(20) - 10)};
                        spawnParticle(e.pos, pv, 6, 1);
                    }
                    if (e.shootTimer == 0) {
                        // SLAM! Dash to player position + 8-way explosive burst
                        // Use frame as dash flag — slam velocity applied AFTER speed cap
                        e.vel.x = fixMul(dir.x, toFixed(5));
                        e.vel.y = fixMul(dir.y, toFixed(5));
                        static const Fixed cos8[] = { 16,11,0,-11,-16,-11,0,11 };
                        static const Fixed sin8[] = { 0,-11,-16,-11,0,11,16,11 };
                        for (int a = 0; a < 8; a++) {
                            Vec2 bv = {static_cast<Fixed>(cos8[a]*2), static_cast<Fixed>(sin8[a]*2)};
                            Bullet* sb = spawnBullet(e.pos, bv, BULLET_COLOR_ENEMY, 5, BFLAG_EXPLODE, 16);
                            if (sb) { sb->lifetime = 40; sb->visualType = BVIS_ENEMY; }
                        }
                        cameraShake(8, 15);
                        spawnParticleBurst(e.pos, 8, 12, 1);
                        e.shootTimer = 50; // recovery
                        e.aiState = 2;
                    }
                } else {
                    // RECOVER: skidding to a stop after slam
                    e.vel.x = static_cast<Fixed>(e.vel.x * 12 / 16);
                    e.vel.y = static_cast<Fixed>(e.vel.y * 12 / 16);
                    if (e.shootTimer == 0) {
                        e.aiState = 0;
                        e.shootTimer = 120; // stalk duration before next cycle
                    }
                }
                break;
            }

            case ETYPE_BOSS_LEVIATHAN: { // Leviathan — circles slowly, summons varied adds, spits
                e.frame++;
                // Slow orbit around player
                e.vel.x += fixMul(-dir.y, FP_ONE / 12) + fixMul(dir.x, FP_ONE / 10);
                e.vel.y += fixMul(dir.x, FP_ONE / 12) + fixMul(dir.y, FP_ONE / 10);

                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    if (e.aiState == 0) {
                        // Summon 2 mixed adds
                        e.shootTimer = 100;
                        static const u8 addTypes[] = { ETYPE_GRUNT, ETYPE_CHARGER, ETYPE_SPITTER, ETYPE_BOMBER };
                        for (int s = 0; s < 2; s++) {
                            u8 addType = addTypes[rngRange(4)];
                            Fixed ox = toFixed(rngRange(40) - 20);
                            Fixed oy = toFixed(rngRange(40) - 20);
                            Vec2 spos = {static_cast<Fixed>(e.pos.x + ox), static_cast<Fixed>(e.pos.y + oy)};
                            spawnEnemy(spos, {0,0}, static_cast<s16>(e.maxHp / 10), addType, SIZE_SMALL, 8);
                        }
                        spawnParticleBurst(e.pos, 6, 10, 2);
                        e.aiState = 1;
                    } else {
                        // Spit 3-way aimed volley
                        e.shootTimer = 80;
                        Fixed spd = static_cast<Fixed>(FP_ONE * 3 / 2);
                        Vec2 aimed = {static_cast<Fixed>(fixMul(dir.x, spd)), static_cast<Fixed>(fixMul(dir.y, spd))};
                        { Bullet* lb = spawnBullet(e.pos, aimed, BULLET_COLOR_ENEMY, 4); if (lb) lb->visualType = BVIS_ENEMY; }
                        Vec2 left = {static_cast<Fixed>(aimed.x - aimed.y/3), static_cast<Fixed>(aimed.y + aimed.x/3)};
                        Vec2 right = {static_cast<Fixed>(aimed.x + aimed.y/3), static_cast<Fixed>(aimed.y - aimed.x/3)};
                        { Bullet* lb = spawnBullet(e.pos, left, BULLET_COLOR_ENEMY, 3); if (lb) lb->visualType = BVIS_ENEMY; }
                        { Bullet* lb = spawnBullet(e.pos, right, BULLET_COLOR_ENEMY, 3); if (lb) lb->visualType = BVIS_ENEMY; }
                        e.aiState = 0;
                    }
                }
                break;
            }

            case ETYPE_BOSS_NIGHTMARE_B: { // Nightmare King — circles, fear pulses, spawns nightmares
                e.frame++;
                // Orbit player, reversing direction periodically
                Fixed orbitDir = ((e.frame >> 6) & 1) ? FP_ONE : static_cast<Fixed>(-FP_ONE);
                e.vel.x += fixMul(-dir.y, fixMul(orbitDir, FP_ONE / 10)) + fixMul(dir.x, FP_ONE / 12);
                e.vel.y += fixMul(dir.x, fixMul(orbitDir, FP_ONE / 10)) + fixMul(dir.y, FP_ONE / 12);

                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    if (e.aiState == 0) {
                        // Fear pulse: fear all companions + slow player
                        e.shootTimer = 120;
                        for (auto& comp : gCompanions) {
                            if (comp.active) comp.iframes = 30; // scatter via iframes
                        }
                        // 6-way slow projectiles
                        static const Fixed cos6[] = { 16, 8, -8, -16, -8, 8 };
                        static const Fixed sin6[] = { 0, -14, -14, 0, 14, 14 };
                        for (int a = 0; a < 6; a++) {
                            Vec2 bv = {cos6[a], sin6[a]};
                            Bullet* sb = spawnBullet(e.pos, bv, BULLET_COLOR_ENEMY, 3, BFLAG_SLOW, 0, 60);
                            if (sb) { sb->lifetime = 90; sb->visualType = BVIS_ENEMY; }
                        }
                        spawnParticleBurst(e.pos, 8, 15, 4);
                        cameraShake(4, 8);
                        e.aiState = 1;
                    } else {
                        // Summon 1 small Nightmare
                        e.shootTimer = 90;
                        Fixed ox = toFixed(rngRange(60) - 30);
                        Fixed oy = toFixed(rngRange(60) - 30);
                        Vec2 spos = {static_cast<Fixed>(e.pos.x + ox), static_cast<Fixed>(e.pos.y + oy)};
                        spawnEnemy(spos, {0,0}, static_cast<s16>(e.maxHp / 8), ETYPE_NIGHTMARE, SIZE_SMALL, 8);
                        spawnParticleBurst(spos, 4, 8, 4);
                        e.aiState = 0;
                    }
                }
                break;
            }

            case ETYPE_BOSS_APOTHECARY: { // Apothecary — 3-phase final boss
                e.frame++;
                // Determine phase: 0 = >66% HP, 1 = 33-66%, 2 = <33%
                int phase = (e.hp > e.maxHp * 2 / 3) ? 0 :
                            (e.hp > e.maxHp / 3) ? 1 : 2;

                // Movement: P0 slow orbit, P1 chase + dash, P2 erratic teleport
                if (phase == 0) {
                    // Slow orbit around arena center
                    Fixed cx = toFixed(ARENA_W / 2), cy = toFixed(ARENA_H / 2);
                    Vec2 toC = {static_cast<Fixed>(cx - e.pos.x), static_cast<Fixed>(cy - e.pos.y)};
                    e.vel.x += fixMul(-toC.y, FP_ONE / 20) + fixMul(dir.x, FP_ONE / 12);
                    e.vel.y += fixMul(toC.x, FP_ONE / 20) + fixMul(dir.y, FP_ONE / 12);
                } else if (phase == 1) {
                    // Aggressive chase
                    e.vel.x += fixMul(dir.x, FP_ONE / 4);
                    e.vel.y += fixMul(dir.y, FP_ONE / 4);
                    // Occasional dash (every 180 frames)
                    if ((e.frame % 180) == 0) {
                        e.vel.x = fixMul(dir.x, toFixed(4));
                        e.vel.y = fixMul(dir.y, toFixed(4));
                        cameraShake(3, 6);
                    }
                } else {
                    // Erratic: short teleports every 90 frames
                    e.vel.x += fixMul(dir.x, FP_ONE / 6);
                    e.vel.y += fixMul(dir.y, FP_ONE / 6);
                    if ((e.frame % 90) == 0) {
                        int px = gPlayer.pos.pixelX() + rngRange(120) - 60;
                        int py = gPlayer.pos.pixelY() + rngRange(120) - 60;
                        if (px < WALL_THICK+32) px = WALL_THICK+32;
                        if (px > ARENA_W-WALL_THICK-32) px = ARENA_W-WALL_THICK-32;
                        if (py < WALL_THICK+32) py = WALL_THICK+32;
                        if (py > ARENA_H-WALL_THICK-32) py = ARENA_H-WALL_THICK-32;
                        e.pos = {toFixed(px), toFixed(py)};
                        e.vel = {0, 0};
                        spawnParticleBurst(e.pos, 6, 10, 5);
                    }
                }

                if (e.shootTimer > 0) { e.shootTimer--; }
                else {
                    if (phase == 0) {
                        // Phase 1: Pestle slam — aimed 4-bullet cross + 1 aimed shot
                        e.shootTimer = 90;
                        Fixed spd = toFixed(2);
                        Vec2 aimed = {static_cast<Fixed>(fixMul(dir.x, spd)), static_cast<Fixed>(fixMul(dir.y, spd))};
                        { Bullet* eb = spawnBullet(e.pos, aimed, BULLET_COLOR_ENEMY, 6); if (eb) eb->visualType = BVIS_ENEMY; }
                        { Bullet* eb = spawnBullet(e.pos, {spd, 0}, BULLET_COLOR_ENEMY, 4); if (eb) eb->visualType = BVIS_ENEMY; }
                        { Bullet* eb = spawnBullet(e.pos, {static_cast<Fixed>(-spd), 0}, BULLET_COLOR_ENEMY, 4); if (eb) eb->visualType = BVIS_ENEMY; }
                        { Bullet* eb = spawnBullet(e.pos, {0, spd}, BULLET_COLOR_ENEMY, 4); if (eb) eb->visualType = BVIS_ENEMY; }
                        { Bullet* eb = spawnBullet(e.pos, {0, static_cast<Fixed>(-spd)}, BULLET_COLOR_ENEMY, 4); if (eb) eb->visualType = BVIS_ENEMY; }
                        cameraShake(4, 8);
                    } else if (phase == 1) {
                        // Phase 2: alternating — spawn adds or grinding zones
                        if (e.aiState & 1) {
                            // Spawn 2 mixed adds
                            e.shootTimer = 70;
                            static const u8 addPool[] = {ETYPE_CHARGER, ETYPE_SPITTER, ETYPE_SHIELD, ETYPE_BOMBER};
                            for (int s = 0; s < 2; s++) {
                                u8 addType = addPool[rngRange(4)];
                                Fixed ox = toFixed(rngRange(48) - 24);
                                Fixed oy = toFixed(rngRange(48) - 24);
                                Vec2 spos = {static_cast<Fixed>(e.pos.x + ox), static_cast<Fixed>(e.pos.y + oy)};
                                spawnEnemy(spos, {0,0}, 12, addType, SIZE_SMALL, 8);
                            }
                            spawnParticleBurst(e.pos, 6, 10, 2);
                        } else {
                            // Spawn 2 poison zones near player
                            e.shootTimer = 80;
                            for (int z = 0; z < 2; z++) {
                                Vec2 zpos = gPlayer.pos;
                                zpos.x = static_cast<Fixed>(zpos.x + toFixed(rngRange(40) - 20));
                                zpos.y = static_cast<Fixed>(zpos.y + toFixed(rngRange(40) - 20));
                                spawnZone(zpos, 20, 2, 20, 180, 4, ZONE_POISON);
                            }
                        }
                        e.aiState++;
                    } else {
                        // Phase 3: Meltdown — 6-way hex burst every attack
                        e.shootTimer = 45;
                        static const Fixed cos6[] = { 16, 8, -8, -16, -8, 8 };
                        static const Fixed sin6[] = { 0, -14, -14, 0, 14, 14 };
                        for (int a = 0; a < 6; a++) {
                            Vec2 bv = {static_cast<Fixed>(cos6[a] * 2), static_cast<Fixed>(sin6[a] * 2)};
                            Bullet* sb = spawnBullet(e.pos, bv, BULLET_COLOR_ENEMY, 5);
                            if (sb) { sb->lifetime = 50; sb->visualType = BVIS_ENEMY; }
                        }
                        // Also fire aimed shot
                        Fixed spd = toFixed(3);
                        { Bullet* eb = spawnBullet(e.pos, {static_cast<Fixed>(fixMul(dir.x, spd)), static_cast<Fixed>(fixMul(dir.y, spd))}, BULLET_COLOR_ENEMY, 7); if (eb) eb->visualType = BVIS_ENEMY; }
                        cameraShake(2, 4);
                        // Spawn dangerous adds periodically
                        if ((e.aiState & 3) == 0) {
                            u8 addType = (rngRange(3) == 0) ? ETYPE_NIGHTMARE : ETYPE_BOMBER;
                            Fixed ox = toFixed(rngRange(60) - 30);
                            Vec2 spos = {static_cast<Fixed>(e.pos.x + ox), static_cast<Fixed>(e.pos.y + ox)};
                            spawnEnemy(spos, {0,0}, 8, addType, SIZE_SMALL, 8);
                        }
                        e.aiState++;
                    }
                }
                break;
            }

            default: break;
        }

        applySpeedCap:
        // Speed cap table indexed by type (0-16 regular+new, 17+ bosses)
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
            static_cast<Fixed>(FP_ONE * 3 / 2),  // 12 Medic
            static_cast<Fixed>(FP_ONE * 3 / 4),  // 13 Anchor
            toFixed(1),                           // 14 Trapper
            static_cast<Fixed>(FP_ONE * 3 / 4),  // 15 Hexer
            static_cast<Fixed>(FP_ONE / 2),      // 16 Hive (stationary)
            static_cast<Fixed>(FP_ONE * 3 / 2),  // 17 Boss: Sentinel — slower orbit, hittable
            toFixed(5),                           // 18 Boss: Dreadnought — needs slam dash
            static_cast<Fixed>(FP_ONE * 3 / 2),  // 19 Boss: Leviathan
            static_cast<Fixed>(FP_ONE * 3 / 2),  // 20 Boss: Nightmare
            toFixed(2),                           // 21 Boss: Apothecary
        };
        int typeIdx = (e.type < 22) ? e.type : 20;
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
void killEnemy(Enemy& e, u8 bulletColor) {
    // Track enemy index for Bounty Board
    int enemyIdx = static_cast<int>(&e - &gEnemies[0]);

    e.active = false;
    spawnParticleBurst(e.pos, 10, 14, bulletColor);
    audioPlaySfx(GSFX_EXPLODE);

    // Bloodlust: track kills
    perkOnEnemyKill(enemyIdx);

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
        // Damage player if in blast radius (respects iframes)
        if (gPlayer.iframes == 0 && !gPlayer.dashInvincible) {
            int dpx = gPlayer.pos.pixelX() - bx;
            int dpy = gPlayer.pos.pixelY() - by;
            if (dpx * dpx + dpy * dpy <= 32 * 32) {
                s16 dmg = synergyOnPlayerHit(4);
                // Juggernaut: reduce all incoming damage by 1 (min 1)
                if (perkIsActive(PERK_JUGGERNAUT) && dmg > 1) dmg--;
                if (dmg > 0) {
                    gPlayer.hp -= dmg;
                    gPlayer.iframes = 90;
                    perkOnPlayerHit();
                }
            }
        }
        spawnParticleBurst(e.pos, 8, 12, 1); // red burst
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
        case ETYPE_MEDIC:       goldValue = 3;  break;
        case ETYPE_ANCHOR:      goldValue = 3;  break;
        case ETYPE_TRAPPER:     goldValue = 2;  break;
        case ETYPE_HEXER:       goldValue = 3;  break;
        case ETYPE_HIVE:        goldValue = 4;  break;
        default:                goldValue = 15; break; // bosses
    }
    // Late-game gold scaling: +12% per wave past 15
    {
        int wave = gameGetWave();
        if (wave > 15) {
            goldValue = static_cast<u8>(goldValue * (100 + (wave - 15) * 12) / 100);
        }
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
    // Double or Nothing: wave-wide gold modifier
    if (gPerks.doubleGoldWave) {
        goldValue = static_cast<u8>(goldValue * 2);
    } else if (gPerks.zeroGoldWave) {
        goldValue = 0;
    }
    // Bounty Board: marked enemy drops 3x gold
    if (gPerks.bountyEnemyIdx >= 0 && enemyIdx == gPerks.bountyEnemyIdx) {
        goldValue = static_cast<u8>(goldValue * 3);
        gPerks.bountyEnemyIdx = -1; // bounty claimed
        spawnParticleBurst(e.pos, 12, 16, 3); // gold burst
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
            e.hurtTimer = 4;
            spawnParticleBurst(e.pos, 4, 8, b.color);
            if (e.hp <= 0) killEnemy(e, b.color);
        }
    }
    // Spawn a brief BURN zone at impact — renders as a visible filled circle
    // like poison clouds, but only lasts 30 frames (0.5s) with no damage ticks
    spawnZone({toFixed(bx), toFixed(by)}, static_cast<u8>(r > 48 ? 48 : r),
              0, 255, 30, 0, ZONE_BURN);
    // Particle burst for extra flair
    spawnParticleBurst({toFixed(bx), toFixed(by)}, 6, 20, b.color);
    cameraShake(2, 4);
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
            if ((b.flags & BFLAG_FREEZE) && b.effectDuration > 0) {
                u16 dur = b.effectDuration;
                if (gPassive.debuffExtendPct > 0)
                    dur = static_cast<u16>(dur * (100 + gPassive.debuffExtendPct) / 100);
                e.freezeTimer = static_cast<u8>(dur > 255 ? 255 : dur);
            } else if ((b.flags & BFLAG_SLOW) && b.effectDuration > 0) {
                u16 dur = b.effectDuration;
                if (gPassive.debuffExtendPct > 0)
                    dur = static_cast<u16>(dur * (100 + gPassive.debuffExtendPct) / 100);
                e.slowTimer = static_cast<u8>(dur > 255 ? 255 : dur);
            }

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

            // PASSIVE_CLOSE_DAMAGE: boost damage if enemy is near the player
            if (gPassive.closeDmgBoostPct > 0 && gPassive.closeDmgRange > 0) {
                int dx = e.pos.pixelX() - gPlayer.pos.pixelX();
                int dy = e.pos.pixelY() - gPlayer.pos.pixelY();
                int range = gPassive.closeDmgRange;
                if (dx * dx + dy * dy <= range * range) {
                    dmg = static_cast<u8>(dmg * (100 + gPassive.closeDmgBoostPct) / 100);
                }
            }

            // Anchor aura: enemies within 32px of an active Anchor take 30% less damage
            for (auto& anchor : gEnemies) {
                if (!anchor.active || anchor.type != ETYPE_ANCHOR) continue;
                if (&anchor == &e) continue;
                Vec2 ad = anchor.pos - e.pos;
                s32 adist = static_cast<s32>(ad.x) * ad.x + static_cast<s32>(ad.y) * ad.y;
                if (adist < static_cast<s32>(toFixed(32)) * toFixed(32)) {
                    dmg = static_cast<u8>(dmg * 70 / 100);
                    break; // only one Anchor reduction
                }
            }

            e.hp -= dmg;
            e.hurtTimer = 4;
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
        // Magnet perk: double pickup range
        if (perkIsActive(PERK_MAGNET)) bonus += 8 + bonus; // effectively doubles total range
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
            c.iframes = 180;  // same generous invuln as player (3 seconds)
            spawnParticleBurst(c.pos, 4, 6, 3);

            // Thorns perk: companions deal 5 damage back when hit
            if (perkIsActive(PERK_THORNS) && e.active) {
                e.hp -= 5;
                e.hurtTimer = 4;
                spawnParticleBurst(e.pos, 3, 6, 4); // purple thorn burst
                if (e.hp <= 0) killEnemy(e, static_cast<u8>(c.color));
            }

            if (c.hp <= 0) {
                spawnParticleBurst(c.pos, 12, 15, static_cast<u8>(c.color));
                companionRemove(i);
            }
            break;  // one enemy hit per companion per frame — prevents stacking
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
            if (gPlayer.dashInvincible) continue;
            if (gPlayer.iframes > 0) continue;  // invincible after recent hit
            // Route damage through synergy (shield absorption, damage reduction)
            u8 rawDmg = (e.type == ETYPE_BOMBER) ? 6 : 3;
            s16 dmg = synergyOnPlayerHit(rawDmg);
            // Juggernaut: reduce all incoming damage by 1 (min 1)
            if (perkIsActive(PERK_JUGGERNAUT) && dmg > 1) {
                dmg--;
            }
            if (dmg > 0) {
                gPlayer.hp -= dmg;
                gPlayer.iframes = 90;  // 1.5 seconds of mercy
                perkOnPlayerHit();
                spawnParticleBurst(gPlayer.pos, 8, 10, 3);
                audioPlaySfx(GSFX_PLAYER_HIT);
            }
            // Only bombers die on contact — other enemies bounce off
            if (e.type == ETYPE_BOMBER) {
                killEnemy(e, 0);
            }
            break;  // only take damage from one enemy per frame
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
                e.hurtTimer = 4;
                spawnParticle(e.pos, {0, 0}, 10, z.color);
                if (e.hp <= 0) killEnemy(e, z.color);
            }
        }
    }
}

void combatUpdate() {
    // Bounty Board: assign bounty target on first frame enemies exist
    if (gPerks.bountyEnemyIdx == -2) {
        int count = 0;
        for (auto& e : gEnemies) { if (e.active) count++; }
        if (count > 0) {
            int pick = rngRange(count);
            int idx = 0;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!gEnemies[i].active) continue;
                if (idx == pick) { gPerks.bountyEnemyIdx = static_cast<s8>(i); break; }
                idx++;
            }
        }
    }

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
