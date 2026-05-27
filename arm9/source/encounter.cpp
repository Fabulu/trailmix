#include "encounter.h"
#include "sayings.h"
#include "save.h"
#include "render.h"
#include "text_util.h"
#include "strings.h"
#include <nds.h>

// ---------------------------------------------------------------------------
// Layout constants
// ---------------------------------------------------------------------------

// Top screen text reading (phase 1)
static constexpr int ENC_MARGIN_X     = 8;
static constexpr int ENC_HEADER_Y     = 4;
static constexpr int ENC_SEP_Y        = 16;
static constexpr int ENC_TEXT_Y       = 20;
static constexpr int ENC_TEXT_WIDTH   = 240;   // 256 - 2*8
static constexpr int ENC_LINE_H      = 10;    // 7px glyph + 3px gap
static constexpr int ENC_VISIBLE     = 16;    // (192 - 20 - 8) / 10

// Colors
static constexpr u16 COL_BG          = RGB15(1, 1, 4);
static constexpr u16 COL_GOLD        = RGB15(31, 28, 0);
static constexpr u16 COL_DIM         = RGB15(14, 14, 14);
static constexpr u16 COL_TEXT        = RGB15(28, 28, 24);
static constexpr u16 COL_HINT        = RGB15(10, 10, 14);
static constexpr u16 COL_SCROLL_IND  = RGB15(20, 20, 20);

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static EncounterResult sResult;
static int  sPhase;          // 0 = announcement, 1 = reading text
static int  sScroll;         // text scroll offset (lines)
static bool sDirty;          // redraw flag for bottom screen
static int  sTotalLines;     // total wrapped lines of encounter text
static int  sTextLen;        // length of encounter text (to '\n')

// ---------------------------------------------------------------------------
// Helper: render master name (stops at ' ===' or '\n')
// ---------------------------------------------------------------------------

// Helper: truncate a name buffer so its pixel width fits within maxPx.
// Appends "..." if truncated.  Buffer must have room for 3 extra chars + NUL.
static void truncateToFit(char* buf, int len, int maxPx) {
    // Quick check: if it already fits, nothing to do
    if (renderTextWidth(buf) <= maxPx) return;

    // Binary-ish search: shorten until it fits with "..."
    for (int cut = len - 1; cut > 0; cut--) {
        buf[cut] = '.'; buf[cut + 1] = '.'; buf[cut + 2] = '.'; buf[cut + 3] = '\0';
        if (renderTextWidth(buf) <= maxPx) return;
    }
    // Absolute fallback
    buf[0] = '.'; buf[1] = '.'; buf[2] = '.'; buf[3] = '\0';
}

static void renderMasterName(int x, int y, int masterId, u16 color,
                             void (*renderFn)(int, int, const char*, u16)) {
    int nameLen = sayingsMasterNameLen(masterId);
    const char* name = kMasters[masterId].name;

    char nameBuf[MASTER_NAME_MAX + 4];
    int copyLen = nameLen < MASTER_NAME_MAX - 1 ? nameLen : MASTER_NAME_MAX - 1;
    for (int i = 0; i < copyLen; i++) nameBuf[i] = name[i];
    nameBuf[copyLen] = '\0';

    truncateToFit(nameBuf, copyLen, 256 - x - 4);

    renderFn(x, y, nameBuf, color);
}

// Overload for centered rendering on top screen
static void renderMasterNameCentered(int y, int masterId, u16 color) {
    int nameLen = sayingsMasterNameLen(masterId);
    const char* name = kMasters[masterId].name;

    char nameBuf[MASTER_NAME_MAX + 4]; // extra room for "..."
    int copyLen = nameLen < MASTER_NAME_MAX - 1 ? nameLen : MASTER_NAME_MAX - 1;
    for (int i = 0; i < copyLen; i++) nameBuf[i] = name[i];
    nameBuf[copyLen] = '\0';

    truncateToFit(nameBuf, copyLen, 248); // 256 - 8 margin

    int tw = renderTextWidth(nameBuf);
    renderText(128 - tw / 2, y, nameBuf, color);
}

