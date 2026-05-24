# Gameplay Overhaul: Implementation Plan

## Phase 1 — Scrolling Arena + Wave-Clear (Foundation)

**Goal:** Player moves in a 512x384 arena with camera. Waves end when all enemies die. No more timed spawning.

**New files:**
- `camera.h/cpp` — `Vec2 gCamera; void cameraInit(); void cameraUpdate(); int camX(); int camY();`
  - `cameraUpdate()`: `gCamera.x = clamp(player.x - toFixed(128), 0, toFixed(256))`, same for y with 96/192.
  - `camX()`/`camY()` return `toInt(gCamera.x/y)` for render offset.

**Modify `types.h`:**
- Add `constexpr int ARENA_W = 512; constexpr int ARENA_H = 384; constexpr int WALL_THICK = 16;`
- Remove `PillTier`, `PILL_TIER_COUNT`, `MERGE_THRESHOLD`.

**Modify `player.h/cpp`:**
- Remove `PillSlot`, `inventory[4]`, `playerCollectPill()`, `playerSynergyLevel()`.
- Add `u16 gold;` to Player. Clamp movement to arena bounds (WALL_THICK to ARENA_W/H - WALL_THICK - PLAYER_SIZE).
- Remove `shootTimer`/`shootCooldown`/`bulletDamage` (companions shoot, not player).
- Add dash system: `u8 dashTimer; u8 dashCooldown; Vec2 dashDir; bool isDashing; bool dashInvincible;`
- R button triggers dash: 8-frame burst at 4x speed in current move direction. Player is invincible during dash.
- Dash clamped to arena walls (cannot dash through boundary).
- Cooldown: 90 frames (~1.5s). `dashCooldown` counts down each frame when not dashing.
- Visual effect: spawn trail particles every frame during dash (player's color, 2px, 10-frame life).
- On dash start: 3-frame screen shake (camera offset ±2px random). Afterimage effect: render a faded copy of player at previous position each dash frame.

**Modify `render.cpp`:**
- Every `renderPixel`/`renderFilledRect`/`renderFilledCircle` call subtracts `camX()`, `camY()`.
- Add `renderArenaFloor()` — draw visible portion of grid pattern.
- Add `renderArenaWalls()` — draw 16px borders where visible.
- Remove `tierGlow()`.

**Modify `game.cpp`:**
- Replace timed wave spawning with wave-clear detection: `bool waveCleared()` checks all enemies inactive.
- `spawnWave()`: count = `4 + wave*2` capped at 30; spawn at arena edges (not screen edges). Enemy HP = `base * (1 + (wave-1) * 0.18)` via fixed-point.
- State: `Upgrade` renamed to `Shop` (Phase 3 hook). For now, wave-clear just starts next wave.

**Modify `combat.cpp`:**
- `updateBullets()`: despawn when outside arena bounds, not screen bounds.
- `updateEnemies()`: same arena-bounds check. Remove off-screen culling that uses SCREEN_W/H.
- Remove `autoShoot()` (moves to companion in Phase 2).
- Remove `updatePillDrops()`, `checkPlayerPillCollisions()`.

**Modify `entities.h/cpp`:**
- Remove `PillDrop`, `gPills[]`, `spawnPill()`.
- Add `GoldDrop { Vec2 pos; u8 value; u8 lifetime; bool active; }`, pool of 16.
- `spawnGold(Vec2 pos, u8 value)`.
- Increase `MAX_PARTICLES` from 128 to 192.

**Delete:** `pills.h`, `pills.cpp`.

**Tests:** Delete `test_pills.cpp`. Update `test_entities.cpp` and `test_types.cpp` to remove PillTier/PillDrop references.

---

## Phase 2 — Companions + Shooting

**New files:**
- `companion.h/cpp`

**Companion struct:**
```
struct Companion {
    Vec2 pos;
    u8 classId;    // 0-5: Gunner,Shotgun,Sniper,Zapper,Bomber,Medic
    PillColor color;
    u8 shootTimer;
    u8 level;      // future use
    bool active;
    Vec2 history[8]; // ring buffer for trail follow
    u8 histIdx;
};
constexpr int MAX_COMPANIONS = 5;
extern Companion gCompanions[MAX_COMPANIONS];
```

**Functions:** `companionInit()`, `companionUpdate()`, `companionShoot(Companion&)`. Each companion follows its predecessor's position 8 frames ago via ring buffer. Movement: `pos += (target - pos) >> 2`. First companion follows player.

**`companionShoot()`:** `findNearestEnemy()` (moved from combat.cpp, now takes a position arg). Per-class behavior:
- Gunner: single bullet, 10-frame cooldown
- Shotgun: 3 bullets in 30-deg spread, 30-frame cooldown
- Sniper: 1 high-damage bullet, 45-frame cooldown
- Zapper/Bomber/Medic: Phase 4 polish

**Modify `combat.cpp`:** Remove old `autoShoot()`. Add `companionUpdate()` call. Gold drops replace pill drops on enemy death.

**Modify `render.cpp`:** Add `renderCompanions()` — draw each as colored circle with class icon.

---

## Phase 3 — Economy + Shop UI

**New files:**
- `economy.h/cpp` — gold value table: `u8 enemyGoldValue(u8 type)` returning 5/7/15/12/80. Pill cost table. Reroll cost function: `10 + 5*rerollCount`.
- `shop.h/cpp` — `ShopState { ShopCard cards[6]; u8 rerollCount; }`. `shopEnter()`, `shopUpdate()`, `shopRender()`, `shopTryBuy(int slot)`.

**ShopCard struct:** `{ u8 classId; PillColor color; u16 cost; bool sold; }`

**Modify `game.h`:** `GameState::Upgrade` becomes `GameState::Shop`.

**Modify `game.cpp`:** On wave-clear, `enterShop()`. Shop calls `shopUpdate()` which reads touch input. Continue button resumes play.

**Modify `ui.cpp`:** Remove `uiRenderUpgradeScreen()`, `uiGenerateChoices()`. Add gold display to HUD. Shop rendering lives in `shop.cpp` (bottom screen, 2x3 grid of 84x54px cards). Inventory bar at bottom shows 5 companion slots.

**Sell mechanic:** Tap owned companion in inventory bar to sell for 40% of buy price.

---

## Phase 4 — Synergies + Polish

**New files:**
- `synergy.h/cpp` — `void synergyRecalc()` scans `gCompanions[]`, counts colors, sets global perk flags. Called on buy/sell.

**Perk effects (global flags checked in combat/companion code):**

Synergy level = count of UNIQUE classes sharing that color among active companions.
Displayed as active badges on the bottom screen at all times.

RED — Carnage
- 2 unique Red: IGNITION. All bullets leave a 12px fire trail that lingers 30 frames and damages enemies passing through it. Fire damage = 25% of bullet damage per frame of contact.
- 3 unique Red: DETONATION. Every enemy death triggers a concussive blast (48px radius, 1.5x that enemy's max HP as damage). Kills chain instantly. Clearing a cluster is one cascading explosion.
- 4+ unique Red: HELLSTORM. All companions simultaneously lock on and dump a full salvo at the nearest enemy every 90 frames regardless of individual cooldowns. Bullets pierce through all targets. Screen flashes white on trigger.

BLUE — Phantom
- 2 unique Blue: REFRACTION. Bullets that miss or overshoot ricochet once off the nearest arena wall and continue. Ricocheted bullets deal full damage.
- 3 unique Blue: TIMESTOP. When the player takes damage, time freezes for all enemies and bullets for 90 frames (1.5s). Player can move and shoot freely during freeze. 600-frame cooldown (10s).
- 4+ unique Blue: ECHO FORM. Player gains a semi-transparent ghost clone that mirrors all movement and fires simultaneously. Clone bullets deal 60% damage. Clone vanishes if player is hit while it is active; reforms after 5s.

GREEN — Bloom
- 2 unique Green: LIFESTEAL. Each bullet hit heals the player for 1 HP. Companions heal player 2 HP per kill.
- 3 unique Green: SPOREFIELD. Killed enemies have a 50% chance to drop a healing orb (8px green circle). Collecting it instantly restores 5 HP. Orbs persist for 600 frames before vanishing.
- 4+ unique Green: OVERGROWTH. Player and companions gain a regenerating shield equal to 30% of max HP. Shield recharges fully 5s after last damage taken. Shield pulses green when active, flickers when breaking.

YELLOW — Fortune
- 2 unique Yellow: INTEREST. At the start of each shop phase, earn bonus gold equal to 10% of your current gold (rounded up, min 2). Stacks with gold drops.
- 3 unique Yellow: PHANTOM PAYROLL. Every 300 frames (5s) during a wave, a free disposable Gunner companion spawns beside the player, fires for 10s, then vanishes. It counts for no synergy slot and costs nothing.
- 4+ unique Yellow: JACKPOT. Every enemy kill has a 20% chance to drop a bonus gold coin worth 3x normal value. On wave-clear, all unsold shop cards are refunded at 100% cost (not 40%). Every fifth kill in a row without taking damage drops a Loot Chest worth 25 gold.

**Modify `audio.h`:** Add `SFX_BUY`, `SFX_SELL`, `SFX_REROLL`, `SFX_WAVE_CLEAR`, `SFX_SYNERGY`. Remove `SFX_MERGE`.

**Boss behavior (every 5th wave):** Hexagon enemy, 3 phases — phase transitions at 66% and 33% HP. Phase 1: chase. Phase 2: chase + bullet ring. Phase 3: fast chase + rapid fire.

---

## File Change Summary

| File | Action |
|------|--------|
| `types.h` | Remove PillTier; add arena constants |
| `player.h/cpp` | Remove inventory/shooting; add gold |
| `game.h/cpp` | Shop state; wave-clear detection; arena spawning |
| `entities.h/cpp` | Remove PillDrop; add GoldDrop; bump particle cap |
| `combat.h/cpp` | Remove autoShoot/pill logic; add gold drops; arena bounds |
| `render.h/cpp` | Camera offset everywhere; arena floor/walls; companions |
| `ui.h/cpp` | Remove upgrade screen; gold HUD |
| `audio.h` | New SFX IDs |
| `pills.h/cpp` | DELETE |
| `camera.h/cpp` | NEW |
| `companion.h/cpp` | NEW |
| `shop.h/cpp` | NEW |
| `economy.h/cpp` | NEW |
| `synergy.h/cpp` | NEW |
| `test_pills.cpp` | DELETE |
| `test_entities.cpp` | Update for GoldDrop |
| `test_types.cpp` | Remove PillTier tests |

---

## Risk Areas

1. **Fixed-point overflow in arena coords.** 512 pixels = toFixed(512) = 8192, which fits in s16 (max 32767). But velocity math with `fixMul` can overflow s16 intermediates. Audit every multiply — may need s32 temps.
2. **Companion trail jitter.** Ring buffer indexing off-by-one will cause teleporting. Unit test the ring buffer logic in isolation before integrating.
3. **Camera + collision mismatch.** All collision must use world-space coords; only rendering subtracts camera. A single missed offset will cause phantom hits. Grep for `pixelX()` in collision code to verify no camera subtraction crept in.
4. **Bottom-screen touch in shop.** Touch coords are absolute (0-255, 0-191). Must map to card grid correctly. Test with manual tap coordinates before relying on hardware.
5. **Dash through walls.** Dash moves player fast — must clamp position to arena bounds every frame of the dash, not just at the end. If clamping is only checked post-dash, player can clip through walls.
6. **Enemy count scaling.** 30 enemies with 5 companions shooting = up to ~30 bullets active. With 192 particles, peak entity count is ~260. Should fit in frame budget but profile after Phase 2.
