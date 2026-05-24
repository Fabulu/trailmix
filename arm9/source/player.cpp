#include "player.h"
#include "entities.h"
#include "audio.h"
#include "camera.h"
#include "perk.h"
#include "rng.h"

DTCM_BSS Player gPlayer;

void playerInit() {
    gPlayer.pos = {toFixed(ARENA_W / 2), toFixed(ARENA_H / 2)};
    gPlayer.speed = toFixed(3);
    gPlayer.hp = 20;
    gPlayer.maxHp = 20;
    gPlayer.gold = 0;
    gPlayer.shootTimer = 0;
    gPlayer.shootCooldown = 24;
    gPlayer.bulletDamage = 3;
    gPlayer.isDashing = false;
    gPlayer.dashInvincible = false;
    gPlayer.dashTimer = 0;
    gPlayer.dashCooldown = 0;
    gPlayer.dashDir = {0, 0};
    gPlayer.facing = {toFixed(1), 0}; // default facing right
}

// Clamp player position to arena bounds (inside walls)
static void clampToArena() {
    Fixed minX = toFixed(WALL_THICK + PLAYER_SIZE / 2);
    Fixed maxX = toFixed(ARENA_W - WALL_THICK - PLAYER_SIZE / 2);
    Fixed minY = toFixed(WALL_THICK + PLAYER_SIZE / 2);
    Fixed maxY = toFixed(ARENA_H - WALL_THICK - PLAYER_SIZE / 2);

    if (gPlayer.pos.x < minX) gPlayer.pos.x = minX;
    if (gPlayer.pos.x > maxX) gPlayer.pos.x = maxX;
    if (gPlayer.pos.y < minY) gPlayer.pos.y = minY;
    if (gPlayer.pos.y > maxY) gPlayer.pos.y = maxY;
}

// Find nearest active enemy
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
    if (gPlayer.shootTimer > 0) {
        --gPlayer.shootTimer;
        return;
    }

    Enemy* target = findNearestEnemy();
    if (!target) return;

    Vec2 dir = target->pos - gPlayer.pos;

    // Simple 8-direction approximation
    Fixed bspeed = toFixed(3);
    Vec2 vel = {0, 0};

    if (dir.x > 0) vel.x = bspeed;
    else if (dir.x < 0) vel.x = static_cast<Fixed>(-bspeed);

    if (dir.y > 0) vel.y = bspeed;
    else if (dir.y < 0) vel.y = static_cast<Fixed>(-bspeed);

    if (vel.x == 0 && vel.y == 0) return;

    // Player bullets use color 255 (white) to distinguish from companion bullets
    spawnBullet(gPlayer.pos, vel, 255, gPlayer.bulletDamage);
    audioPlaySfx(GSFX_SHOOT);

    // Bullet Hell: fire in all 4 cardinal directions
    if (perkIsActive(PERK_BULLET_HELL)) {
        Fixed spd = toFixed(3);
        spawnBullet(gPlayer.pos, {spd, 0}, 255, gPlayer.bulletDamage);
        spawnBullet(gPlayer.pos, {static_cast<Fixed>(-spd), 0}, 255, gPlayer.bulletDamage);
        spawnBullet(gPlayer.pos, {0, spd}, 255, gPlayer.bulletDamage);
        spawnBullet(gPlayer.pos, {0, static_cast<Fixed>(-spd)}, 255, gPlayer.bulletDamage);
    }

    // Overcharge: player also fires 50% faster
    u8 cd = gPlayer.shootCooldown;
    if (perkIsActive(PERK_OVERCHARGE)) {
        cd = cd / 2;
        if (cd < 4) cd = 4;
    }
    gPlayer.shootTimer = cd;
}

void playerDash() {
    gPlayer.isDashing = true;
    gPlayer.dashInvincible = true;
    gPlayer.dashTimer = perkIsActive(PERK_WARP_DRIVE) ? 16 : 8;
    gPlayer.dashCooldown = 90;

    // Dash in current movement direction, or last-faced direction
    gPlayer.dashDir = gPlayer.facing;

    audioPlaySfx(GSFX_DASH);
    cameraShake(6, 8);
}

