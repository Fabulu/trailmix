#ifndef ENTITIES_H
#define ENTITIES_H

#include "types.h"

constexpr int MAX_BULLETS   = 64;
constexpr int MAX_ENEMIES   = 32;
constexpr int MAX_GOLD      = 24;
constexpr int MAX_SWARMS    = 4;
constexpr int MAX_HEAL_ORBS = 8;   // Green3 synergy: healing orbs on the ground
constexpr int MAX_PARTICLES  = 128;
constexpr int MAX_ZONES      = 8;

// Bullet behaviour flags (combinable with |)
constexpr u8 BFLAG_PIERCE  = 1;   // don't deactivate on first hit
constexpr u8 BFLAG_EXPLODE = 2;   // AoE damage on impact (uses aoeRadius)
constexpr u8 BFLAG_SLOW    = 4;   // reduce target speed for effectDuration frames
constexpr u8 BFLAG_FREEZE  = 8;   // stop target for effectDuration frames
constexpr u8 BFLAG_ERASE   = 16;  // deactivate enemy bullets on contact
constexpr u8 BFLAG_ZONE    = 32;  // spawn a persistent damage zone on impact instead of exploding

// visualType values for Bullet::visualType
// 0=normal  1=fire  2=ice  3=pierce  4=explosive  5=poison  6=electric  7=heavy
constexpr u8 BVIS_NORMAL    = 0;
constexpr u8 BVIS_FIRE      = 1;
constexpr u8 BVIS_ICE       = 2;
constexpr u8 BVIS_PIERCE    = 3;
constexpr u8 BVIS_EXPLOSIVE = 4;
constexpr u8 BVIS_POISON    = 5;
constexpr u8 BVIS_ELECTRIC  = 6;
constexpr u8 BVIS_HEAVY     = 7;
constexpr u8 BVIS_NORMAL_T2 = 8;  // 4x4 white-yellow, T2 companion bullets
constexpr u8 BVIS_NORMAL_T3 = 9;  // 5x5 white-yellow + trailing glow pixel, T3 companion bullets
constexpr u8 BVIS_ENEMY     = 10; // 4x4 bright red with trailing red pixel, enemy projectiles

// Dedicated color value for enemy-fired bullets (distinct from player=255 and pill colors 0-5)
constexpr u8 BULLET_COLOR_ENEMY = 200;

struct Bullet {
    Vec2 pos;
    Vec2 vel;
    u8 color;
    u8 damage;
    u8 flags;           // combination of BFLAG_* constants; 0 = plain bullet
    u8 aoeRadius;       // pixels; 0 = no AoE, >0 = explosion radius
    u8 effectDuration;  // frames for SLOW / FREEZE; ignored otherwise
    u8 lifetime;        // 0 = unlimited; >0 = deactivate after this many frames
    u8 visualType;      // BVIS_* constant --- controls appearance in renderGameplay
    bool active;
};

// Enemy type constants (0-11 = regular, 12-15 = bosses)
constexpr u8 ETYPE_GRUNT          =  0;  // slow marcher
constexpr u8 ETYPE_CHARGER        =  1;  // fast flanker
constexpr u8 ETYPE_BRUTE          =  2;  // heavy tank
constexpr u8 ETYPE_SPITTER        =  3;  // ranged kiter
constexpr u8 ETYPE_SNIPER         =  4;  // long-range accurate shot
constexpr u8 ETYPE_SPLITTER       =  5;  // splits into 2 small Grunts on death
constexpr u8 ETYPE_SHIELD         =  6;  // blocks frontal shots, must be flanked
constexpr u8 ETYPE_ARTILLERY      =  7;  // stationary, lobs explosive shells
constexpr u8 ETYPE_NIGHTMARE      =  8;  // fear pulse; scatters nearby enemies
constexpr u8 ETYPE_GHOST          =  9;  // teleports near player every 2s
constexpr u8 ETYPE_SWARM_DRONE    = 10;  // very fast, used in swarm patterns
constexpr u8 ETYPE_BOMBER         = 11;  // charges in, contact triggers AoE explosion

constexpr u8 ETYPE_MEDIC         = 12;  // heals nearby allies, flees player
constexpr u8 ETYPE_ANCHOR        = 13;  // damage-resist aura to nearby enemies
constexpr u8 ETYPE_TRAPPER       = 14;  // plants slowing mines
constexpr u8 ETYPE_HEXER         = 15;  // fires debuff projectile (slow fire rate)
constexpr u8 ETYPE_HIVE          = 16;  // stationary, spawns drones

constexpr u8 ETYPE_BOSS_SENTINEL     = 17;  // 4-way rotating turret + adds
constexpr u8 ETYPE_BOSS_DREADNOUGHT  = 18;  // 8-way shockwave slam
constexpr u8 ETYPE_BOSS_LEVIATHAN    = 19;  // summoner + charge
constexpr u8 ETYPE_BOSS_NIGHTMARE_B  = 20;  // circles player + mass fear pulse
constexpr u8 ETYPE_BOSS_APOTHECARY   = 21;  // wave 30 final boss, 3 phases

constexpr u8 ETYPE_COUNT             = 22;  // total enemy types

