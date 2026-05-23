#include "ui.h"
#include "player.h"
#include "rng.h"
#include "audio.h"
#include <nds.h>
#include <stdio.h>

static UpgradeChoice choices[UI_CHOICE_COUNT];
static int selectedChoice = -1;
static PrintConsole bottomConsole;

// Choice button regions on bottom screen (y positions)
constexpr int CHOICE_X = 16;
constexpr int CHOICE_W = 224;
constexpr int CHOICE_H = 48;
constexpr int CHOICE_GAP = 8;
constexpr int CHOICE_START_Y = 16;

static int choiceY(int idx) {
    return CHOICE_START_Y + idx * (CHOICE_H + CHOICE_GAP);
}

void uiInit() {
    // Set up a console on the sub screen for text
    consoleInit(&bottomConsole, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    consoleSelect(&bottomConsole);
    selectedChoice = -1;
}

void uiUpdate() {
    selectedChoice = -1;

    if (keysDown() & KEY_TOUCH) {
        touchPosition touch;
        touchRead(&touch);

        for (int i = 0; i < UI_CHOICE_COUNT; ++i) {
            if (!choices[i].available) continue;

            int cy = choiceY(i);
            if (touch.px >= CHOICE_X && touch.px < CHOICE_X + CHOICE_W &&
                touch.py >= cy && touch.py < cy + CHOICE_H) {
                selectedChoice = i;
                audioPlaySfx(SFX_SELECT);
                break;
            }
        }
    }
}

void uiRenderHUD() {
    consoleSelect(&bottomConsole);
    consoleClear();

    printf("\x1b[1;1H PILL SHOOTER");
    printf("\x1b[3;1H HP: %d/%d", gPlayer.hp, gPlayer.maxHp);

    // Show inventory
    const char* colorNames[] = {"RED", "BLU", "GRN", "YLW"};
    const char* tierNames[] = {"", "S", "U", "M"};

    for (int i = 0; i < PILL_COLOR_COUNT; ++i) {
        const PillSlot& slot = gPlayer.inventory[i];
        printf("\x1b[%d;1H %s: %d %s",
                5 + i, colorNames[i], slot.count, tierNames[static_cast<int>(slot.tier)]);
    }
}

void uiRenderUpgradeScreen() {
    consoleSelect(&bottomConsole);
    consoleClear();

    printf("\x1b[0;6H== CHOOSE UPGRADE ==");

    for (int i = 0; i < UI_CHOICE_COUNT; ++i) {
        if (!choices[i].available) continue;
        int row = 3 + i * 6;
        printf("\x1b[%d;2H [%d] %s", row, i + 1, choices[i].name);
        printf("\x1b[%d;2H     %s", row + 1, choices[i].desc);
    }

    printf("\x1b[22;3H Tap a choice to select");
}

void uiGenerateChoices() {
    static const char* names[] = {
        "Damage Boost", "Speed Boost", "Regen Field", "Rapid Fire"
    };
    static const char* descs[] = {
        "+1 bullet damage", "+1 move speed", "+1 HP regen/10s", "-2 shoot cooldown"
    };

    for (int i = 0; i < UI_CHOICE_COUNT; ++i) {
        int type = rngRange(PILL_COLOR_COUNT);
        choices[i].color = static_cast<PillColor>(type);
        choices[i].tier = PillTier::Normal;
        choices[i].name = names[type];
        choices[i].desc = descs[type];
        choices[i].available = true;
    }

    selectedChoice = -1;
}

int uiGetSelection() {
    return selectedChoice;
}
