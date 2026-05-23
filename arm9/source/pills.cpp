#include "pills.h"
#include "player.h"

DTCM_DATA static u8 regenCounter = 0;

void pillsReset() {
    regenCounter = 0;
}

void pillsApplySynergies() {
    // Speed synergy (Blue) — modify player speed
    int speedLevel = playerSynergyLevel(PillColor::Blue);
    gPlayer.speed = toFixed(2 + speedLevel / 2);

    // Damage synergy (Red) — applied in combat.cpp via playerSynergyLevel

    // Fire rate synergy (Yellow) — applied in combat.cpp via playerSynergyLevel

    // Regen synergy (Green) — heal over time
    int regenLevel = playerSynergyLevel(PillColor::Green);
    if (regenLevel > 0) {
        regenCounter++;
        // Heal 1 HP every (600 / regenLevel) frames (~10s at level 1, ~5s at level 2...)
        u8 interval = static_cast<u8>(600 / regenLevel);
        if (interval < 30) interval = 30;

        if (regenCounter >= interval) {
            regenCounter = 0;
            if (gPlayer.hp < gPlayer.maxHp) {
                gPlayer.hp++;
            }
        }
    }
}

const char* pillTierName(PillTier tier) {
    switch (tier) {
        case PillTier::Normal: return "Normal";
        case PillTier::Super:  return "Super";
        case PillTier::Ultra:  return "Ultra";
        case PillTier::Mega:   return "MEGA";
    }
    return "???";
}
