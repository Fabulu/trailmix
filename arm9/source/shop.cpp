#include "shop.h"
#include "render.h"
#include "player.h"
#include "companion.h"
#include "audio.h"
#include "rng.h"
#include "game.h"
#include "strings.h"
#include "synergy.h"
// Forward declarations from perk.h (avoid including directly due to
// PERK_COUNT redefinition conflict with strings.h in the same TU)
enum PerkId : u8 {
    PERK_BULLET_HELL = 0, PERK_OVERCHARGE, PERK_GLASS_CANNON, PERK_CHAIN_LIGHTNING,
    PERK_PHOENIX_DOWN, PERK_FORTRESS, PERK_SHIELD_BASH, PERK_SECOND_WIND,
    PERK_GOLD_FEVER, PERK_WAR_CHEST, PERK_JACKPOT, PERK_WHOLESALE,
    PERK_PACK_RAT, PERK_WARP_DRIVE, PERK_SOUL_SURGE,
    // Trail Mix
    PERK_RICOCHET, PERK_BLOODLUST, PERK_ECHO_CHAMBER,
    PERK_THORNS, PERK_LAST_STAND, PERK_JUGGERNAUT,
    PERK_DOUBLE_OR_NOTHING, PERK_SHORTCUT, PERK_BLACK_MARKET,
    PERK_MAGNET, PERK_PENSION, PERK_LOAN_SHARK,
    PERK_REWIND, PERK_BOUNTY_BOARD, PERK_WILDCARD,
};
bool perkIsActive(PerkId id);
int perkGetRandom();
void perkApplyOnBuy(PerkId id);
int perkMaxCompanions();
static const u16 kPerkPriceShop[30] = {
    60,70,75,65,60,65,55,70,50,65,60,80,55,65,70,
    70,60,85,60,75,90,50,90,65,55,80,50,75,70,100
};
#include <nds.h>
#include <cstdio>

ShopState gShop;

// Dirty flag: only re-render when something changed
static bool shopDirty = true;

// ---------------------------------------------------------------------------
// Layout constants -- all pixel coordinates for the 256x192 sub screen
// ---------------------------------------------------------------------------

// Card grid: 2 rows of 3 cards, 78x38 each
static const int CARD_W      = 78;
static const int CARD_H      = 38;
static const int CARD_X[3]   = { 2, 84, 166 };
static const int CARD_ROW_Y[2] = { 4, 46 };

// Row 3: perk card + reroll
static const int PERK_Y      = 90;
static const int PERK_W      = 120;
static const int PERK_H      = 32;
static const int PERK_X      = 2;

static const int REROLL_X    = 180;
static const int REROLL_Y    = 90;
static const int REROLL_W    = 70;
static const int REROLL_H    = 32;

// Inventory bar
static const int COMP_Y      = 128;

// Heal button
static const int HEAL_Y      = 144;
static const int HEAL_H      = 16;
static const int HEAL_W      = 80;
static const int HEAL_X      = 2;
static const int HEAL_COST   = 8;

// Start wave button
static const int START_Y     = 164;
static const int START_W     = 240;
static const int START_H     = 24;
static const int START_X     = (256 - START_W) / 2;

// ── shopGenerate ──────────────────────────────────────────────────