// Same but for sub screen
static void renderMasterNameCenteredSub(int y, int masterId, u16 color) {
    int nameLen = sayingsMasterNameLen(masterId);
    const char* name = kMasters[masterId].name;

    char nameBuf[MASTER_NAME_MAX + 4];
    int copyLen = nameLen < MASTER_NAME_MAX - 1 ? nameLen : MASTER_NAME_MAX - 1;
    for (int i = 0; i < copyLen; i++) nameBuf[i] = name[i];
    nameBuf[copyLen] = '\0';

    truncateToFit(nameBuf, copyLen, 248);

    int tw = renderTextWidth(nameBuf);
    renderTextSub(128 - tw / 2, y, nameBuf, color);
}

// ---------------------------------------------------------------------------
// encounterEnter
// ---------------------------------------------------------------------------

void encounterEnter() {
    sResult   = sayingsRollEncounter();
    sPhase    = 0;
    sScroll   = 0;
    sDirty    = true;
    sTotalLines = 0;
    sTextLen  = sayingsEncounterTextLen(sResult.masterId, sResult.encounter);
}

// ---------------------------------------------------------------------------
// encounterUpdate
// ---------------------------------------------------------------------------

bool encounterUpdate() {
    u32 kd = keysDown();

    if (sPhase == 0) {
        // Phase 0: announcement -- wait for A
        if (kd & KEY_A) {
            sPhase = 1;
            sScroll = 0;
            sDirty = true;
        }
        return false;
    }

    // Phase 1: text reading
    if (kd & KEY_DOWN) {
        int maxScroll = sTotalLines - ENC_VISIBLE;
        if (maxScroll < 0) maxScroll = 0;
        if (sScroll < maxScroll) {
            sScroll++;
            sDirty = true;
        }
    }
    if (kd & KEY_UP) {
        if (sScroll > 0) {
            sScroll--;
            sDirty = true;
        }
    }

    if (kd & (KEY_A | KEY_B)) {
        // Finish reading
        if (!sResult.isDuplicate) {
            sayingsMarkFound(sResult);
            saveWrite();
        }
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// encounterRender — top screen, phase 0 (announcement)
// ---------------------------------------------------------------------------

static void renderTopPhase0() {
    renderClearParticleLayer();

    // Hide OAM sprites
    for (int i = 0; i < 128; i++)
        oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, nullptr, -1, false, true, false, false, false);
    oamUpdate(&oamMain);

    // Dark background
    renderFilledRect(0, 0, 256, 192, COL_BG);

    if (sResult.isDuplicate) {
        // Duplicate: "A FAMILIAR VOICE..."
        const char* familiar = str(kSayingsUI[SAY_FAMILIAR]);
        int fw = renderTextWidth(familiar);
        renderText(128 - fw / 2, 60, familiar, COL_DIM);

        // Master name below
        renderMasterNameCentered(82, sResult.masterId, COL_GOLD);
    } else {
        // New encounter: "YOU FOUND [NAME]'S SAYINGS!"
        const char* youFound = str(kSayingsUI[SAY_YOU_FOUND]);
        int yfw = renderTextWidth(youFound);
        renderText(128 - yfw / 2, 50, youFound, COL_GOLD);

        // Master name
        renderMasterNameCentered(68, sResult.masterId, COL_GOLD);

        // "'S SAYINGS!"
        const char* suffix = str(kSayingsUI[SAY_S_SAYINGS]);
        int sw = renderTextWidth(suffix);
        renderText(128 - sw / 2, 86, suffix, COL_GOLD);
    }

    // "PRESS A TO READ"
    const char* pressA = str(kSayingsUI[SAY_PRESS_A]);
    int pw = renderTextWidth(pressA);
    renderText(128 - pw / 2, 140, pressA, COL_HINT);
}

// ---------------------------------------------------------------------------
// encounterRender — top screen, phase 1 (text reading)
// ---------------------------------------------------------------------------

static void renderTopPhase1() {
    renderClearParticleLayer();

    // Hide OAM sprites
    for (int i = 0; i < 128; i++)
        oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, nullptr, -1, false, true, false, false, false);
    oamUpdate(&oamMain);

    // Dark background
    renderFilledRect(0, 0, 256, 192, COL_BG);

    // Header: master name left-aligned
    renderMasterName(ENC_MARGIN_X, ENC_HEADER_Y, sResult.masterId, COL_GOLD, renderText);

    // Separator line
    renderFilledRect(0, ENC_SEP_Y, 256, 1, RGB15(8, 8, 24));

    // Word-wrapped encounter text with scroll (decrypt on demand)
    const char* text = sayingsDecryptEncounter(sResult.masterId, sResult.encounter);
    if (text) {
        sTotalLines = textRenderWrapped(ENC_MARGIN_X, ENC_TEXT_Y,
                                         text, sTextLen,
                                         ENC_TEXT_WIDTH, COL_TEXT,
                                         sScroll, ENC_VISIBLE);
    }

    // Scroll indicators
    if (sScroll > 0)
        renderText(124, ENC_SEP_Y - 6, "^", COL_SCROLL_IND);
    if (sTotalLines > ENC_VISIBLE && sScroll < sTotalLines - ENC_VISIBLE)
        renderText(124, 184, "V", COL_SCROLL_IND);
}

