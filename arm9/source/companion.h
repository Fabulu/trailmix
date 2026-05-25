#ifndef COMPANION_H
#define COMPANION_H

#include "types.h"

constexpr int MAX_COMPANIONS = 6;  // 5 base + 1 from Pack Rat perk
constexpr int TRAIL_HIST_SIZE = 64;
constexpr int TRAIL_SPACING = 5;   // frames of offset — tighter formation for dodging

struct Companion {
    Vec2 pos;
    union {
        PillColor color;
        PillColor baseColor;  // alias for shop display
    };
    u8 classId;     // 0-5 within color (index into 6 classes per color)
    u8 tier;        // 0=T1, 1=T2, 2=T3
    u8 shootTimer;
    bool active;

    // --- Combat state ---
    s16 hp;
    s16 maxHp;
    u8  iframes;        // invincibility frames remaining (>0 = iframes active)
    u8  hitFlashTimer;  // frames to render white hit flash (counts down from 3)
    u8  healFlashTimer; // frames to render green heal particles (counts down)
    u8  dmgBuffTimer;   // frames of 20% damage buff remaining (from Lifeshaper pulse)
};

struct ClassDef {
    u8 cooldown;
    u8 damage;
    u8 bulletCount;    // bullets per volley
    u8 spreadAngle;    // degrees of spread (0=straight, 30=shotgun)
    u8 flags;          // BFLAG_PIERCE, etc
    u8 aoeRadius;
    u8 effectDuration; // for slow/freeze
    u8 rarity;         // 0=Common, 1=Uncommon, 2=Rare
};

extern const ClassDef kClassDefs[36];

enum PassiveType : u8 {
    PASSIVE_NONE = 0,
    PASSIVE_TEAM_DAMAGE,     // +param1% damage to all
    PASSIVE_TEAM_SPEED,      // +param1 subpixels speed
    PASSIVE_TEAM_COOLDOWN,   // -param1% cooldown
    PASSIVE_REGEN,           // heal param1 HP every param2 frames (0=on gold pickup)
    PASSIVE_ENEMY_SLOW_AURA, // slow enemies within param1 px by param2%
    PASSIVE_GOLD_BONUS,      // +param1% gold
    PASSIVE_PICKUP_RANGE,    // +param1 px pickup radius
    PASSIVE_DEBUFF_EXTEND,   // debuffs +param1% duration
    PASSIVE_CLOSE_DAMAGE,    // +param1% damage within param2 px
    PASSIVE_FORMATION_TIGHT, // companions follow param1 frames closer
    PASSIVE_MAX_HP,          // player +param1 max HP (on acquire, not per frame)
    PASSIVE_REROLL_DISCOUNT, // reroll costs param1 less
    PASSIVE_EXTRA_SHOP,      // shop +param1 cards
};

struct PassiveDef {
    PassiveType type;
    u8 param1;
    u16 param2;
};

extern const PassiveDef kPassiveDefs[36];

struct PassiveState {
    u8 damageBoostPct;      // total % damage boost for player
    u8 companionDmgBoostPct;// 50% of above for companions
    u8 speedBonus;          // added to base speed (subpixels)
    u8 cooldownReducePct;   // total % cooldown reduction
    u8 goldBonusPct;        // total % gold bonus
    u8 pickupRangeBonus;    // extra px for gold pickup
    u8 rerollDiscount;      // gold discount on reroll
    u8 extraShopCards;      // additional shop cards
    u8 closeDmgBoostPct;    // +% damage when enemy within closeDmgRange px
    u8 closeDmgRange;       // range in px for close damage bonus
    u8 debuffExtendPct;     // +% duration on slow/freeze effects
};

extern PassiveState gPassive;

void companionApplyPassives();

extern Companion gCompanions[MAX_COMPANIONS];
extern int gCompanionCount;

void companionInit();
void companionUpdate();
Companion* companionSpawn(PillColor color, u8 classId, u8 tier);
void companionRemove(int index);
int companionCount();

// Buy a companion from the shop (spawns at tier 0)
inline Companion* companionBuy(u8 classId, PillColor color) {
    return companionSpawn(color, classId, 0);
}

// Get the full class index (0-35) from color + classId
inline int companionFullClassId(const Companion& c) {
    return static_cast<int>(c.color) * 6 + c.classId;
}

// Check and perform auto-merge: 3 same classId+tier → 1 at tier+1
// Returns true if a merge happened
bool companionCheckMerge();

// Visual event triggers — call from combat/game logic as events occur
void companionOnDamage(int index, s16 dmg);   // apply damage, set hitFlashTimer + iframes
void companionOnHeal(int index, s16 amount);  // apply heal, set healFlashTimer
void companionOnDeath(int index);             // hide sprite, burst particles, play GSFX_EXPLODE
void companionOnWaveHeal(int index);          // green pulse (wave-start / shop-entry heal)

#endif