void shopGenerate(int wave) {
    for (int i = 0; i < SHOP_CARDS; ++i) {
        ShopCard& c = gShop.cards[i];

        // Skip locked cards
        if (c.locked) continue;

        // Roll class using weighted rarity
        // Base: Common=60%, Uncommon=30%, Rare=10%
        // PASSIVE_EXTRA_SHOP shifts to: Common=50%, Uncommon=35%, Rare=15%
        // Black Market: Common=30%, Uncommon=45%, Rare=25%
        int roll = rngRange(20);
        u8 targetRarity;
        if (perkIsActive(PERK_BLACK_MARKET)) {
            // Black Market: 6/20=30% Common, 9/20=45% Uncommon, 5/20=25% Rare
            if (roll < 6)       targetRarity = 0;
            else if (roll < 15) targetRarity = 1;
            else                targetRarity = 2;
        } else if (gPassive.extraShopCards > 0) {
            // Improved odds: 10/20=50% Common, 7/20=35% Uncommon, 3/20=15% Rare
            if (roll < 10)      targetRarity = 0;
            else if (roll < 17) targetRarity = 1;
            else                targetRarity = 2;
        } else {
            // Normal odds: 12/20=60% Common, 6/20=30% Uncommon, 2/20=10% Rare
            if (roll < 12)      targetRarity = 0;
            else if (roll < 18) targetRarity = 1;
            else                targetRarity = 2;
        }

        // Pick a random color
        c.color = static_cast<PillColor>(rngRange(PILL_COLOR_COUNT));

        // Pick a random class of the target rarity within that color
        int colorBase = static_cast<int>(c.color) * 6;
        u8 candidates[6];
        int numCandidates = 0;
        for (int ci = 0; ci < 6; ci++) {
            if (kClassDefs[colorBase + ci].rarity == targetRarity) {
                candidates[numCandidates++] = static_cast<u8>(ci);
            }
        }
        c.classId = candidates[rngRange(numCandidates)];
        c.rarity = targetRarity;

        // Price by rarity: flat prices
        int base;
        switch (c.rarity) {
            case 0:  base = 8; break;
            case 1:  base = 14; break;
            default: base = 32; break;
        }
        // Wholesale: 30% discount on companions
        if (perkIsActive(PERK_WHOLESALE)) {
            base = base * 7 / 10;
            if (base < 1) base = 1;
        }
        c.price = static_cast<u16>(base);

        c.sold   = false;
        c.isPerk = false;
        c.perkId = 0;
        c.locked = false;
    }

    // Owned-class weighting: 40% chance to match an owned companion
    for (int i = 0; i < SHOP_CARDS; i++) {
        ShopCard& c = gShop.cards[i];
        if (c.sold || c.locked) continue;
        if (gCompanionCount > 0 && rngRange(100) < 40) {
            // Match an owned companion's COLOR only; pick a random class within that color.
            int pick = rngRange(gCompanionCount);
            int found = 0;
            for (int j = 0; j < MAX_COMPANIONS; ++j) {
                if (!gCompanions[j].active) continue;
                if (found == pick) {
                    c.color = gCompanions[j].color;
                    // Re-roll classId within the matched color at the card's existing rarity
                    int colorBase = static_cast<int>(c.color) * 6;
                    u8 cands[6];
                    int numCands = 0;
                    for (int ci = 0; ci < 6; ci++) {
                        if (kClassDefs[colorBase + ci].rarity == c.rarity) {
                            cands[numCands++] = static_cast<u8>(ci);
                        }
                    }
                    if (numCands > 0) c.classId = cands[rngRange(numCands)];
                    int fci = colorBase + c.classId;
                    int base = (c.rarity == 0) ? 8 : (c.rarity == 1) ? 14 : 32;
                    if (perkIsActive(PERK_WHOLESALE)) {
                        base = base * 7 / 10;
                        if (base < 1) base = 1;
                    }
                    c.price = static_cast<u16>(base);
                    break;
                }
                found++;
            }
        }
    }

    // Perk slot
    gShop.perkCard.isPerk = true;
    int pid = perkGetRandom();
    if (pid >= 0) {
        gShop.perkCard.perkId = static_cast<u8>(pid);
        gShop.perkCard.price = kPerkPriceShop[pid];
        gShop.perkCard.sold = false;
        gShop.perkCard.locked = false;
    } else {
        // All perks owned
        gShop.perkCard.sold = true;
    }

    gShop.rerollCount = 0;
    gShop.selectedCard = -1;
    gShop.selectedCompanion = -1;
    gShop.lockTarget = -1;
    gShop.errorCard = -1;
    gShop.errorTimer = 0;

    // Yellow T1 (Fortune): reset free reroll for this shop visit
    gSynergy.yellowFreeRerollUsed = false;

    shopDirty = true;
}

// ── shopUpdate ────────────────────────────────────────────────────