// ---------------------------------------------------------------------------
// encounterRender — bottom screen
// ---------------------------------------------------------------------------

static void renderBottomScreen() {
    renderClearSub();
    renderFilledRectSub(0, 0, 256, 192, COL_BG);

    if (sPhase == 0) {
        // Phase 0: simple prompt
        const char* readPrompt = str(kSayingsUI[SAY_A_READ]);
        int rw = renderTextWidth(readPrompt);
        renderTextSub(128 - rw / 2, 90, readPrompt, COL_GOLD);
    } else {
        // Phase 1: navigation hints + encounter info

        // Master name
        renderMasterNameCenteredSub(20, sResult.masterId, COL_GOLD);

        // Encounter number (E1 or E2): determined at roll time
        const char* encLabel;
        if (sResult.encounter == 1) {
            encLabel = str(kSayingsUI[SAY_ENCOUNTER2]);
        } else {
            encLabel = str(kSayingsUI[SAY_ENCOUNTER1]);
        }
        int ew = renderTextWidth(encLabel);
        renderTextSub(128 - ew / 2, 44, encLabel, COL_DIM);

        // Navigation hints
        const char* scrollHint = str(kSayingsUI[SAY_SCROLL]);
        int shw = renderTextWidth(scrollHint);
        renderTextSub(128 - shw / 2, 130, scrollHint, COL_HINT);

        // "B: BACK" repurposed as "A/B: CONTINUE"
        // Use a simple combined hint since there's no dedicated string
        const char* backHint = str(kSayingsUI[SAY_B_BACK]);
        int bhw = renderTextWidth(backHint);
        renderTextSub(128 - bhw / 2, 150, backHint, COL_HINT);

        // Scroll position indicator if scrollable
        if (sTotalLines > ENC_VISIBLE) {
            // Manual int-to-str for line range display
            char posBuf[32];
            int pi = 0;

            // Start line
            int startLine = sScroll + 1;
            char tmp[8];
            int ti = 0;
            if (startLine == 0) { tmp[ti++] = '0'; }
            else { int v = startLine; while (v > 0) { tmp[ti++] = '0' + (v % 10); v /= 10; } }
            for (int j = ti - 1; j >= 0; j--) posBuf[pi++] = tmp[j];

            posBuf[pi++] = '-';

            // End line
            int endLine = sScroll + ENC_VISIBLE;
            if (endLine > sTotalLines) endLine = sTotalLines;
            ti = 0;
            if (endLine == 0) { tmp[ti++] = '0'; }
            else { int v = endLine; while (v > 0) { tmp[ti++] = '0' + (v % 10); v /= 10; } }
            for (int j = ti - 1; j >= 0; j--) posBuf[pi++] = tmp[j];

            posBuf[pi++] = '/';

            // Total lines
            ti = 0;
            if (sTotalLines == 0) { tmp[ti++] = '0'; }
            else { int v = sTotalLines; while (v > 0) { tmp[ti++] = '0' + (v % 10); v /= 10; } }
            for (int j = ti - 1; j >= 0; j--) posBuf[pi++] = tmp[j];

            posBuf[pi] = '\0';

            int pw2 = renderTextWidth(posBuf);
            renderTextSub(128 - pw2 / 2, 170, posBuf, COL_DIM);
        }
    }
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void encounterRender() {
    // Top screen: always render (double-buffered)
    if (sPhase == 0) {
        renderTopPhase0();
    } else {
        renderTopPhase1();
    }

    // Bottom screen: only redraw when dirty (single-buffered)
    if (sDirty) {
        renderBottomScreen();
        sDirty = false;
    }

    // Wipe decrypted encounter text from RAM after rendering
    sayingsWipeBuffer();
}
