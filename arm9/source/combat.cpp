#include "combat.h"
#include "player.h"
#include "entities.h"
#include "audio.h"
#include "rng.h"

// Find the nearest active enemy to the player
static Enemy* findNearestEnemy() {
    Enemy* nearest = nullptr;
    s32 bestDist = 0x7FFFFFFF;

    for (auto& e : gEnemies) {
        if (!e.active) continue;
        Vec2 diff = e.pos - gPlayer.pos;
        s32 dist = static_cast<s32>(diff.x) * diff.x + static_cast<s32>(diff.y) * diff.y;
        if (dist < bestDist) {
            bestDist = dist;
            nearest = &e;
        }
    }
    return nearest;
}

// Auto-shoot toward nearest enemy
static void autoShoot() {
    if (gPlayer.shootTimer > 0) return;

    Enemy* target = findNearestEnemy();
    if (!target) return;

    Vec2 dir = target->pos - gPlayer.pos;

    // Normalize direction to bullet speed (fixed-point)
    // Simple 8-direction approximation for now
    Fixed bspeed = toFixed(3);
    Vec2 vel = {0, 0};

    if (dir.x > 0) vel.x = bspeed;
    else if (dir.x < 0) vel.x = static_cast<Fixed>(-bspeed);

    if (dir.y > 0) vel.y = bspeed;
    else if (dir.y < 0) vel.y = static_cast<Fixed>(-bspeed);

    if (vel.x == 0 && vel.y == 0) return;

    u8 dmg = static_cast<u8>(gPlayer.bulletDamage + playerSynergyLevel(PillColor::Red));
    spawnBullet(gPlayer.pos, vel, 0, dmg);
    audioPlaySfx(SFX_SHOOT);

    // Apply fire rate synergy
    int frSynergy = playerSynergyLevel(PillColor::Yellow);
    int cooldown = gPlayer.shootCooldown - frSynergy;
    if (cooldown < 3) cooldown = 3;
    gPlayer.shootTimer = static_cast<u8>(cooldown);
}

// Update bullets: move and remove off-screen
static void updateBullets() {
    for (auto& b : gBullets) {
        if (!b.active) continue;
        b.pos += b.vel;
        int px = b.pos.pixelX();
        int py = b.pos.pixelY();
        if (px < -8 || px > SCREEN_W + 8 || py < -8 || py > SCREEN_H + 8) {
            b.active = false;
        }
    }
}

// Update enemies: move, apply simple AI
static void updateEnemies() {
    for (auto& e : gEnemies) {
        if (!e.active) continue;
        e.pos += e.vel;
        e.frame++;

        // Remove if off-screen (with margin)
        int px = e.pos.pixelX();
        int py = e.pos.pixelY();
        if (px < -32 || px > SCREEN_W + 32 || py < -32 || py > SCREEN_H + 32) {
            e.active = false;
        }
    }
}

// Update pill drops: countdown lifetime
static void updatePillDrops() {
    for (auto& p : gPills) {
        if (!p.active) continue;
        if (p.lifetime > 0) {
            p.lifetime--;
        } else {
            p.active = false;
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

// Check bullet-enemy collisions
static void checkBulletEnemyCollisions() {
    for (auto& b : gBullets) {
        if (!b.active) continue;
        Rect br = {
            static_cast<s16>(b.pos.pixelX() - 2),
            static_cast<s16>(b.pos.pixelY() - 2),
            4, 4
        };

        for (auto& e : gEnemies) {
            if (!e.active) continue;
            int half = e.size / 2;
            Rect er = {
                static_cast<s16>(e.pos.pixelX() - half),
                static_cast<s16>(e.pos.pixelY() - half),
                static_cast<s16>(e.size),
                static_cast<s16>(e.size)
            };

            if (br.overlaps(er)) {
                e.hp -= b.damage;
                b.active = false;

                spawnParticleBurst(e.pos, 4, 8, b.color);

                if (e.hp <= 0) {
                    e.active = false;
                    spawnParticleBurst(e.pos, 12, 15, b.color);
                    audioPlaySfx(SFX_EXPLODE);

                    // Drop a pill at the player's current tier for that color
                    PillColor dropColor = static_cast<PillColor>(rngRange(PILL_COLOR_COUNT));
                    PillTier dropTier = gPlayer.inventory[static_cast<int>(dropColor)].tier;
                    spawnPill(e.pos, dropColor, dropTier);
                }
                break;
            }
        }
    }
}

// Check player-pill collisions
static void checkPlayerPillCollisions() {
    Rect pr = playerRect();

    for (auto& p : gPills) {
        if (!p.active) continue;
        Rect pillR = {
            static_cast<s16>(p.pos.pixelX() - 4),
            static_cast<s16>(p.pos.pixelY() - 4),
            8, 8
        };

        if (pr.overlaps(pillR)) {
            bool merged = playerCollectPill(p.color, p.tier);
            if (merged) {
                spawnParticleBurst(p.pos, 16, 20, static_cast<u8>(p.color));
            }
            p.active = false;
        }
    }
}

// Check enemy-player collisions
static void checkEnemyPlayerCollisions() {
    Rect pr = playerRect();

    for (auto& e : gEnemies) {
        if (!e.active) continue;
        int half = e.size / 2;
        Rect er = {
            static_cast<s16>(e.pos.pixelX() - half),
            static_cast<s16>(e.pos.pixelY() - half),
            static_cast<s16>(e.size),
            static_cast<s16>(e.size)
        };

        if (pr.overlaps(er)) {
            // Green synergy reduces contact damage
            int regenLevel = playerSynergyLevel(PillColor::Green);
            int dmg = 3 - regenLevel;
            if (dmg < 1) dmg = 1;
            gPlayer.hp -= static_cast<s16>(dmg);

            e.active = false;
            spawnParticleBurst(gPlayer.pos, 8, 10, 3);
            audioPlaySfx(SFX_HIT);
        }
    }
}

void combatUpdate() {
    autoShoot();
    updateBullets();
    updateEnemies();
    updatePillDrops();
    updateParticles();
    checkBulletEnemyCollisions();
    checkPlayerPillCollisions();
    checkEnemyPlayerCollisions();
}
