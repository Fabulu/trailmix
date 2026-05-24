#include "perk_choice.h"
#include "render.h"
#include "strings.h"
// perk.h forward declarations (avoid PERK_COUNT redefinition with strings.h)
enum PerkId : u8;
struct PerkState {
    bool active[15];
    u8 phoenixUsedThisWave;
    u8 secondWindUsed;
    u8 goldFeverWaves;
    // (reserved padding byte)
};
extern PerkState gPerks;
bool perkIsActive(PerkId id);
int perkGetRandom();
void perkApplyOnBuy(PerkId id);
#include "audio.h"
#include <nds.h>
#include <cstdio>

// ---------------------------------------------------------------------------
// Layout constants — bottom screen (256×192 bitmap)
// ---------------------------------------------------------------------------

// 3 perk buttons stacked vertically, each 240×50, centered (x=8)
static constexpr int PC_BTN_X  = 8;
static constexpr int PC_BTN_W  = 240;
static constexpr int PC_BTN_H  = 50;
static constexpr int PC_BTN_Y[3] = { 10, 66, 122 };

// Selected-border thickness (px)
static constexpr int PC_BORDER = 2;

// Top screen column layout: 3 perk cards, each ~80px wide
// Column centres: 42, 128, 214  (screen is 256px wide)
static constexpr int PC_COL_CX[3]  = { 42, 128, 214 };
static constexpr int PC_COL_TOP    = 56;   // y where card content starts
static constexpr int PC_COL_ICON_R = 12;   // icon circle radius
static constexpr int PC_TITLE_Y    = 8;    // y for "CHOOSE A PERK" title

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static int  sPerkIds[3];      // PerkId values of the 3 offered perks (-1 = none)
static int  sSelected  = -1;  // which button is highlighted (-1 = none)
static bool sDirty     = true; // re-render on next frame

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Perk-tinted icon colours — cycle through the pill palette by perk index.
// Not semantically meaningful, just visually distinct.
static u16 perkIconColor(int perkId) {
    static const u16 kColors[] = {
        RGB15(31,  6,  6),   // red-ish     (Iron Will, +HP)
        RGB15( 0, 20, 31),   // blue        (Quick Hands)
        RGB15(31, 28,  0),   // yellow      (Gold Rush)
        RGB15(31, 16,  0),   // orange      (Double Tap)
        RGB15( 0, 28,  0),   // green       (Pack Rat)
        RGB15(31,  0, 24),   // magenta     (Adrenaline)
        RGB15( 0, 20, 28),   // cyan        (Magnet)
        RGB15(31,  8,  0),   // fire-orange (Pyromaniac)
        RGB15(14, 14, 31),   // lavender    (Slipstream)
        RGB15(20, 20, 20),   // silver      (Resilience)
        RGB15(28,  0, 31),   // purple      (Catalyst)
        RGB15(28, 20,  0),   // gold        (Piggy Bank)
        RGB15(31, 31,  8),   // bright yel  (Sharp Eye)
        RGB15( 0, 31, 16),   // teal        (Steady Rhythm)
        RGB15(20, 31,  0),   // lime        (Overclock)
    };
    if (perkId < 0 || perkId >= PERK_COUNT) return RGB15(20, 20, 20);
    return kColors[perkId];
}

// ---------------------------------------------------------------------------
// perkChoiceEnter
// ---------------------------------------------------------------------------

void perkChoiceEnter() {
    // Draw 3 distinct random unowned perks.
    for (int i = 0; i < 3; ++i) sPerkIds[i] = -1;
    for (int slot = 0; slot < 3; ++slot) {
        // Mark already-selected offers as temporarily active so perkGetRandom
        // doesn't repeat them, then restore.
        bool saved[3] = {};
        for (int j = 0; j < slot; ++j) {
            if (sPerkIds[j] >= 0) {
                saved[j] = gPerks.active[sPerkIds[j]];
                gPerks.active[sPerkIds[j]] = true;
            }
        }
        sPerkIds[slot] = perkGetRandom();
        // Restore
        for (int j = 0; j < slot; ++j) {
            if (sPerkIds[j] >= 0) gPerks.active[sPerkIds[j]] = saved[j];
        }
        if (sPerkIds[slot] < 0) break; // fewer than 3 perks available — leave rest -1
    }

    sSelected = -1;
    sDirty    = true;
}

// ---------------------------------------------------------------------------
// perkChoiceUpdate
// ---------------------------------------------------------------------------

