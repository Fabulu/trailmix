#include "player.h"
#include "entities.h"
#include "audio.h"

DTCM_BSS Player gPlayer;

void playerInit() {
    gPlayer.pos = {toFixed(SCREEN_W / 2), toFixed(SCREEN_H / 2)};
    gPlayer.speed = toFixed(2);
    gPlayer.hp = 10;
    gPlayer.maxHp = 10;
    gPlayer.shootTimer = 0;
    gPlayer.shootCooldown = 10;
    gPlayer.bulletDamage = 1;

    for (int i = 0; i < PILL_COLOR_COUNT; ++i) {
        gPlayer.inventory[i].count = 0;
        gPlayer.inventory[i].tier = PillTier::Normal;
    }
}

void playerUpdate(u32 held) {
    Vec2 move = {0, 0};
    if (held & KEY_UP)    move.y = static_cast<Fixed>(-gPlayer.speed);
    if (held & KEY_DOWN)  move.y = gPlayer.speed;
    if (held & KEY_LEFT)  move.x = static_cast<Fixed>(-gPlayer.speed);
    if (held & KEY_RIGHT) move.x = gPlayer.speed;

    gPlayer.pos += move;

    // Clamp to screen bounds
    Fixed minX = toFixed(PLAYER_SIZE / 2);
    Fixed maxX = toFixed(SCREEN_W - PLAYER_SIZE / 2);
    Fixed minY = toFixed(PLAYER_SIZE / 2);
    Fixed maxY = toFixed(SCREEN_H - PLAYER_SIZE / 2);

    if (gPlayer.pos.x < minX) gPlayer.pos.x = minX;
    if (gPlayer.pos.x > maxX) gPlayer.pos.x = maxX;
    if (gPlayer.pos.y < minY) gPlayer.pos.y = minY;
    if (gPlayer.pos.y > maxY) gPlayer.pos.y = maxY;

    // Auto-shoot cooldown
    if (gPlayer.shootTimer > 0) {
        --gPlayer.shootTimer;
    }
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

bool playerCollectPill(PillColor color, PillTier tier) {
    int idx = static_cast<int>(color);
    PillSlot& slot = gPlayer.inventory[idx];

    // Only collect if tier matches current slot tier
    if (tier != slot.tier) return false;

    // At max tier and already capped — no more to collect
    if (slot.tier == PillTier::Mega && slot.count >= MERGE_THRESHOLD) {
        return false;
    }

    slot.count++;
    if (slot.count >= MERGE_THRESHOLD) {
        if (slot.tier < PillTier::Mega) {
            // Merge up to next tier, reset count
            slot.count = 0;
            slot.tier = static_cast<PillTier>(static_cast<u8>(slot.tier) + 1);
            audioPlaySfx(SFX_MERGE);
            return true;
        } else {
            // 3 Megas collected — max power reached, keep count at max
            audioPlaySfx(SFX_MERGE);
            return true;
        }
    } else {
        audioPlaySfx(SFX_PICKUP);
    }
    return false;
}

int playerSynergyLevel(PillColor color) {
    int idx = static_cast<int>(color);
    const PillSlot& slot = gPlayer.inventory[idx];
    // Base synergy from tier + count bonus
    return static_cast<int>(slot.tier) * 3 + slot.count;
}