bool shopUpdate() {
    // Tick down error flash timer
    if (gShop.errorTimer > 0) {
        gShop.errorTimer--;
        if (gShop.errorTimer == 0) {
            gShop.errorCard = -1;
            shopDirty = true;
        } else {
            shopDirty = true;
        }
    }

    if (keysDown() & KEY_START) {
        return true;
    }

    if (keysDown() & KEY_TOUCH) {
        touchPosition touch;
        touchRead(&touch);
        int ty = touch.py;
        int tx = touch.px;

        // Card buttons: row 0 y=4..42, row 1 y=46..84
        int btnRow = -1;
        if (ty >= CARD_ROW_Y[0] && ty < CARD_ROW_Y[0] + CARD_H) btnRow = 0;
        if (ty >= CARD_ROW_Y[1] && ty < CARD_ROW_Y[1] + CARD_H) btnRow = 1;

        if (btnRow >= 0) {
            int col = -1;
            for (int c = 0; c < 3; ++c) {
                if (tx >= CARD_X[c] && tx < CARD_X[c] + CARD_W) { col = c; break; }
            }
            if (col >= 0) {
                int cardIdx = btnRow * 3 + col;
                if (cardIdx >= 0 && cardIdx < SHOP_CARDS) {
                    ShopCard& c = gShop.cards[cardIdx];
                    if (!c.sold) {
                        if (gShop.selectedCard == static_cast<s8>(cardIdx)) {
                            // Tap same card again: deselect
                            gShop.selectedCard = -1;
                        } else {
                            gShop.selectedCard = static_cast<s8>(cardIdx);
                            gShop.selectedCompanion = -1;
                        }
                        shopDirty = true;
                    }
                }
            }
        }

        // Lock toggle button: fixed position between perk card and reroll (x=124..178, y=90..122)
        static const int LOCK_BTN_X = 124;
        static const int LOCK_BTN_W = 54;
        if (ty >= PERK_Y && ty < PERK_Y + PERK_H
            && tx >= LOCK_BTN_X && tx < LOCK_BTN_X + LOCK_BTN_W
            && gShop.selectedCard >= 0 && gShop.selectedCard < SHOP_CARDS) {
            gShop.cards[gShop.selectedCard].locked = !gShop.cards[gShop.selectedCard].locked;
            shopDirty = true;
        }

        // Perk card: y=90..122, x < 130 (only if lock button didn't handle it)
        else if (ty >= PERK_Y && ty < PERK_Y + PERK_H && tx < 130) {
            ShopCard& pc = gShop.perkCard;
            if (!pc.sold) {
                if (gShop.selectedCard == 6) {
                    // Tap perk again: deselect
                    gShop.selectedCard = -1;
                } else {
                    gShop.selectedCard = 6;
                    gShop.selectedCompanion = -1;
                }
                shopDirty = true;
            }
        }

        // REROLL button: y=90..122, x >= 150 (only if lock/perk didn't handle it)
        else if (ty >= REROLL_Y && ty < REROLL_Y + REROLL_H && tx >= 150) {
            int cost = 3 + gShop.rerollCount;
            // PASSIVE_REROLL_DISCOUNT: reduce reroll cost
            cost -= gPassive.rerollDiscount;
            if (cost < 0) cost = 0;

            // Yellow T1 (Fortune): free reroll once per shop visit
            bool freeRR = synergyFreeReroll();
            if (freeRR) cost = 0;

            if (gPlayer.gold >= static_cast<u16>(cost)) {
                gPlayer.gold = static_cast<u16>(gPlayer.gold - cost);
                u8 savedRerolls = static_cast<u8>(gShop.rerollCount + 1);

                if (freeRR) synergyMarkFreeRerollUsed();

                // Save lock states
                ShopCard savedCards[SHOP_CARDS];
                for (int i = 0; i < SHOP_CARDS; i++) {
                    savedCards[i] = gShop.cards[i];
                }

                gShop.selectedCard = -1;
                gShop.selectedCompanion = -1;
                audioPlaySfx(GSFX_REROLL);
                shopGenerate(gameGetWave());
                gShop.rerollCount = savedRerolls;

                // Restore locked cards (shopGenerate already skips them,
                // but we saved state just in case)
            }
        }

        // Context button: y=164..188
        if (ty >= START_Y && ty < START_Y + START_H
            && tx >= START_X && tx < START_X + START_W) {
            if (gShop.selectedCard >= 0 && gShop.selectedCard < SHOP_CARDS) {
                // BUY companion
                ShopCard& c = gShop.cards[gShop.selectedCard];
                if (!c.sold && gPlayer.gold >= c.price) {
                    bool canAdd = (gCompanionCount < perkMaxCompanions());
                    if (!canAdd) {
                        // Check if buying would trigger a merge chain that frees a slot
                        // Buying spawns at T0. Need 2 existing T0 of same class for direct merge.
                        // Cascade: 3 T0 → 1 T1. If that T1 joins 1 existing T1 → 1 T2. Net -2 slots.
                        int fci = static_cast<int>(c.color) * 6 + c.classId;
                        int countT0 = 0, countT1 = 0;
                        for (int j = 0; j < MAX_COMPANIONS; j++) {
                            if (gCompanions[j].active && companionFullClassId(gCompanions[j]) == fci) {
                                if (gCompanions[j].tier == 0) countT0++;
                                else if (gCompanions[j].tier == 1) countT1++;
                            }
                        }
                        // With Shortcut perk: merge needs only 2 (1 existing + 1 new)
                        // Without: merge needs 3 (2 existing + 1 new)
                        int mergeThreshold = perkIsActive(PERK_SHORTCUT) ? 1 : 2;
                        canAdd = (countT0 >= mergeThreshold);
                    }
                    if (!canAdd) {
                        audioPlaySfx(GSFX_HIT);
                        gShop.errorCard = gShop.selectedCard;
                        gShop.errorTimer = 30;
                    } else {
                        gPlayer.gold = static_cast<u16>(gPlayer.gold - c.price);
                        Companion* spawned = companionBuy(c.classId, c.color);
                        if (spawned) {
                            while (companionCheckMerge()) {}
                            c.sold = true;
                            audioPlaySfx(GSFX_BUY);
                        } else {
                            // Inventory full, no merge possible -- refund and show error
                            gPlayer.gold = static_cast<u16>(gPlayer.gold + c.price);
                            audioPlaySfx(GSFX_HIT);
                            gShop.errorCard = gShop.selectedCard;
                            gShop.errorTimer = 30;
                        }
                    }
                }
                gShop.selectedCard = -1;
                shopDirty = true;
            } else if (gShop.selectedCard == 6) {
                // BUY perk
                ShopCard& pc = gShop.perkCard;
                if (!pc.sold && gPlayer.gold >= pc.price) {
                    gPlayer.gold = static_cast<u16>(gPlayer.gold - pc.price);
                    perkApplyOnBuy(static_cast<PerkId>(pc.perkId));
                    pc.sold = true;
                    audioPlaySfx(GSFX_BUY);
                }
                gShop.selectedCard = -1;
                shopDirty = true;
            } else if (gShop.selectedCompanion >= 0) {
                // SELL companion
                int slot = gShop.selectedCompanion;
                if (slot >= 0 && slot < MAX_COMPANIONS && gCompanions[slot].active) {
                    static const u16 sellByRarity[] = {3, 5, 12};
                    int fci = companionFullClassId(gCompanions[slot]);
                    u16 sellValue = sellByRarity[kClassDefs[fci].rarity];
                    gPlayer.gold = static_cast<u16>(gPlayer.gold + sellValue);
                    companionRemove(slot);
                    audioPlaySfx(GSFX_SELL);
                }
                gShop.selectedCompanion = -1;
                shopDirty = true;
            } else {
                // START WAVE
                return true;
            }
        }

        // Inventory tap: select companion for preview (y=128..140)
        if (ty >= COMP_Y && ty < COMP_Y + 12) {
            int slotX = 4;
            for (int i = 0; i < MAX_COMPANIONS; ++i) {
                if (!gCompanions[i].active) continue;
                if (tx >= slotX && tx < slotX + 10) {
                    if (gShop.selectedCompanion == static_cast<s8>(i)) {
                        // Tap same companion again: deselect
                        gShop.selectedCompanion = -1;
                    } else {
                        gShop.selectedCompanion = static_cast<s8>(i);
                        gShop.selectedCard = -1;
                    }
                    shopDirty = true;
                    break;
                }
                slotX += 14;
            }
        }

        // Heal button: y=144..160, x=2..82
        if (ty >= HEAL_Y && ty < HEAL_Y + HEAL_H
            && tx >= HEAL_X && tx < HEAL_X + HEAL_W) {
            if (gPlayer.gold >= HEAL_COST && gPlayer.hp < gPlayer.maxHp) {
                gPlayer.gold = static_cast<u16>(gPlayer.gold - HEAL_COST);
                // Heal player 20%
                s16 heal = gPlayer.maxHp / 5;
                if (heal < 1) heal = 1;
                gPlayer.hp += heal;
                if (gPlayer.hp > gPlayer.maxHp) gPlayer.hp = gPlayer.maxHp;
                // Heal all companions 20%
                for (auto& c : gCompanions) {
                    if (!c.active) continue;
                    s16 cheal = c.maxHp / 5;
                    if (cheal < 1) cheal = 1;
                    c.hp += cheal;
                    if (c.hp > c.maxHp) c.hp = c.maxHp;
                }
                audioPlaySfx(GSFX_HEAL);
                shopDirty = true;
            }
        }
    }

    // D-pad navigation: cursor through shop items
    // Layout: row 0 = cards 0-2, row 1 = cards 3-5, row 2 = perk/reroll, row 3 = start
    u32 kd = keysDown();

    if (kd & KEY_B) {
        // B deselects
        if (gShop.selectedCard >= 0 || gShop.selectedCompanion >= 0) {
            gShop.selectedCard = -1;
            gShop.selectedCompanion = -1;
            shopDirty = true;
        }
    }

    if (kd & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)) {
        int cur = gShop.selectedCard; // -1=none, 0-5=cards, 6=perk, 7=reroll, 8=start, 9+=companions
        if (gShop.selectedCompanion >= 0) {
            // Map companion slot to cursor index
            int idx = 0;
            for (int i = 0; i < gShop.selectedCompanion && i < MAX_COMPANIONS; i++) {
                if (gCompanions[i].active) idx++;
            }
            cur = 9 + idx;
        }

        if (cur < 0) {
            // Nothing selected — start at card 0
            cur = 0;
        } else if (kd & KEY_RIGHT) {
            if (cur < 5) cur++;
            else if (cur == 6) cur = 7;  // perk → reroll
            else if (cur >= 9) cur++;     // next companion
        } else if (kd & KEY_LEFT) {
            if (cur > 0 && cur <= 5) cur--;
            else if (cur == 7) cur = 6;  // reroll → perk
            else if (cur > 9) cur--;      // prev companion
        } else if (kd & KEY_DOWN) {
            if (cur <= 2) cur += 3;       // row 0 → row 1
            else if (cur <= 5) cur = 6;   // row 1 → perk
            else if (cur <= 7) cur = 8;   // perk/reroll → START (skip companions)
            else if (cur == 8) cur = 0;   // start → wrap to top
            else if (cur >= 9) cur = 8;   // companions → start
        } else if (kd & KEY_UP) {
            if (cur >= 3 && cur <= 5) cur -= 3; // row 1 → row 0
            else if (cur == 6 || cur == 7) cur = 3; // perk/reroll → row 1
            else if (cur == 8) cur = 6;   // start → perk
            else if (cur >= 9) cur = 6;   // companions → perk
        }

        // Companion row: cur 9+ means companion slot (9=first active, 10=second, etc)
        if (cur >= 9) {
            // Find the Nth active companion
            int target = cur - 9;
            int found = -1;
            int count = 0;
            for (int i = 0; i < MAX_COMPANIONS; i++) {
                if (gCompanions[i].active) {
                    if (count == target) { found = i; break; }
                    count++;
                }
            }
            if (found >= 0) {
                gShop.selectedCompanion = static_cast<s8>(found);
                gShop.selectedCard = -1;
            } else {
                // No companion at that index — clamp to last
                cur = 8; // fall to start
                gShop.selectedCard = -1;
                gShop.selectedCompanion = -1;
            }
        } else if (cur >= 0 && cur <= 7) {
            gShop.selectedCard = static_cast<s8>(cur);
            gShop.selectedCompanion = -1;
        } else if (cur == 8) {
            gShop.selectedCard = -1;
            gShop.selectedCompanion = -1;
        }
        shopDirty = true;
    }

    // R button = cycle through owned companions (separate from main D-pad flow)
    if (kd & KEY_R) {
        if (gShop.selectedCompanion < 0) {
            // Enter companion selection — find first active
            for (int i = 0; i < MAX_COMPANIONS; i++) {
                if (gCompanions[i].active) { gShop.selectedCompanion = static_cast<s8>(i); break; }
            }
        } else {
            // Cycle to next active companion
            int start = gShop.selectedCompanion + 1;
            gShop.selectedCompanion = -1;
            for (int i = start; i < start + MAX_COMPANIONS; i++) {
                int idx = i % MAX_COMPANIONS;
                if (gCompanions[idx].active) { gShop.selectedCompanion = static_cast<s8>(idx); break; }
            }
        }
        if (gShop.selectedCompanion >= 0) gShop.selectedCard = -1;
        shopDirty = true;
    }

    // L button = toggle lock on selected card
    if ((kd & KEY_L) && gShop.selectedCard >= 0 && gShop.selectedCard < SHOP_CARDS) {
        gShop.cards[gShop.selectedCard].locked = !gShop.cards[gShop.selectedCard].locked;
        shopDirty = true;
    }

    // A button = confirm current selection
    if (kd & KEY_A) {
        // "Start" selected or nothing selected → start wave
        if (gShop.selectedCard == 8 || (gShop.selectedCard < 0 && gShop.selectedCompanion < 0)) {
            return true;
        }
        // Reroll selected
        if (gShop.selectedCard == 7) {
            int cost = 3 + gShop.rerollCount;
            cost -= gPassive.rerollDiscount;
            if (cost < 0) cost = 0;
            bool freeRR = synergyFreeReroll();
            if (freeRR) cost = 0;
            if (gPlayer.gold >= static_cast<u16>(cost)) {
                gPlayer.gold = static_cast<u16>(gPlayer.gold - cost);
                u8 savedRerolls = static_cast<u8>(gShop.rerollCount + 1);
                if (freeRR) synergyMarkFreeRerollUsed();
                gShop.selectedCard = -1;
                gShop.selectedCompanion = -1;
                audioPlaySfx(GSFX_REROLL);
                shopGenerate(gameGetWave());
                gShop.rerollCount = savedRerolls;
            }
            shopDirty = true;
        }
        // Card 0-5 selected → buy companion
        else if (gShop.selectedCard >= 0 && gShop.selectedCard < SHOP_CARDS) {
            ShopCard& c = gShop.cards[gShop.selectedCard];
            if (!c.sold && gPlayer.gold >= c.price) {
                bool canAdd = (gCompanionCount < perkMaxCompanions());
                if (!canAdd) {
                    int fci = static_cast<int>(c.color) * 6 + c.classId;
                    int countT0 = 0;
                    for (int j = 0; j < MAX_COMPANIONS; j++) {
                        if (gCompanions[j].active && companionFullClassId(gCompanions[j]) == fci && gCompanions[j].tier == 0)
                            countT0++;
                    }
                    int mergeThreshold = perkIsActive(PERK_SHORTCUT) ? 1 : 2;
                    canAdd = (countT0 >= mergeThreshold);
                }
                if (canAdd) {
                    gPlayer.gold = static_cast<u16>(gPlayer.gold - c.price);
                    Companion* spawned = companionBuy(c.classId, c.color);
                    if (spawned) {
                        while (companionCheckMerge()) {}
                        c.sold = true;
                        audioPlaySfx(GSFX_BUY);
                    }
                } else {
                    audioPlaySfx(GSFX_HIT);
                    gShop.errorCard = gShop.selectedCard;
                    gShop.errorTimer = 30;
                }
            }
            gShop.selectedCard = -1;
            shopDirty = true;
        }
        // Perk card selected → buy perk
        else if (gShop.selectedCard == 6) {
            ShopCard& pc = gShop.perkCard;
            if (!pc.sold && gPlayer.gold >= pc.price) {
                gPlayer.gold = static_cast<u16>(gPlayer.gold - pc.price);
                perkApplyOnBuy(static_cast<PerkId>(pc.perkId));
                pc.sold = true;
                audioPlaySfx(GSFX_BUY);
            }
            gShop.selectedCard = -1;
            shopDirty = true;
        }
        // Companion selected → sell
        else if (gShop.selectedCompanion >= 0) {
            int slot = gShop.selectedCompanion;
            if (slot >= 0 && slot < MAX_COMPANIONS && gCompanions[slot].active) {
                static const u16 sellByRarity[] = {3, 5, 12};
                int fci = companionFullClassId(gCompanions[slot]);
                u16 sellValue = sellByRarity[kClassDefs[fci].rarity];
                gPlayer.gold = static_cast<u16>(gPlayer.gold + sellValue);
                companionRemove(slot);
                audioPlaySfx(GSFX_SELL);
            }
            gShop.selectedCompanion = -1;
            shopDirty = true;
        }
    }

    return false;
}

