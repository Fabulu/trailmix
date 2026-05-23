// Tests for pill merge progression and synergy levels.
// player.cpp depends on audio.h, so we stub it out here.
#include "audio_stub.h"
#define AUDIO_H   // prevent real audio.h from being included

#include "../arm9/source/types.h"
#include "../arm9/source/rng.h"
#include "../arm9/source/entities.h"
#include "../arm9/source/player.h"
#include "../arm9/source/pills.h"

// Pull in implementations
#include "../arm9/source/rng.cpp"
#include "../arm9/source/entities.cpp"
#include "../arm9/source/player.cpp"
#include "../arm9/source/pills.cpp"

#include "test_framework.h"

// Helper: reset player for each test
static void resetPlayer() {
    playerInit();
}

// --- Merge progression ---

TEST(pill_merge_normal_to_super) {
    resetPlayer();
    PillColor c = PillColor::Red;
    // Collect 3 Normal pills -> should merge to Super
    ASSERT(!playerCollectPill(c, PillTier::Normal));  // count=1
    ASSERT(!playerCollectPill(c, PillTier::Normal));  // count=2
    ASSERT(playerCollectPill(c, PillTier::Normal));   // count=3 -> merge!

    ASSERT_EQ(static_cast<int>(gPlayer.inventory[0].tier), static_cast<int>(PillTier::Super));
    ASSERT_EQ(gPlayer.inventory[0].count, 0);
}

TEST(pill_merge_super_to_ultra) {
    resetPlayer();
    PillColor c = PillColor::Blue;
    int idx = static_cast<int>(c);

    // First: Normal -> Super
    for (int i = 0; i < MERGE_THRESHOLD; ++i)
        playerCollectPill(c, PillTier::Normal);
    ASSERT_EQ(static_cast<int>(gPlayer.inventory[idx].tier), static_cast<int>(PillTier::Super));

    // Now Super pills
    ASSERT(!playerCollectPill(c, PillTier::Super));
    ASSERT(!playerCollectPill(c, PillTier::Super));
    ASSERT(playerCollectPill(c, PillTier::Super));  // merge!

    ASSERT_EQ(static_cast<int>(gPlayer.inventory[idx].tier), static_cast<int>(PillTier::Ultra));
    ASSERT_EQ(gPlayer.inventory[idx].count, 0);
}

TEST(pill_merge_ultra_to_mega) {
    resetPlayer();
    PillColor c = PillColor::Green;
    int idx = static_cast<int>(c);

    // Normal -> Super -> Ultra
    for (int i = 0; i < MERGE_THRESHOLD; ++i)
        playerCollectPill(c, PillTier::Normal);
    for (int i = 0; i < MERGE_THRESHOLD; ++i)
        playerCollectPill(c, PillTier::Super);

    ASSERT_EQ(static_cast<int>(gPlayer.inventory[idx].tier), static_cast<int>(PillTier::Ultra));

    // Ultra -> Mega
    ASSERT(!playerCollectPill(c, PillTier::Ultra));
    ASSERT(!playerCollectPill(c, PillTier::Ultra));
    ASSERT(playerCollectPill(c, PillTier::Ultra));

    ASSERT_EQ(static_cast<int>(gPlayer.inventory[idx].tier), static_cast<int>(PillTier::Mega));
    ASSERT_EQ(gPlayer.inventory[idx].count, 0);
}

TEST(pill_mega_no_further_merge) {
    resetPlayer();
    PillColor c = PillColor::Yellow;
    int idx = static_cast<int>(c);

    // Get to Mega
    for (int i = 0; i < MERGE_THRESHOLD; ++i)
        playerCollectPill(c, PillTier::Normal);
    for (int i = 0; i < MERGE_THRESHOLD; ++i)
        playerCollectPill(c, PillTier::Super);
    for (int i = 0; i < MERGE_THRESHOLD; ++i)
        playerCollectPill(c, PillTier::Ultra);

    ASSERT_EQ(static_cast<int>(gPlayer.inventory[idx].tier), static_cast<int>(PillTier::Mega));

    // Collecting more Mega pills should increment count but not merge further
    playerCollectPill(c, PillTier::Mega);
    playerCollectPill(c, PillTier::Mega);
    ASSERT_EQ(gPlayer.inventory[idx].count, 2);
    ASSERT_EQ(static_cast<int>(gPlayer.inventory[idx].tier), static_cast<int>(PillTier::Mega));
}

TEST(pill_count_resets_on_merge) {
    resetPlayer();
    PillColor c = PillColor::Red;
    playerCollectPill(c, PillTier::Normal);
    playerCollectPill(c, PillTier::Normal);
    ASSERT_EQ(gPlayer.inventory[0].count, 2);

    playerCollectPill(c, PillTier::Normal);  // merge
    ASSERT_EQ(gPlayer.inventory[0].count, 0);
}

TEST(pill_tier_mismatch_rejected) {
    resetPlayer();
    PillColor c = PillColor::Red;
    // Slot starts at Normal tier; collecting a Super pill should fail
    bool collected = playerCollectPill(c, PillTier::Super);
    ASSERT(!collected);
    ASSERT_EQ(gPlayer.inventory[0].count, 0);
    ASSERT_EQ(static_cast<int>(gPlayer.inventory[0].tier), static_cast<int>(PillTier::Normal));
}

TEST(pill_different_colors_independent) {
    resetPlayer();
    // Collect 2 Red Normals and 1 Blue Normal
    playerCollectPill(PillColor::Red, PillTier::Normal);
    playerCollectPill(PillColor::Red, PillTier::Normal);
    playerCollectPill(PillColor::Blue, PillTier::Normal);

    ASSERT_EQ(gPlayer.inventory[0].count, 2);  // Red
    ASSERT_EQ(gPlayer.inventory[1].count, 1);  // Blue
}

// --- Synergy level ---

TEST(synergy_level_calculation) {
    resetPlayer();
    // At Normal tier with 0 count: level = 0*3 + 0 = 0
    ASSERT_EQ(playerSynergyLevel(PillColor::Red), 0);

    playerCollectPill(PillColor::Red, PillTier::Normal);
    // Normal tier, count=1: level = 0*3 + 1 = 1
    ASSERT_EQ(playerSynergyLevel(PillColor::Red), 1);

    playerCollectPill(PillColor::Red, PillTier::Normal);
    // Normal tier, count=2: level = 0*3 + 2 = 2
    ASSERT_EQ(playerSynergyLevel(PillColor::Red), 2);

    playerCollectPill(PillColor::Red, PillTier::Normal);
    // Merged to Super, count=0: level = 1*3 + 0 = 3
    ASSERT_EQ(playerSynergyLevel(PillColor::Red), 3);
}

// --- Pill tier name ---

TEST(pillTierName_values) {
    ASSERT(std::string(pillTierName(PillTier::Normal)) == "Normal");
    ASSERT(std::string(pillTierName(PillTier::Super))  == "Super");
    ASSERT(std::string(pillTierName(PillTier::Ultra))  == "Ultra");
    ASSERT(std::string(pillTierName(PillTier::Mega))   == "MEGA");
}

int main() {
    std::cout << "=== test_pills ===\n";
    return runAllTests();
}