bool perkChoiceUpdate() {
    // D-pad up/down to cycle selection
    u32 kd = keysDown();
    if (kd & KEY_UP)   { sSelected = (sSelected <= 0) ? 2 : sSelected - 1; sDirty = true; }
    if (kd & KEY_DOWN) { sSelected = (sSelected >= 2) ? 0 : sSelected + 1; sDirty = true; }

    // A button confirms highlighted choice
    if ((kd & KEY_A) && sSelected >= 0) {
        if (sPerkIds[sSelected] >= 0) {
            perkApplyOnBuy(static_cast<PerkId>(sPerkIds[sSelected]));
            audioPlaySfx(GSFX_BUY);
        }
        return true;
    }

    // Touch input: single tap highlights; second tap on same button confirms
    if (kd & KEY_TOUCH) {
        touchPosition tp;
        touchRead(&tp);
        int tx = tp.px;
        int ty = tp.py;

        for (int i = 0; i < 3; ++i) {
            if (sPerkIds[i] < 0) continue;
            if (tx >= PC_BTN_X && tx < PC_BTN_X + PC_BTN_W
                && ty >= PC_BTN_Y[i] && ty < PC_BTN_Y[i] + PC_BTN_H) {

                if (sSelected == i) {
                    // Second tap on the same perk: confirm
                    perkApplyOnBuy(static_cast<PerkId>(sPerkIds[i]));
                    audioPlaySfx(GSFX_BUY);
                    return true;
                } else {
                    sSelected = i;
                    sDirty = true;
                }
                break;
            }
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// perkChoiceRender — bottom screen
// ---------------------------------------------------------------------------

static void renderBottomScreen() {
    renderClearSub();

    // Dark background
    renderFilledRectSub(0, 0, 256, 192, RGB15(2, 2, 6));

    // Draw each of the 3 perk buttons
    for (int i = 0; i < 3; ++i) {
        int id = sPerkIds[i];
        int bx = PC_BTN_X;
        int by = PC_BTN_Y[i];

        bool sel = (sSelected == i);
        bool avail = (id >= 0);

        // Button background: slightly brighter when selected
        u16 iconCol = avail ? perkIconColor(id) : RGB15(8, 8, 8);
        u16 bgCol   = sel
            ? RGB15(8, 8, 20)
            : RGB15(4, 4, 12);
        renderFilledRectSub(bx, by, PC_BTN_W, PC_BTN_H, bgCol);

        // Left colour accent strip (6px) using perk icon colour
        u16 strip = avail ? (u16)((iconCol >> 1) & 0x3DEF) : RGB15(4, 4, 4);
        if (sel) strip = iconCol;
        renderFilledRectSub(bx, by, 6, PC_BTN_H, strip);

        // Selection border (2px, bright white)
        if (sel) {
            u16 bc = RGB15(31, 31, 31);
            renderFilledRectSub(bx,                    by,                    PC_BTN_W, PC_BORDER, bc);
            renderFilledRectSub(bx,                    by + PC_BTN_H - PC_BORDER, PC_BTN_W, PC_BORDER, bc);
            renderFilledRectSub(bx,                    by,                    PC_BORDER, PC_BTN_H, bc);
            renderFilledRectSub(bx + PC_BTN_W - PC_BORDER, by,               PC_BORDER, PC_BTN_H, bc);
        }

        if (!avail) continue;

        // Perk name (large, y+10 from button top)
        const char* name = str(kPerks[id].name);
        u16 nameCol = sel ? RGB15(31, 31, 31) : RGB15(24, 24, 31);
        renderTextSub(bx + 12, by + 10, name, nameCol);

        // Description (smaller/dimmer, y+26)
        const char* desc = str(kPerks[id].desc);
        // Truncate description to ~30 chars so it fits in 240px (each char ≈6px)
        char descBuf[31];
        int di = 0;
        while (desc[di] && di < 30) { descBuf[di] = desc[di]; di++; }
        descBuf[di] = '\0';
        u16 descCol = sel ? RGB15(20, 20, 24) : RGB15(14, 14, 18);
        renderTextSub(bx + 12, by + 26, descBuf, descCol);

        // Perk index hint on right edge (tap number label)
        char numBuf[2] = { static_cast<char>('1' + i), '\0' };
        renderTextSub(bx + PC_BTN_W - 14, by + (PC_BTN_H - 7) / 2, numBuf,
            sel ? RGB15(31, 31, 0) : RGB15(14, 14, 0));
    }

    // Footer hint
    renderTextSub(24, 178, "TAP ONCE TO SELECT  TAP AGAIN TO CONFIRM", RGB15(10, 10, 14));
}

// ---------------------------------------------------------------------------
// perkChoiceRender — top screen
// ---------------------------------------------------------------------------

static void renderTopScreen() {
    renderClearParticleLayer();

    // Hide OAM sprites so companions/player don't show through
    for (int i = 0; i < 128; i++)
        oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, nullptr, -1, false, true, false, false, false);
    oamUpdate(&oamMain);

    // Deep-space background fill
    renderFilledRect(0, 0, 256, 192, RGB15(1, 1, 4));

    // Title bar
    renderFilledRect(0, 0, 256, 18, RGB15(4, 4, 16));
    const char* title = str(kGameplay[11]); // "CHOOSE A PERK"
    int tw = renderTextWidth(title);
    renderText(128 - tw / 2, PC_TITLE_Y + 2, title, RGB15(20, 20, 31));

    // Divider below title
    renderFilledRect(0, 18, 256, 1, RGB15(8, 8, 24));

    // 3 vertical column dividers (thin, subtle)
    renderFilledRect(85,  19, 1, 173, RGB15(6, 6, 16));
    renderFilledRect(170, 19, 1, 173, RGB15(6, 6, 16));

    // Draw each perk card
    for (int i = 0; i < 3; ++i) {
        int id  = sPerkIds[i];
        int cx  = PC_COL_CX[i];
        bool sel = (sSelected == i);

        if (id < 0) {
            // No perk available — draw a greyed-out placeholder
            renderText(cx - 10, PC_COL_TOP + 30, "N/A", RGB15(10, 10, 10));
            continue;
        }

        u16 iconCol = perkIconColor(id);

        // Column highlight background when selected
        if (sel) {
            int colLeft = (i == 0) ? 0 : (i == 1 ? 86 : 171);
            int colW    = (i == 2) ? 256 - 171 : 85;
            renderFilledRect(colLeft, 19, colW, 173, RGB15(4, 4, 12));
        }

        // Icon circle
        int iconY = PC_COL_TOP;
        u16 drawCol = sel ? iconCol : (u16)((iconCol >> 1) & 0x3DEF);
        renderFilledCircle(cx, iconY, PC_COL_ICON_R, drawCol);
        // Bright ring when selected: draw slightly larger circle behind, then redraw icon
        if (sel) {
            renderFilledCircle(cx, iconY, PC_COL_ICON_R + 2, RGB15(28, 28, 31));
            renderFilledCircle(cx, iconY, PC_COL_ICON_R,     drawCol);
        }

        // Perk slot number above icon
        char slotBuf[2] = { static_cast<char>('1' + i), '\0' };
        renderText(cx - 2, iconY - PC_COL_ICON_R - 10, slotBuf,
            sel ? RGB15(31, 31, 0) : RGB15(16, 16, 0));

        // Perk name (centred below icon)
        const char* name = str(kPerks[id].name);
        int nw = renderTextWidth(name);
        u16 nameCol = sel ? RGB15(31, 31, 31) : RGB15(22, 22, 28);
        renderText(cx - nw / 2, iconY + PC_COL_ICON_R + 6, name, nameCol);

        // Description lines — word-wrap manually at ~13 chars per line
        // (each char is 6px; column is ~80px wide → ~13 chars)
        const char* desc = str(kPerks[id].desc);
        int dy = iconY + PC_COL_ICON_R + 18;
        u16 descCol = sel ? RGB15(18, 18, 22) : RGB15(12, 12, 16);

        char line[14];
        int li = 0;
        int ci = 0;
        while (desc[ci]) {
            if (li >= 13 || !desc[ci]) {
                // Back-track to last space if possible
                int back = li;
                while (back > 0 && line[back - 1] != ' ') back--;
                int lineLen = (back > 0 && desc[ci]) ? back : li;
                line[lineLen] = '\0';
                int lw = renderTextWidth(line);
                renderText(cx - lw / 2, dy, line, descCol);
                dy += 9;
                // Restart from cut point
                if (back > 0 && desc[ci]) ci -= (li - back);
                li = 0;
            }
            if (desc[ci]) line[li++] = desc[ci++];
        }
        if (li > 0) {
            line[li] = '\0';
            int lw = renderTextWidth(line);
            renderText(cx - lw / 2, dy, line, descCol);
        }
    }
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void perkChoiceRender() {
    // Top screen: always render (double-buffered, no flicker)
    renderTopScreen();

    // Bottom screen: only redraw when data changes (single-buffered, would flicker otherwise)
    if (sDirty) {
        renderBottomScreen();
        sDirty = false;
    }
}
