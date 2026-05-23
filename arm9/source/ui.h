#ifndef UI_H
#define UI_H

#include "types.h"

// Roguelike upgrade choices shown on the bottom screen
struct UpgradeChoice {
    PillColor color;
    PillTier tier;
    const char* name;
    const char* desc;
    bool available;
};

constexpr int UI_CHOICE_COUNT = 3;

void uiInit();
void uiUpdate();
void uiRenderHUD();
void uiRenderUpgradeScreen();

// Call when entering upgrade state — generates 3 random choices
void uiGenerateChoices();

// Returns selected choice index (0-2) or -1 if none
int uiGetSelection();

#endif // UI_H
