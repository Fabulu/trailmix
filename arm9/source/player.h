#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"

constexpr int PLAYER_SIZE = 12;      // pixels

struct Player {
    Vec2 pos;
    Fixed speed;
    s16 hp;
    s16 maxHp;
    u16 gold;
    // Shooting
    u8 shootTimer;
    u8 shootCooldown;  // base: 24 frames
    u8 bulletDamage;   // base: 3
    // Dash
    bool isDashing;
    bool dashInvincible;
    u8 dashTimer;       // frames remaining in current dash
    u8 dashCooldown;    // frames until dash available (max 90)
    Vec2 dashDir;
    Vec2 facing;        // last-faced direction for dash fallback
    Vec2 prevPos;       // previous frame position (for afterimage rendering)
};

extern Player gPlayer;

void playerInit();
void playerUpdate(u32 keysHeld, u32 keysDown);
Rect playerRect();
void playerDash();

#endif // PLAYER_H
