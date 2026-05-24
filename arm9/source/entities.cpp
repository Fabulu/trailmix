#include "entities.h"
#include "rng.h"

Bullet   gBullets[MAX_BULLETS];
Enemy    gEnemies[MAX_ENEMIES];
GoldDrop gGold[MAX_GOLD];
Swarm    gSwarms[MAX_SWARMS];
HealOrb  gHealOrbs[MAX_HEAL_ORBS];
Particle gParticles[MAX_PARTICLES];
Zone     gZones[MAX_ZONES];

void entitiesInit() {
    entitiesClear();
}

void entitiesClear() {
    for (auto& b : gBullets)   b.active = false;
    for (auto& e : gEnemies)   e.active = false;
    for (auto& g : gGold)      g.active = false;
    for (auto& s : gSwarms)    s.active = false;
    for (auto& h : gHealOrbs)  h.active = false;
    for (auto& p : gParticles) p.active = false;
    for (auto& z : gZones)     z.active = false;
}

template<typename T, int N>
static T* findSlot(T (&pool)[N]) {
    for (auto& e : pool) {
        if (!e.active) return &e;
    }
    return nullptr;
}

Bullet* spawnBullet(Vec2 pos, Vec2 vel, u8 color, u8 damage,
                    u8 flags, u8 aoeRadius, u8 effectDuration, u8 lifetime) {
    Bullet* b = findSlot(gBullets);
    if (!b) return nullptr;
    b->pos = pos;
    b->vel = vel;
    b->color = color;
    b->damage = damage;
    b->flags = flags;
    b->aoeRadius = aoeRadius;
    b->effectDuration = effectDuration;
    b->lifetime = lifetime;
    b->visualType = BVIS_NORMAL;  // caller may override immediately after
    b->active = true;
    return b;
}

Enemy* spawnEnemy(Vec2 pos, Vec2 vel, s16 hp, u8 type,
                  u8 sizeClass, u8 spriteSize,
                  u8 swarmId, u8 swarmIndex) {
    Enemy* e = findSlot(gEnemies);
    if (!e) return nullptr;
    e->pos        = pos;
    e->vel        = vel;
    e->hp         = hp;
    e->maxHp      = hp;
    e->type       = type;
    e->sizeClass  = sizeClass;
    e->spriteSize = spriteSize;
    e->frame      = 0;
    e->aiState    = 0;
    e->shootTimer = 0;
    e->swarmId    = swarmId;
    e->swarmIndex = swarmIndex;
    e->slowTimer  = 0;
    e->freezeTimer = 0;
    e->fearTimer  = 0;
    e->goldValue  = 0;  // set by caller or wave spawner
    e->active     = true;
    return e;
}

Swarm* spawnSwarm(Vec2 leaderPos, Vec2 targetPos, u8 pattern) {
    Swarm* s = findSlot(gSwarms);
    if (!s) return nullptr;
    s->leaderPos   = leaderPos;
    s->targetPos   = targetPos;
    s->pattern     = pattern;
    s->memberCount = 0;
    s->aliveCount  = 0;
    s->active      = true;
    return s;
}

GoldDrop* spawnGold(Vec2 pos, u8 value) {
    GoldDrop* g = findSlot(gGold);
    if (!g) return nullptr;
    g->pos = pos;
    g->value = value;
    g->lifetime = 255;
    g->active = true;
    return g;
}

HealOrb* spawnHealOrb(Vec2 pos, u8 healAmount) {
    HealOrb* h = findSlot(gHealOrbs);
    if (!h) return nullptr;
    h->pos        = pos;
    h->healAmount = healAmount;
    h->lifetime   = 240;  // ~4 seconds at 60fps
    h->active     = true;
    return h;
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

Zone* spawnZone(Vec2 pos, u8 radius, u8 damage, u8 tickInterval,
                u8 lifetime, u8 color, u8 type) {
    Zone* z = findSlot(gZones);
    if (!z) return nullptr;
    z->pos          = pos;
    z->radius       = radius;
    z->damage       = damage;
    z->tickInterval = tickInterval;
    z->tickTimer    = tickInterval;
    z->lifetime     = lifetime;
    z->color        = color;
    z->type         = type;
    z->active       = true;
    return z;
}

void spawnParticleBurst(Vec2 center, int count, u8 life, u8 color) {
    for (int i = 0; i < count; ++i) {
        Vec2 vel;
        vel.x = static_cast<Fixed>((rngRange(32) - 16));
        vel.y = static_cast<Fixed>((rngRange(32) - 16));
        spawnParticle(center, vel, life, color);
    }
}