void playerUpdate(u32 held, u32 down) {
    // Save previous position for afterimage rendering
    gPlayer.prevPos = gPlayer.pos;

    // Build movement direction from D-pad
    Vec2 move = {0, 0};
    if (held & KEY_UP)    move.y = static_cast<Fixed>(-gPlayer.speed);
    if (held & KEY_DOWN)  move.y = gPlayer.speed;
    if (held & KEY_LEFT)  move.x = static_cast<Fixed>(-gPlayer.speed);
    if (held & KEY_RIGHT) move.x = gPlayer.speed;

    // Diagonal normalization: multiply by 11/16 ≈ 0.6875 (approx 1/√2)
    if (move.x != 0 && move.y != 0) {
        move.x = static_cast<Fixed>(static_cast<s32>(move.x) * 11 / 16);
        move.y = static_cast<Fixed>(static_cast<s32>(move.y) * 11 / 16);
    }

    // Fortress: 25% slower movement
    if (perkIsActive(PERK_FORTRESS)) {
        move.x = static_cast<Fixed>(move.x * 3 / 4);
        move.y = static_cast<Fixed>(move.y * 3 / 4);
    }

    // Update facing direction if player is pressing a direction
    if (move.x != 0 || move.y != 0) {
        gPlayer.facing = move;
    }

    if (gPlayer.isDashing) {
        // Dash movement: 4x speed in dash direction
        Fixed dashSpeed = toFixed(8); // 4x base speed of 2
        Vec2 dashMove = {0, 0};

        // Normalize dashDir to unit-ish components, then scale by dashSpeed
        if (gPlayer.dashDir.x > 0) dashMove.x = dashSpeed;
        else if (gPlayer.dashDir.x < 0) dashMove.x = static_cast<Fixed>(-dashSpeed);

        if (gPlayer.dashDir.y > 0) dashMove.y = dashSpeed;
        else if (gPlayer.dashDir.y < 0) dashMove.y = static_cast<Fixed>(-dashSpeed);

        gPlayer.pos += dashMove;
        clampToArena();

        // Shield Bash: damage enemies overlapping player during dash
        if (perkIsActive(PERK_SHIELD_BASH)) {
            Rect pr = playerRect();
            for (auto& e : gEnemies) {
                if (!e.active) continue;
                int half = e.spriteSize / 2;
                Rect er = {static_cast<s16>(e.pos.pixelX() - half),
                           static_cast<s16>(e.pos.pixelY() - half),
                           static_cast<s16>(e.spriteSize), static_cast<s16>(e.spriteSize)};
                if (pr.overlaps(er)) {
                    e.hp -= 8;
                    if (e.hp <= 0) { e.active = false; spawnParticleBurst(e.pos, 8, 12, 0); }
                }
            }
        }

        // Spawn trail particle each frame during dash
        Vec2 pvel = {static_cast<Fixed>(rngRange(8) - 4), static_cast<Fixed>(rngRange(8) - 4)};
        spawnParticle(gPlayer.pos, pvel, 10, 2);

        // Warp Drive: extra trail particles + damage hitbox along trail
        if (perkIsActive(PERK_WARP_DRIVE)) {
            Vec2 pvel2 = {static_cast<Fixed>(rngRange(12) - 6), static_cast<Fixed>(rngRange(12) - 6)};
            spawnParticle(gPlayer.pos, pvel2, 14, 5);

            // Damage trail: hurt enemies overlapping the dash path
            Rect pr = playerRect();
            // Expand hitbox slightly for trail width
            Rect trailRect = {
                static_cast<s16>(pr.x - 4),
                static_cast<s16>(pr.y - 4),
                static_cast<s16>(pr.w + 8),
                static_cast<s16>(pr.h + 8)
            };
            for (auto& e : gEnemies) {
                if (!e.active) continue;
                int half = e.spriteSize / 2;
                Rect er = {static_cast<s16>(e.pos.pixelX() - half),
                           static_cast<s16>(e.pos.pixelY() - half),
                           static_cast<s16>(e.spriteSize), static_cast<s16>(e.spriteSize)};
                if (trailRect.overlaps(er)) {
                    e.hp -= 4;
                    spawnParticleBurst(e.pos, 4, 8, 5);
                    if (e.hp <= 0) { e.active = false; spawnParticleBurst(e.pos, 8, 12, 5); }
                }
            }
        }

        --gPlayer.dashTimer;
        if (gPlayer.dashTimer == 0) {
            gPlayer.isDashing = false;
            gPlayer.dashInvincible = false;
        }
    } else {
        // Normal movement
        gPlayer.pos += move;
        clampToArena();

        // Dash cooldown tick
        if (gPlayer.dashCooldown > 0) {
            --gPlayer.dashCooldown;
        }

        // Dash trigger
        if ((down & KEY_R) && gPlayer.dashCooldown == 0) {
            playerDash();
        }
    }

    // Auto-shoot
    autoShoot();
}

Rect playerRect() {
    int px = gPlayer.pos.pixelX();
    int py = gPlayer.pos.pixelY();
    return {
        static_cast<s16>(px - PLAYER_SIZE / 2),
        static_cast<s16>(py - PLAYER_SIZE / 2),
        PLAYER_SIZE,
        PLAYER_SIZE
    };
}
