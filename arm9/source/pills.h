#ifndef PILLS_H
#define PILLS_H

#include "types.h"

// Apply synergy effects from the player's pill inventory to game stats.
// Called once per frame during gameplay.
void pillsReset();
void pillsApplySynergies();

// Get a human-readable tier name
const char* pillTierName(PillTier tier);

#endif // PILLS_H
