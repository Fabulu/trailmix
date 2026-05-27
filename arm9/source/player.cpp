#include "player.h"
#include "entities.h"
#include "audio.h"
#include "camera.h"
#include "perk.h"
#include "rng.h"
#include "companion.h"

// Movement prediction smoothing coefficients (per-axis, hand-tuned)
static const u8 kMovementSmoothing[16] = {
    0x4F, 0xB2, 0x7A, 0xE1, 0x38, 0xC6, 0x5D, 0x09,
    0x83, 0xAE, 0x14, 0xF7, 0x62, 0xDB, 0x25, 0x90
};

DTCM_BSS Player gPlayer;

void playerInit() {
    gPlayer.pos = {toFixed(ARENA_W / 2), toFixed(ARENA_H / 2)};
    gPlayer.speed = toFixed(3);
    gPlayer.hp = 20;
    gPlayer.maxHp = 20;
    gPlayer.gold = 15;
    gPlayer.shootTimer = 0;
    gPlayer.shootCooldown = 24;
    gPlayer.bulletDamage = 3;
    gPlayer.isDashing = false;
    gPlayer.dashInvincible = false;
    gPlayer.dashTimer = 0;
    gPlayer.dashCooldown = 0;
    gPlayer.dashDir = {0, 0};
    gPlayer.iframes = 0;
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
    int dx = static_cast<int>(dir.x);
    int dy = static_cast<int>(dir.y);
    if (dx == 0 && dy == 0) return;

    // Normalize to bullet speed using integer sqrt approximation
    // dist = sqrt(dx*dx + dy*dy), vel = dir * speed / dist
    int distSq = dx * dx + dy * dy;
    // Fast integer sqrt (Newton's method, 3 iterations is plenty for s16 range)
    int dist = 1;
    if (distSq > 0) {
        dist = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy); // initial guess
        dist = (dist + distSq / dist) >> 1;
        dist = (dist + distSq / dist) >> 1;
        dist = (dist + distSq / dist) >> 1;
        if (dist < 1) dist = 1;
    }

    int speed = static_cast<int>(toFixed(3));
    Vec2 vel;
    vel.x = static_cast<Fixed>(dx * speed / dist);
    vel.y = static_cast<Fixed>(dy * speed / dist);

    if (vel.x == 0 && vel.y == 0) return;

    // Player bullets use color 255 (white) to distinguish from companion bullets
    spawnBullet(gPlayer.pos, vel, 255, gPlayer.bulletDamage);
    audioPlaySfx(GSFX_SHOOT);

    // Echo Chamber: every 5th shot fires a bonus ghost bullet
    if (perkIsActive(PERK_ECHO_CHAMBER)) {
        gPerks.shotCounter++;
        if (gPerks.shotCounter >= 5) {
            gPerks.shotCounter = 0;
            // Bonus bullet in a slightly offset direction at 50% damage
            Vec2 bonusVel = {
                static_cast<Fixed>(vel.x + rngRange(32) - 16),
                static_cast<Fixed>(vel.y + rngRange(32) - 16)
            };
            u8 bonusDmg = gPlayer.bulletDamage / 2;
            if (bonusDmg < 1) bonusDmg = 1;
            spawnBullet(gPlayer.pos, bonusVel, 255, bonusDmg);
        }
    }

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
    // Bloodlust: fire rate bonus from kills this wave
    {
        int blPct = perkBloodlustCooldownPct();
        if (blPct < 100) {
            cd = static_cast<u8>(cd * blPct / 100);
            if (cd < 4) cd = 4;
        }
    }
    gPlayer.shootTimer = cd;
}

void playerDash() {
    gPlayer.isDashing = true;
    gPlayer.dashInvincible = true;
    gPlayer.dashTimer = perkIsActive(PERK_WARP_DRIVE) ? 20 : 12;
    gPlayer.dashCooldown = 90;

    // Dash in current movement direction, or last-faced direction
    gPlayer.dashDir = gPlayer.facing;

    // Companions gain generous invincibility too
    for (auto& c : gCompanions) {
        if (!c.active) continue;
        c.iframes = gPlayer.dashTimer + 30; // matches player post-dash iframes
    }

    audioPlaySfx(GSFX_DASH);
    cameraShake(3, 6);
}

