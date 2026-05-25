#include "ui.h"
#include "shop.h"
#include "strings.h"
#include "player.h"
#include "game.h"
#include "render.h"
#include "companion.h"
#include "synergy.h"
#include <nds.h>
#include <stdio.h>

void uiInit() {
    // Switch sub screen to 16-bit bitmap mode so we can draw colored buttons.
    // renderInitSub() sets MODE_5_2D + VRAM_C_SUB_BG + bgInitSub(3,Bmp16).
    renderInitSub();
}

void uiUpdate() {
    // Reserved for future per-frame sub-screen logic.
}

// Force next HUD/shop draw to redraw everything (call on state transitions)
static bool sForceRedraw = false;
void uiForceRedraw() { sForceRedraw = true; }

// ---------------------------------------------------------------------------
// HUD — drawn onto sub-screen bitmap during gameplay
// ---------------------------------------------------------------------------
void uiRenderHUD() {
    static s16 prevHp       = -1;
    static s16 prevMaxHp    = -1;
    static u16 prevGold     = 0xFFFF;
    static int prevWave     = -1;
    static int prevDashState      = -1;
    static int prevWaveClear      = -1;
    static int prevBossDefeated   = -1;
    static bool firstCall         = true;
    if (sForceRedraw) { firstCall = true; sForceRedraw = false; }
    // Companion / synergy dirty tracking: hash of (hp, maxHp, active, color) per slot
    static u32 prevCompHash = 0;

    int curWave = gameGetWave();
    // Dash state: quantized to ~6 steps during cooldown (not per-frame) to limit redraws
    int curDashState;
    if (gPlayer.isDashing)              curDashState = 1;
    else if (gPlayer.dashCooldown > 0)  curDashState = 2 + gPlayer.dashCooldown / 15;
    else                                curDashState = 0;
    // Quantize countdowns to avoid per-frame redraws
    int curWaveClear    = gameGetWaveClearTimer() / 15;
    int curBossDefeated = gameGetBossDefeatedTimer() / 15;

    // Build a lightweight hash over companion state for dirty detection
    u32 curCompHash = 0;
    for (int i = 0; i < MAX_COMPANIONS; i++) {
        const Companion& c = gCompanions[i];
        curCompHash ^= static_cast<u32>(c.active ? 1 : 0) << (i * 5);
        if (c.active) {
            curCompHash += static_cast<u32>(c.hp) * 7u + static_cast<u32>(c.maxHp) * 31u
                         + static_cast<u32>(c.color) * 97u;
        }
    }

    bool changed = firstCall
        || gPlayer.hp       != prevHp
        || gPlayer.maxHp    != prevMaxHp
        || gPlayer.gold     != prevGold
        || curWave          != prevWave
        || curDashState     != prevDashState
        || curWaveClear     != prevWaveClear
        || curBossDefeated  != prevBossDefeated
        || curCompHash      != prevCompHash;

    if (!changed) return;

    firstCall       = false;
    prevHp          = gPlayer.hp;
    prevMaxHp       = gPlayer.maxHp;
    prevGold        = gPlayer.gold;
    prevWave        = curWave;
    prevDashState   = curDashState;
    prevWaveClear   = curWaveClear;
    prevBossDefeated = curBossDefeated;
    prevCompHash    = curCompHash;

    // Dark background panel
    renderClearSub();
    renderFilledRectSub(0, 0, 256, 192, RGB15(2, 2, 4));

    // Title bar
    renderFilledRectSub(0, 0, 256, 12, RGB15(4, 4, 12));
    renderTextSub(4, 3, "TRAIL MIX", RGB15(20, 20, 31));

    // Wave
    char waveBuf[16];
    snprintf(waveBuf, sizeof(waveBuf), "%s %d", str(kUI[7]), curWave);
    renderTextSub(160, 3, waveBuf, RGB15(20, 20, 31));

    // HP bar background + fill
    renderFilledRectSub(4, 18, 120, 10, RGB15(8, 2, 2));
    int hpFill = (gPlayer.maxHp > 0)
        ? (static_cast<int>(gPlayer.hp) * 120) / static_cast<int>(gPlayer.maxHp)
        : 0;
    if (hpFill < 0) hpFill = 0;
    if (hpFill > 120) hpFill = 120;
    renderFilledRectSub(4, 18, hpFill, 10, RGB15(0, 22, 0));
    char hpBuf[20];
    snprintf(hpBuf, sizeof(hpBuf), "%s %d/%d", str(kUI[9]), gPlayer.hp, gPlayer.maxHp);
    renderTextSub(6, 19, hpBuf, RGB15(31, 31, 31));

    // Gold
    char goldBuf[16];
    snprintf(goldBuf, sizeof(goldBuf), "%s: %d", str(kUI[8]), gPlayer.gold);
    renderTextSub(4, 34, goldBuf, RGB15(31, 28, 0));

    // Dash indicator — bar + text
    if (gPlayer.isDashing) {
        renderTextSub(4, 46, str(kUI[51]), RGB15(20, 31, 20));
    } else if (gPlayer.dashCooldown > 0) {
        renderTextSub(4, 46, str(kUI[51]), RGB15(14, 14, 14));
        // Cooldown bar: fills left to right as cooldown expires
        int barW = 40;
        int filled = barW * (90 - gPlayer.dashCooldown) / 90;
        renderFilledRectSub(36, 47, barW, 4, RGB15(6, 6, 6));
        if (filled > 0) renderFilledRectSub(36, 47, filled, 4, RGB15(14, 14, 20));
    } else {
        renderTextSub(4, 46, str(kUI[51]), RGB15(10, 31, 10));
        renderFilledRectSub(36, 47, 40, 4, RGB15(10, 31, 10));
    }

    // -------------------------------------------------------------------------
    // Active synergies — y=60, one row, left-to-right, skip colors with <2
    // Each badge: 8x8 colored square | count digit | synergy name (max 8 chars)
    // -------------------------------------------------------------------------
    {
        // Count companions per color
        int colorCount[PILL_COLOR_COUNT] = {};
        for (int i = 0; i < MAX_COMPANIONS; i++) {
            if (gCompanions[i].active)
                colorCount[static_cast<int>(gCompanions[i].color)]++;
        }

        int sx = 4;
        const int SYN_Y = 60;
        for (int ci = 0; ci < PILL_COLOR_COUNT; ci++) {
            int cnt = colorCount[ci];
            if (cnt < 2) continue;

            u16 col = pillColorToRGB(static_cast<PillColor>(ci));

            // Colored 8x8 square badge
            renderFilledRectSub(sx, SYN_Y, 8, 8, col);
            // Darken interior leaving 1px border
            renderFilledRectSub(sx + 1, SYN_Y + 1, 6, 6, RGB15(4, 4, 6));

            // Count digit centered in the badge
            char cntBuf[2] = { static_cast<char>('0' + cnt), '\0' };
            renderTextSub(sx + 2, SYN_Y + 1, cntBuf, col);

            sx += 10; // gap after badge

            // Synergy name — tier index is cnt-2, clamped to SYN_TIERS-1
            int tier = cnt - 2;
            if (tier >= SYN_TIERS) tier = SYN_TIERS - 1;
            const char* synName = str(kSynergyNames[ci][tier]);

            // Truncate to 8 characters so badges don't overflow
            char nameBuf[9];
            int nc = 0;
            while (synName[nc] && nc < 8) { nameBuf[nc] = synName[nc]; nc++; }
            nameBuf[nc] = '\0';

            renderTextSub(sx, SYN_Y + 1, nameBuf, col);

            // Advance by text width (6px per char) + 6px gap
            sx += nc * 6 + 6;

            if (sx >= 250) break; // no room for more
        }

        // Active synergy effect description — compact line below badges
        // Shows the actual effect description from kSynergyDesc.
        // If multiple synergies are active, cycle between them every ~3s.
        const int DESC_Y = 78;
        static const char kLabel[PILL_COLOR_COUNT] = { 'R', 'B', 'G', 'Y', 'P', 'C' };

        // Collect active synergies (color index + tier)
        int activeSyn[PILL_COLOR_COUNT];
        int activeCount = 0;
        for (int ci = 0; ci < PILL_COLOR_COUNT; ci++) {
            int cnt = colorCount[ci];
            if (cnt < 2) continue;
            activeSyn[activeCount++] = ci;
        }

        if (activeCount > 0) {
            // Cycle through active synergies every ~180 frames (~3s at 60fps)
            static int synCycleFrame = 0;
            synCycleFrame++;
            int showIdx = (synCycleFrame / 180) % activeCount;
            int ci = activeSyn[showIdx];

            int tier = colorCount[ci] - 2;
            if (tier >= SYN_TIERS) tier = SYN_TIERS - 1;

            u16 col = pillColorToRGB(static_cast<PillColor>(ci));
            const char* desc = str(kSynergyDesc[ci][tier]);

            // "R2:" prefix
            int dx = 4;
            char prefix[4] = { kLabel[ci], static_cast<char>('1' + tier), ':', '\0' };
            renderTextSub(dx, DESC_Y, prefix, col);
            dx += 3 * 6; // 3 chars × 6px

            // Description, truncated to 39 chars (256px - 18px prefix = 238px / 6px)
            char dBuf[40];
            int nc = 0;
            while (desc[nc] && nc < 39) { dBuf[nc] = desc[nc]; nc++; }
            dBuf[nc] = '\0';
            renderTextSub(dx, DESC_Y, dBuf, RGB15(22, 22, 22));
        }
    }

    // -------------------------------------------------------------------------
    // Companion HP bars — y=100, one entry per active companion
    // [6x6 color square] [30px HP bar]   up to 6 companions, 40px pitch
    // -------------------------------------------------------------------------
    {
        const int CP_Y = 100;
        const int CP_PITCH = 40;
        int cx = 4;
        for (int i = 0; i < MAX_COMPANIONS; i++) {
            if (!gCompanions[i].active) continue;
            const Companion& c = gCompanions[i];
            u16 col = pillColorToRGB(c.color);

            // 6x6 colored icon
            renderFilledRectSub(cx, CP_Y, 6, 6, col);

            // HP bar background (30x4)
            renderFilledRectSub(cx + 7, CP_Y + 1, 30, 4, RGB15(6, 2, 2));

            // HP fill
            int fill = (c.maxHp > 0)
                ? (static_cast<int>(c.hp) * 30) / static_cast<int>(c.maxHp)
                : 0;
            if (fill < 0) fill = 0;
            if (fill > 30) fill = 30;
            renderFilledRectSub(cx + 7, CP_Y + 1, fill, 4, col);

            cx += CP_PITCH;
            if (cx + CP_PITCH > 256) break;
        }
    }

    // Wave-clear banner (normal waves)
    if (curWaveClear > 0 && curBossDefeated == 0) {
        int tw = renderTextWidth(str(kGameplay[0]));
        renderFilledRectSub(64, 88, 128, 16, RGB15(4, 12, 4));
        renderTextSub(128 - tw / 2, 92, str(kGameplay[0]), RGB15(10, 31, 10));
    }

    // "BOSS DEFEATED!" banner (boss waves — shown while bossDefeatedTimer counts down)
    if (curBossDefeated > 0) {
        const char* bd = str(kGameplay[10]); // "BOSS DEFEATED!"
        int bdw = renderTextWidth(bd);
        renderFilledRectSub(32, 80, 192, 32, RGB15(12, 4, 4));
        // 2px golden border
        u16 gold = RGB15(31, 24, 0);
        renderFilledRectSub(32,       80,  192, 2,  gold);
        renderFilledRectSub(32,       110, 192, 2,  gold);
        renderFilledRectSub(32,       80,  2,   32, gold);
        renderFilledRectSub(222,      80,  2,   32, gold);
        renderTextSub(128 - bdw / 2, 88, bd, RGB15(31, 28, 4));
        // Sub-line: "PERK INCOMING..."
        const char* sub = str(kUI[52]);
        int sw = renderTextWidth(sub);
        renderTextSub(128 - sw / 2, 100, sub, RGB15(20, 18, 4));
    }

    // ── Synergy HUD strip ────────────────────────────────────────────────────
    // Six colored squares at y=170, one per PillColor.
    // Each is 11x11px with a 1px dark border.
    // Active tier (>=0): full color + tier number inside.
    // Inactive (tier==-1): dim/dark + letter label only.
    //
    //   [R:2] [B:-] [G:3] [Y:-] [P:-] [C:1]
    //
    // Layout: 6 squares × 12px stride, centered at x=128.
    // Starting x = 128 - (6 * 12) / 2 = 92
    static const u16 kSynColor[PILL_COLOR_COUNT] = {
        RGB15(28,  4,  4),  // Red
        RGB15( 2, 10, 28),  // Blue
        RGB15( 2, 22,  2),  // Green
        RGB15(28, 24,  0),  // Yellow
        RGB15(18,  0, 28),  // Purple
        RGB15( 0, 22, 22),  // Cyan
    };
    static const u16 kSynDim[PILL_COLOR_COUNT] = {
        RGB15( 8,  2,  2),  // Red dim
        RGB15( 1,  3,  8),  // Blue dim
        RGB15( 1,  6,  1),  // Green dim
        RGB15( 8,  6,  0),  // Yellow dim
        RGB15( 5,  0,  8),  // Purple dim
        RGB15( 0,  6,  6),  // Cyan dim
    };
    static const char kSynLabel[PILL_COLOR_COUNT] = { 'R', 'B', 'G', 'Y', 'P', 'C' };

    int stripX = 92;
    int stripY = 170;
    for (int c = 0; c < PILL_COLOR_COUNT; ++c) {
        int sx = stripX + c * 27;  // 27px stride for 11px square + 3px label + gaps
        s8 tier = gSynergy.tier[c];
        bool active = (tier >= 0);

        // Dark border
        renderFilledRectSub(sx - 1, stripY - 1, 13, 13, RGB15(2, 2, 2));
        // Color fill
        renderFilledRectSub(sx, stripY, 11, 11, active ? kSynColor[c] : kSynDim[c]);

        // Tier number inside (or '-' if inactive)
        char tierChar = active ? static_cast<char>('0' + tier + 1) : '-';
        char tierStr[2] = { tierChar, '\0' };
        u16 textCol = active ? RGB15(31, 31, 31) : RGB15(10, 10, 10);
        renderTextSub(sx + 3, stripY + 2, tierStr, textCol);

        // Color letter below the square
        char labelStr[2] = { kSynLabel[c], '\0' };
        renderTextSub(sx + 3, stripY + 13, labelStr, active ? kSynColor[c] : kSynDim[c]);
    }
}

// ---------------------------------------------------------------------------
// Shop screen — delegates to shop.cpp which calls renderFilledRectSub etc.
// ---------------------------------------------------------------------------
void uiRenderShop() {
    shopRender();
}

void uiRenderShopScreen() {
    shopRender();
}