// ── shopRender ────────────────────────────────────────────────────

static void drawCompanionCard(int cardIdx) {
    const ShopCard& card = gShop.cards[cardIdx];
    int row = cardIdx / 3;
    int col = cardIdx % 3;
    int bx  = CARD_X[col];
    int by  = CARD_ROW_Y[row];

    bool selected = (gShop.selectedCard == static_cast<s8>(cardIdx));
    bool canAfford = (gPlayer.gold >= card.price);
    int colorIdx = static_cast<int>(card.color);

    // Background color
    u16 bgColor;
    bool isErrorFlash = (gShop.errorCard == static_cast<s8>(cardIdx) && gShop.errorTimer > 0);
    if (card.sold) {
        bgColor = RGB15(5, 5, 5);
    } else if (isErrorFlash) {
        bgColor = RGB15(28, 4, 4); // red flash: slots full
    } else {
        u16 full = pillColorToRGB(card.color);
        if (selected) {
            bgColor = full;
        } else if (!canAfford) {
            bgColor = (full >> 2) & 0x1CE7;
        } else {
            bgColor = (full >> 1) & 0x3DEF;
        }
    }
    renderFilledRectSub(bx, by, CARD_W, CARD_H, bgColor);

    if (card.sold) {
        int tw = renderTextWidth(str(kUI[44]));
        renderTextSub(bx + (CARD_W - tw) / 2, by + 16, str(kUI[44]), RGB15(14, 14, 14));
        return;
    }

    // Locked: gold/yellow border (2px)
    if (card.locked) {
        u16 lc = RGB15(31, 28, 0);
        renderFilledRectSub(bx,              by,              CARD_W, 2,      lc);
        renderFilledRectSub(bx,              by + CARD_H - 2, CARD_W, 2,      lc);
        renderFilledRectSub(bx,              by,              2,      CARD_H, lc);
        renderFilledRectSub(bx + CARD_W - 2, by,              2,      CARD_H, lc);
    }

    // Selected: 2px bright border (on top of lock border if both)
    if (selected) {
        u16 bc = RGB15(31, 31, 31);
        renderFilledRectSub(bx,              by,              CARD_W, 2,      bc);
        renderFilledRectSub(bx,              by + CARD_H - 2, CARD_W, 2,      bc);
        renderFilledRectSub(bx,              by,              2,      CARD_H, bc);
        renderFilledRectSub(bx + CARD_W - 2, by,              2,      CARD_H, bc);
    }

    // Rarity indicator: small yellow dots top-right
    if (card.rarity >= 1)
        renderFilledRectSub(bx + CARD_W - 5, by + 2, 3, 3, RGB15(31, 28, 0));
    if (card.rarity >= 2)
        renderFilledRectSub(bx + CARD_W - 9, by + 2, 3, 3, RGB15(31, 28, 0));

    // Class name (centered, word-wrap to 2 lines if needed)
    const char* name = str(kClassNames[colorIdx][card.classId][0]);
    u16 textColor = canAfford ? RGB15(31, 31, 31) : RGB15(16, 16, 16);
    constexpr int NAME_MAX = 13; // CARD_W(78) / 6px per char
    int nameLen = 0;
    while (name[nameLen]) nameLen++;

    if (nameLen <= NAME_MAX) {
        // Single line
        int tw = renderTextWidth(name);
        renderTextSub(bx + (CARD_W - tw) / 2, by + 6, name, textColor);
    } else {
        // Word-wrap: find last space within NAME_MAX chars
        int split = -1;
        for (int si = NAME_MAX - 1; si >= 0; --si) {
            if (name[si] == ' ') { split = si; break; }
        }
        char line1[16], line2[16];
        if (split > 0) {
            for (int i = 0; i < split; i++) line1[i] = name[i];
            line1[split] = '\0';
            int li = 0;
            for (int i = split + 1; name[i] && li < NAME_MAX; i++, li++)
                line2[li] = name[i];
            line2[li] = '\0';
        } else {
            // No space — truncate with "."
            for (int i = 0; i < NAME_MAX - 1; i++) line1[i] = name[i];
            line1[NAME_MAX - 1] = '.';
            line1[NAME_MAX] = '\0';
            line2[0] = '\0';
        }
        int tw1 = renderTextWidth(line1);
        renderTextSub(bx + (CARD_W - tw1) / 2, by + 6, line1, textColor);
        if (line2[0]) {
            int tw2 = renderTextWidth(line2);
            renderTextSub(bx + (CARD_W - tw2) / 2, by + 14, line2, textColor);
        }
    }

    // Cost (bottom, centered)
    char costBuf[8];
    snprintf(costBuf, sizeof(costBuf), "%dg", card.price);
    int cw = renderTextWidth(costBuf);
    renderTextSub(bx + (CARD_W - cw) / 2, by + CARD_H - 12, costBuf,
        canAfford ? RGB15(31, 28, 0) : RGB15(14, 12, 0));
}