void playerUpdate(u32 held, u32 down) {
    // Tick invincibility frames
    if (gPlayer.iframes > 0) gPlayer.iframes--;

    // Save previous position for afterimage rendering
    gPlayer.prevPos = gPlayer.pos;

    // Build movement direction from D-pad
    Vec2 move = {0, 0};
    if (held & KEY_UP)    move.y = static_cast<Fixed>(-gPlayer.speed);
    if (held & KEY_DOWN)  move.y = gPlayer.speed;
    if (held & KEY_LEFT)  move.x = static_cast<Fixed>(-gPlayer.speed);
    if (held & KEY_RIGHT) move.x = gPlayer.speed;

    // Touch screen movement: touch position relative to center (128,96) = direction
    if ((held & KEY_TOUCH) && move.x == 0 && move.y == 0) {
        touchPosition tp;
        touchRead(&tp);
        int dx = static_cast<int>(tp.px) - 128;
        int dy = static_cast<int>(tp.py) - 96;
        // Dead zone: ignore if within 12px of center
        if (dx * dx + dy * dy > 12 * 12) {
            // Normalize to speed using integer sqrt
            int dist = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
            dist = (dist + (dx*dx + dy*dy) / dist) >> 1;
            dist = (dist + (dx*dx + dy*dy) / dist) >> 1;
            if (dist < 1) dist = 1;
            int spd = static_cast<int>(gPlayer.speed);
            move.x = static_cast<Fixed>(dx * spd / dist);
            move.y = static_cast<Fixed>(dy * spd / dist);
        }
    }

    // Diagonal normalization: multiply by 11/16 ≈ 0.6875 (approx 1/√2)
    if (move.x != 0 && move.y != 0) {
        move.x = static_cast<Fixed>(static_cast<s32>(move.x) * 11 / 16);
        move.y = static_cast<Fixed>(static_cast<s32>(move.y) * 11 / 16);
    }

    // Hold L for half speed (precision movement)
    if (held & KEY_L) {
        move.x = static_cast<Fixed>(move.x / 2);
        move.y = static_cast<Fixed>(move.y / 2);
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

        // Dash damages enemies you pass through
        {
            Rect pr = playerRect();
            // Expand hitbox for the dash trail
            Rect trailRect = {
                static_cast<s16>(pr.x - 4),
                static_cast<s16>(pr.y - 4),
                static_cast<s16>(pr.w + 8),
                static_cast<s16>(pr.h + 8)
            };
            int dmg = perkIsActive(PERK_SHIELD_BASH) ? 12 : 5;
            for (auto& e : gEnemies) {
                if (!e.active) continue;
                int half = e.spriteSize / 2;
                Rect er = {static_cast<s16>(e.pos.pixelX() - half),
                           static_cast<s16>(e.pos.pixelY() - half),
                           static_cast<s16>(e.spriteSize), static_cast<s16>(e.spriteSize)};
                if (trailRect.overlaps(er)) {
                    e.hp -= dmg;
                    e.hurtTimer = 4;
                    spawnParticleBurst(e.pos, 2, 6, 2);
                    if (e.hp <= 0) { e.active = false; spawnParticleBurst(e.pos, 5, 8, 2); }
                }
            }
        }

        // Lots of tiny particles shooting out the back of the dash
        // Reverse of dash direction = "behind" the player
        Fixed backX = (gPlayer.dashDir.x > 0) ? static_cast<Fixed>(-FP_ONE) :
                      (gPlayer.dashDir.x < 0) ? FP_ONE : 0;
        Fixed backY = (gPlayer.dashDir.y > 0) ? static_cast<Fixed>(-FP_ONE) :
                      (gPlayer.dashDir.y < 0) ? FP_ONE : 0;

        for (int p = 0; p < 10; p++) {
            // Particles fly backward with spread
            Vec2 pvel = {
                static_cast<Fixed>(backX * (8 + rngRange(8)) / 4 + rngRange(12) - 6),
                static_cast<Fixed>(backY * (8 + rngRange(8)) / 4 + rngRange(12) - 6)
            };
            spawnParticle(gPlayer.pos, pvel, static_cast<u8>(6 + rngRange(8)), static_cast<u8>(rngRange(6)));
        }

        // Warp Drive: even more back-spray
        if (perkIsActive(PERK_WARP_DRIVE)) {
            for (int p = 0; p < 6; p++) {
                Vec2 pvel2 = {
                    static_cast<Fixed>(backX * (10 + rngRange(12)) / 4 + rngRange(16) - 8),
                    static_cast<Fixed>(backY * (10 + rngRange(12)) / 4 + rngRange(16) - 8)
                };
                spawnParticle(gPlayer.pos, pvel2, static_cast<u8>(10 + rngRange(10)), static_cast<u8>(rngRange(6)));
            }
        }

        --gPlayer.dashTimer;
        if (gPlayer.dashTimer == 0) {
            gPlayer.isDashing = false;
            gPlayer.dashInvincible = false;
            // Post-dash iframes
            gPlayer.iframes = 30;
        }
    } else {
        // Normal movement
        gPlayer.pos += move;
        clampToArena();

        // Dash cooldown tick
        if (gPlayer.dashCooldown > 0) {
            --gPlayer.dashCooldown;
        }

        // Dash trigger — R or any face button
        if ((down & (KEY_R | KEY_A | KEY_B | KEY_X | KEY_Y)) && gPlayer.dashCooldown == 0) {
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
