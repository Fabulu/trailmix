// Tests for entity pool allocation, slot exhaustion, and clear.
#include "../arm9/source/types.h"
#include "../arm9/source/rng.h"
#include "../arm9/source/entities.h"

// Pull in implementations directly
#include "../arm9/source/rng.cpp"
#include "../arm9/source/entities.cpp"

#include "test_framework.h"

TEST(entities_init_all_inactive) {
    entitiesInit();
    for (int i = 0; i < MAX_BULLETS; ++i)   ASSERT(!gBullets[i].active);
    for (int i = 0; i < MAX_ENEMIES; ++i)   ASSERT(!gEnemies[i].active);
    for (int i = 0; i < MAX_PILLS; ++i)     ASSERT(!gPills[i].active);
    for (int i = 0; i < MAX_PARTICLES; ++i) ASSERT(!gParticles[i].active);
}

TEST(spawnBullet_basic) {
    entitiesInit();
    Vec2 pos = {toFixed(10), toFixed(20)};
    Vec2 vel = {toFixed(1), toFixed(0)};
    Bullet* b = spawnBullet(pos, vel, 0, 5);
    ASSERT(b != nullptr);
    ASSERT(b->active);
    ASSERT_EQ(b->damage, 5);
    ASSERT_EQ(toInt(b->pos.x), 10);
}

TEST(spawnBullet_exhaustion) {
    entitiesInit();
    Vec2 pos = {0, 0};
    Vec2 vel = {0, 0};
    for (int i = 0; i < MAX_BULLETS; ++i) {
        ASSERT(spawnBullet(pos, vel, 0, 1) != nullptr);
    }
    // Pool is full — next spawn must return nullptr
    ASSERT(spawnBullet(pos, vel, 0, 1) == nullptr);
}

TEST(spawnEnemy_basic) {
    entitiesInit();
    Vec2 pos = {toFixed(50), toFixed(60)};
    Vec2 vel = {0, toFixed(1)};
    Enemy* e = spawnEnemy(pos, vel, 10, 2, 8);
    ASSERT(e != nullptr);
    ASSERT(e->active);
    ASSERT_EQ(e->hp, 10);
    ASSERT_EQ(e->maxHp, 10);
    ASSERT_EQ(e->type, 2);
    ASSERT_EQ(e->size, 8);
    ASSERT_EQ(e->frame, 0);
}

TEST(spawnEnemy_exhaustion) {
    entitiesInit();
    Vec2 p = {0, 0}, v = {0, 0};
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        ASSERT(spawnEnemy(p, v, 1, 0, 4) != nullptr);
    }
    ASSERT(spawnEnemy(p, v, 1, 0, 4) == nullptr);
}

TEST(spawnPill_basic) {
    entitiesInit();
    Vec2 pos = {toFixed(30), toFixed(40)};
    PillDrop* p = spawnPill(pos, PillColor::Red, PillTier::Normal);
    ASSERT(p != nullptr);
    ASSERT(p->active);
    ASSERT_EQ(static_cast<int>(p->color), static_cast<int>(PillColor::Red));
    ASSERT_EQ(p->lifetime, 255);
}

TEST(entitiesClear_resets_all) {
    entitiesInit();
    // Fill up some slots
    Vec2 z = {0, 0};
    spawnBullet(z, z, 0, 1);
    spawnEnemy(z, z, 1, 0, 4);
    spawnPill(z, PillColor::Green, PillTier::Super);
    spawnParticle(z, z, 10, 1);

    entitiesClear();
    for (int i = 0; i < MAX_BULLETS; ++i)   ASSERT(!gBullets[i].active);
    for (int i = 0; i < MAX_ENEMIES; ++i)   ASSERT(!gEnemies[i].active);
    for (int i = 0; i < MAX_PILLS; ++i)     ASSERT(!gPills[i].active);
    for (int i = 0; i < MAX_PARTICLES; ++i) ASSERT(!gParticles[i].active);
}

TEST(spawnBullet_reuse_slot_after_deactivate) {
    entitiesInit();
    Vec2 z = {0, 0};
    // Fill all bullet slots
    for (int i = 0; i < MAX_BULLETS; ++i)
        spawnBullet(z, z, 0, 1);
    ASSERT(spawnBullet(z, z, 0, 1) == nullptr);

    // Deactivate one
    gBullets[5].active = false;
    Bullet* b = spawnBullet(z, z, 0, 99);
    ASSERT(b != nullptr);
    ASSERT_EQ(b->damage, 99);
    ASSERT(b == &gBullets[5]);
}

int main() {
    std::cout << "=== test_entities ===\n";
    return runAllTests();
}