// Size class constants
constexpr u8 SIZE_SMALL  = 0;  // spriteSize = 8
constexpr u8 SIZE_MEDIUM = 1;  // spriteSize = 16
constexpr u8 SIZE_LARGE  = 2;  // spriteSize = 24

// Pixel sizes corresponding to each size class
constexpr u8 SPRITE_SIZE_SMALL  =  8;
constexpr u8 SPRITE_SIZE_MEDIUM = 16;
constexpr u8 SPRITE_SIZE_LARGE  = 24;
constexpr u8 SPRITE_SIZE_BOSS   = 32;
constexpr u8 SPRITE_SIZE_BOSS_LARGE = 64;

// sizeClass: 0=small, 1=medium, 2=large
// spriteSize: actual pixel dimension (8,16,24,32,48,64)
// swarmId: 0xFF = not in a swarm

struct Enemy {
    Vec2 pos;
    Vec2 vel;
    s16  hp;
    s16  maxHp;
    u8   type;         // 0-16 regular, 17+ bosses
    u8   sizeClass;    // 0=small, 1=medium, 2=large
    u8   spriteSize;   // pixel size: 8,16,24,32,48,64
    u8   frame;
    u8   aiState;      // per-type AI state machine
    u8   shootTimer;   // countdown for ranged attacks
    u8   swarmId;      // swarm group index, 0xFF = none
    u8   swarmIndex;   // position within the swarm
    u8   slowTimer;    // frames of reduced speed remaining
    u8   freezeTimer;  // frames of full stop remaining
    u8   fearTimer;    // frames of fleeing (Nightmare fear pulse)
    u8   hurtTimer;    // frames of hit flash remaining (per-type visual)
    u8   goldValue;    // gold dropped on death
    bool active;
};

// Swarm formation patterns
constexpr u8 SWARM_FISH   = 0;  // tight schooling cluster
constexpr u8 SWARM_PINCER = 1;  // split and flank from both sides
constexpr u8 SWARM_SPIRAL = 2;  // rotating spiral approach
constexpr u8 SWARM_WAVE   = 3;  // sine-wave formation
constexpr u8 SWARM_SCATTER = 4; // disperse on proximity

struct Swarm {
    Vec2 leaderPos;   // current center/leader position
    Vec2 targetPos;   // where the swarm is heading
    u8   pattern;     // SWARM_* constant
    u8   memberCount; // total members assigned at spawn
    u8   aliveCount;  // living members remaining
    bool active;
};

struct GoldDrop {
    Vec2 pos;
    u8 value;
    u8 lifetime;  // frames until despawn (255 = ~4.25s)
    bool active;
};

// Green3 synergy: green healing orbs dropped on the ground — heal player on pickup
struct HealOrb {
    Vec2 pos;
    u8   healAmount;  // HP to restore on pickup
    u8   lifetime;    // frames until despawn
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

// Zone types
constexpr u8 ZONE_POISON = 0;  // poison cloud: green particles, periodic DoT
constexpr u8 ZONE_BURN   = 1;  // burn zone:   red/orange particles, higher damage

struct Zone {
    Vec2 pos;
    u8   radius;       // pixels
    u8   damage;       // hp per tick
    u8   tickInterval; // frames between damage applications
    u8   tickTimer;    // frames until next damage tick
    u8   lifetime;     // frames until zone expires
    u8   color;        // particle color index
    u8   type;         // ZONE_POISON or ZONE_BURN
    bool active;
};

// Global pools
extern Bullet   gBullets[MAX_BULLETS];
extern Enemy    gEnemies[MAX_ENEMIES];
extern GoldDrop gGold[MAX_GOLD];
extern Swarm    gSwarms[MAX_SWARMS];
extern HealOrb  gHealOrbs[MAX_HEAL_ORBS];
extern Particle gParticles[MAX_PARTICLES];
extern Zone     gZones[MAX_ZONES];

void entitiesInit();
void entitiesClear();

// flags/aoeRadius/effectDuration/lifetime all default to 0 for plain bullets
Bullet*   spawnBullet(Vec2 pos, Vec2 vel, u8 color, u8 damage,
                      u8 flags = 0, u8 aoeRadius = 0,
                      u8 effectDuration = 0, u8 lifetime = 0);
// sizeClass/spriteSize/swarmId default to small/16px/unaffiliated
Enemy*    spawnEnemy(Vec2 pos, Vec2 vel, s16 hp, u8 type,
                    u8 sizeClass = 0, u8 spriteSize = 16,
                    u8 swarmId = 0xFF, u8 swarmIndex = 0);
Swarm*    spawnSwarm(Vec2 leaderPos, Vec2 targetPos, u8 pattern);
GoldDrop* spawnGold(Vec2 pos, u8 value);
HealOrb*  spawnHealOrb(Vec2 pos, u8 healAmount);
Particle* spawnParticle(Vec2 pos, Vec2 vel, u8 life, u8 color);

// Spawn a burst of particles (for explosions, merges)
void spawnParticleBurst(Vec2 center, int count, u8 life, u8 color);

// Spawn a damage zone (poison cloud or burn area).
// tickInterval: frames between damage ticks (e.g. 30 = twice/sec at 60fps)
// lifetime: total frames the zone persists
Zone* spawnZone(Vec2 pos, u8 radius, u8 damage, u8 tickInterval,
                u8 lifetime, u8 color, u8 type);

#endif // ENTITIES_H
