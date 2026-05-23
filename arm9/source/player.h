#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"

constexpr int PLAYER_SIZE = 12;      // pixels
constexpr int INVENTORY_SLOTS = 4;   // one per color

struct PillSlot {
    u8 count;       // 0-2 at current tier (3 triggers merge)
    PillTier tier;
};

struct Player {
    Vec2 pos;
    Fixed speed;
    s16 hp;
    s16 maxHp;
    u8 shootTimer;
    u8 shootCooldown;
    u8 bulletDamage;
    PillSlot inventory[PILL_COLOR_COUNT];
};

extern Player gPlayer;

void playerInit();
void playerUpdate(u32 keysHeld);
Rect playerRect();

// Returns true if a merge happened
bool playerCollectPill(PillColor color, PillTier tier);

// Get synergy multiplier for a color (based on pill tier held)
int playerSynergyLevel(PillColor color);

#endif // PLAYER_H
