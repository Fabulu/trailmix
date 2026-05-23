#ifndef ENTITIES_H
#define ENTITIES_H

#include "types.h"

constexpr int MAX_BULLETS   = 64;
constexpr int MAX_ENEMIES   = 32;
constexpr int MAX_PILLS     = 16;
constexpr int MAX_PARTICLES  = 128;

struct Bullet {
    Vec2 pos;
    Vec2 vel;
    u8 color;
    u8 damage;
    bool active;
};

struct Enemy {
    Vec2 pos;
    Vec2 vel;
    s16 hp;
    s16 maxHp;
    u8 type;       // geometric shape type
    u8 size;       // collision/render size in pixels
    u8 frame;
    bool active;
};

struct PillDrop {
    Vec2 pos;
    PillColor color;
    PillTier tier;
    u8 lifetime;   // frames before despawn
    bool active;
};

struct Particle {
    Vec2 pos;
    Vec2 vel;
    u8 life;       // frames remaining
    u8 maxLife;
    u8 color;      // palette index
    bool active;
};

// Global pools
extern Bullet   gBullets[MAX_BULLETS];
extern Enemy    gEnemies[MAX_ENEMIES];
extern PillDrop gPills[MAX_PILLS];
extern Particle gParticles[MAX_PARTICLES];

void entitiesInit();
void entitiesClear();

Bullet*   spawnBullet(Vec2 pos, Vec2 vel, u8 color, u8 damage);
Enemy*    spawnEnemy(Vec2 pos, Vec2 vel, s16 hp, u8 type, u8 size);
PillDrop* spawnPill(Vec2 pos, PillColor color, PillTier tier);
Particle* spawnParticle(Vec2 pos, Vec2 vel, u8 life, u8 color);

// Spawn a burst of particles (for explosions, merges)
void spawnParticleBurst(Vec2 center, int count, u8 life, u8 color);

#endif // ENTITIES_H
