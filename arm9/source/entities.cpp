#include "entities.h"
#include "rng.h"

Bullet   gBullets[MAX_BULLETS];
Enemy    gEnemies[MAX_ENEMIES];
PillDrop gPills[MAX_PILLS];
Particle gParticles[MAX_PARTICLES];

void entitiesInit() {
    entitiesClear();
}

void entitiesClear() {
    for (auto& b : gBullets)   b.active = false;
    for (auto& e : gEnemies)   e.active = false;
    for (auto& p : gPills)     p.active = false;
    for (auto& p : gParticles) p.active = false;
}

template<typename T, int N>
static T* findSlot(T (&pool)[N]) {
    for (auto& e : pool) {
        if (!e.active) return &e;
    }
    return nullptr;
}

Bullet* spawnBullet(Vec2 pos, Vec2 vel, u8 color, u8 damage) {
    Bullet* b = findSlot(gBullets);
    if (!b) return nullptr;
    b->pos = pos;
    b->vel = vel;
    b->color = color;
    b->damage = damage;
    b->active = true;
    return b;
}

Enemy* spawnEnemy(Vec2 pos, Vec2 vel, s16 hp, u8 type, u8 size) {
    Enemy* e = findSlot(gEnemies);
    if (!e) return nullptr;
    e->pos = pos;
    e->vel = vel;
    e->hp = hp;
    e->maxHp = hp;
    e->type = type;
    e->size = size;
    e->frame = 0;
    e->active = true;
    return e;
}

PillDrop* spawnPill(Vec2 pos, PillColor color, PillTier tier) {
    PillDrop* p = findSlot(gPills);
    if (!p) return nullptr;
    p->pos = pos;
    p->color = color;
    p->tier = tier;
    p->lifetime = 255;
    p->active = true;
    return p;
}

Particle* spawnParticle(Vec2 pos, Vec2 vel, u8 life, u8 color) {
    Particle* p = findSlot(gParticles);
    if (!p) return nullptr;
    p->pos = pos;
    p->vel = vel;
    p->life = life;
    p->maxLife = life;
    p->color = color;
    p->active = true;
    return p;
}

void spawnParticleBurst(Vec2 center, int count, u8 life, u8 color) {
    for (int i = 0; i < count; ++i) {
        Vec2 vel;
        vel.x = static_cast<Fixed>((rngRange(32) - 16));
        vel.y = static_cast<Fixed>((rngRange(32) - 16));
        spawnParticle(center, vel, life, color);
    }
}