static void drawPerkCard() {
    const ShopCard& card = gShop.perkCard;
    bool selected = (gShop.selectedCard == 6);
    bool canAfford = (gPlayer.gold >= card.price);

    // Gold background
    u16 bgColor;
    if (card.sold) {
        bgColor = RGB15(5, 5, 5);
    } else {
        bgColor = RGB15(31, 28, 0);
    }
    renderFilledRectSub(PERK_X, PERK_Y, PERK_W, PERK_H, bgColor);

    if (card.sold) {
        int tw = renderTextWidth(str(kUI[44]));
        renderTextSub(PERK_X + (PERK_W - tw) / 2, PERK_Y + 12, str(kUI[44]), RGB15(14, 14, 14));
        return;
    }

    // Selected border
    if (selected) {
        u16 bc = RGB15(31, 31, 31);
        renderFilledRectSub(PERK_X,              PERK_Y,              PERK_W, 2,      bc);
        renderFilledRectSub(PERK_X,              PERK_Y + PERK_H - 2, PERK_W, 2,      bc);
        renderFilledRectSub(PERK_X,              PERK_Y,              2,      PERK_H, bc);
        renderFilledRectSub(PERK_X + PERK_W - 2, PERK_Y,              2,      PERK_H, bc);
    }

    // Perk name (dark text on gold)
    const char* name = str(kPerks[card.perkId].name);
    char nameBuf[16];
    int ni = 0;
    while (name[ni] && ni < 15) { nameBuf[ni] = name[ni]; ni++; }
    nameBuf[ni] = '\0';

    u16 textColor = RGB15(4, 4, 0);
    int tw = renderTextWidth(nameBuf);
    renderTextSub(PERK_X + (PERK_W - tw) / 2, PERK_Y + 4, nameBuf, textColor);

    // Cost
    char costBuf[8];
    snprintf(costBuf, sizeof(costBuf), "%dg", card.price);
    int cw = renderTextWidth(costBuf);
    renderTextSub(PERK_X + (PERK_W - cw) / 2, PERK_Y + PERK_H - 12, costBuf,
        canAfford ? RGB15(4, 4, 0) : RGB15(14, 12, 0));
}

void shopRender() {
    if (!shopDirty) return;
    shopDirty = false;

    renderClearSub();

    // ── Companion cards (2 rows x 3 columns) ─────────────────────
    for (int i = 0; i < SHOP_CARDS; ++i) {
        drawCompanionCard(i);
    }

    // ── Lock button: fixed slot between perk card and reroll (x=124, y=90, 54x32) ──
    // Only visible when a companion card is selected.
    static const int LOCK_BTN_X = 124;
    static const int LOCK_BTN_W = 54;
    if (gShop.selectedCard >= 0 && gShop.selectedCard < SHOP_CARDS) {
        bool isLocked = gShop.cards[gShop.selectedCard].locked;
        u16 lbBg = isLocked ? RGB15(14, 12, 0) : RGB15(6, 6, 6);
        renderFilledRectSub(LOCK_BTN_X, PERK_Y, LOCK_BTN_W, PERK_H, lbBg);
        const char* lockText = isLocked ? str(kUI[46]) : str(kUI[45]);
        int ltw = renderTextWidth(lockText);
        u16 lockColor = isLocked ? RGB15(31, 28, 0) : RGB15(20, 20, 20);
        renderTextSub(LOCK_BTN_X + (LOCK_BTN_W - ltw) / 2, PERK_Y + (PERK_H - 8) / 2, lockText, lockColor);
    }

    // ── Perk card (row 3 left) ───────────────────────────────────
    drawPerkCard();

    // ── Reroll button (row 3 right) ──────────────────────────────
    int rerollCost = 3 + gShop.rerollCount;
    // PASSIVE_REROLL_DISCOUNT: reduce displayed reroll cost
    rerollCost -= gPassive.rerollDiscount;
    if (rerollCost < 0) rerollCost = 0;
    bool freeRR = synergyFreeReroll();
    bool canReroll = freeRR || (gPlayer.gold >= static_cast<u16>(rerollCost));
    bool rerollSel = (gShop.selectedCard == 7);
    u16 rerollBg = rerollSel ? RGB15(10, 14, 10) : freeRR ? RGB15(10, 16, 0) : canReroll ? RGB15(6, 10, 6) : RGB15(4, 4, 4);
    renderFilledRectSub(REROLL_X, REROLL_Y, REROLL_W, REROLL_H, rerollBg);
    if (rerollSel) {
        renderFilledRectSub(REROLL_X, REROLL_Y, REROLL_W, 1, RGB15(31,31,31));
        renderFilledRectSub(REROLL_X, REROLL_Y+REROLL_H-1, REROLL_W, 1, RGB15(31,31,31));
    }
    char rerollBuf[12];
    if (freeRR)
        snprintf(rerollBuf, sizeof(rerollBuf), "%s", str(kUI[32]));
    else
        snprintf(rerollBuf, sizeof(rerollBuf), str(kUI[33]), rerollCost);
    int rtw = renderTextWidth(rerollBuf);
    u16 rerollTextColor = freeRR ? RGB15(31, 31, 0) : canReroll ? RGB15(10, 31, 10) : RGB15(10, 14, 10);
    renderTextSub(REROLL_X + (REROLL_W - rtw) / 2, REROLL_Y + 10, rerollBuf, rerollTextColor);

    // ── Gold display + Interest ──────────────────────────────────
    char goldBuf[16];
    snprintf(goldBuf, sizeof(goldBuf), "%s %d", str(kUI[47]), gPlayer.gold);
    renderTextSub(140, COMP_Y + 2, goldBuf, RGB15(31, 28, 0));

    char intBuf[12];
    snprintf(intBuf, sizeof(intBuf), "+%d %s", gameGetShopInterest(), str(kUI[48]));
    renderTextSub(210, COMP_Y + 2, intBuf, RGB15(20, 28, 20));

    // ── Companion inventory bar (y=128) ──────────────────────────
    int compX = 4;
    for (int i = 0; i < MAX_COMPANIONS; ++i) {
        if (!gCompanions[i].active) continue;
        const Companion& comp = gCompanions[i];

        u16 fullColor = pillColorToRGB(comp.baseColor);
        int hpPct = (comp.maxHp > 0) ? (comp.hp * 100) / comp.maxHp : 0;
        if (hpPct < 0) hpPct = 0;
        if (hpPct > 100) hpPct = 100;
        u16 r = ((fullColor >>  0) & 0x1F) * (50 + hpPct / 2) / 100;
        u16 g = ((fullColor >>  5) & 0x1F) * (50 + hpPct / 2) / 100;
        u16 b = ((fullColor >> 10) & 0x1F) * (50 + hpPct / 2) / 100;
        u16 sqColor = static_cast<u16>(r | (g << 5) | (b << 10));
        renderFilledRectSub(compX, COMP_Y, 10, 10, sqColor);

        // Selection border for selected companion
        if (gShop.selectedCompanion == static_cast<s8>(i)) {
            u16 bc = RGB15(31, 31, 31);
            renderFilledRectSub(compX,     COMP_Y,     10, 1,  bc);
            renderFilledRectSub(compX,     COMP_Y + 9, 10, 1,  bc);
            renderFilledRectSub(compX,     COMP_Y,     1,  10, bc);
            renderFilledRectSub(compX + 9, COMP_Y,     1,  10, bc);
        }

        // Tier number
        char tBuf[2];
        tBuf[0] = static_cast<char>('0' + comp.tier + 1);
        tBuf[1] = '\0';
        renderTextSub(compX + 2, COMP_Y + 2, tBuf, RGB15(31, 31, 31));

        compX += 14;
    }

    // ── Context-sensitive bottom button ────────────────────────────
    if (gShop.selectedCard >= 0 && gShop.selectedCard < SHOP_CARDS) {
        // BUY companion button
        const ShopCard& selCard = gShop.cards[gShop.selectedCard];
        bool canBuy = (!selCard.sold && gPlayer.gold >= selCard.price);
        u16 buyBg = canBuy ? RGB15(2, 16, 2) : RGB15(6, 4, 4);
        renderFilledRectSub(START_X, START_Y, START_W, START_H, buyBg);
        char buyBuf[20];
        snprintf(buyBuf, sizeof(buyBuf), "%s %dG", str(kUI[49]), selCard.price);
        int btw = renderTextWidth(buyBuf);
        renderTextSub(START_X + (START_W - btw) / 2, START_Y + 8, buyBuf, RGB15(31, 31, 31));
    } else if (gShop.selectedCard == 6) {
        // BUY perk button
        const ShopCard& pc = gShop.perkCard;
        bool canBuy = (!pc.sold && gPlayer.gold >= pc.price);
        u16 buyBg = canBuy ? RGB15(2, 16, 2) : RGB15(6, 4, 4);
        renderFilledRectSub(START_X, START_Y, START_W, START_H, buyBg);
        char buyBuf[20];
        snprintf(buyBuf, sizeof(buyBuf), "%s %dG", str(kUI[49]), pc.price);
        int btw = renderTextWidth(buyBuf);
        renderTextSub(START_X + (START_W - btw) / 2, START_Y + 8, buyBuf, RGB15(31, 31, 31));
    } else if (gShop.selectedCompanion >= 0) {
        // SELL companion button
        static const u16 sellByRarity[] = {3, 5, 12};
        int slot = gShop.selectedCompanion;
        u16 sellPrice = 0;
        if (slot >= 0 && slot < MAX_COMPANIONS && gCompanions[slot].active) {
            int fci = companionFullClassId(gCompanions[slot]);
            sellPrice = sellByRarity[kClassDefs[fci].rarity];
        }
        renderFilledRectSub(START_X, START_Y, START_W, START_H, RGB15(16, 2, 2));
        char sellBuf[20];
        snprintf(sellBuf, sizeof(sellBuf), "%s %dG", str(kUI[50]), sellPrice);
        int stw = renderTextWidth(sellBuf);
        renderTextSub(START_X + (START_W - stw) / 2, START_Y + 8, sellBuf, RGB15(31, 31, 31));
    } else {
        // START WAVE (default)
        renderFilledRectSub(START_X, START_Y, START_W, START_H, RGB15(2, 14, 2));
        const char* swText = str(kUI[12]);
        int swtw = renderTextWidth(swText);
        renderTextSub(START_X + (START_W - swtw) / 2, START_Y + 8, swText,
            RGB15(14, 31, 14));
    }

    // Heal button
    {
        bool canHeal = (gPlayer.gold >= HEAL_COST && gPlayer.hp < gPlayer.maxHp);
        u16 healBg = canHeal ? RGB15(2, 12, 4) : RGB15(4, 4, 4);
        u16 healTxt = canHeal ? RGB15(10, 31, 10) : RGB15(10, 10, 10);
        renderFilledRectSub(HEAL_X, HEAL_Y, HEAL_W, HEAL_H, healBg);
        char healBuf[20];
        snprintf(healBuf, sizeof(healBuf), str(kUI[31]), HEAL_COST);
        int htw = renderTextWidth(healBuf);
        renderTextSub(HEAL_X + (HEAL_W - htw) / 2, HEAL_Y + 4, healBuf, healTxt);
    }

    // ── Control hints at bottom ──────────────────────────────────────
    u16 hintCol = RGB15(10, 10, 14);
    renderTextSub(4, 184, str(kUI[61]), hintCol);
}
