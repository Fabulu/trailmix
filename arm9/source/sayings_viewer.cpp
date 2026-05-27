#include "sayings_viewer.h"
#include "sayings.h"
#include "render.h"
#include "text_util.h"
#include "strings.h"
#include <nds.h>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

// Navigation
static int sLevel;              // 0 = master list, 1 = encounter select, 2 = reading

// Level 0: Master list
static int sCursor;             // index into sSortedIds[] (which entry is highlighted)
static int sListScroll;         // first visible entry in the list (scroll offset)
static int sFilter;             // 0 = ALL, 1 = FOUND, 2 = NOT FOUND
static int sSortedAll[MASTER_COUNT]; // full alphabetical list (base for filtering)
static int sSortedAllCount;
static int sSortedIds[MASTER_COUNT]; // filtered + sorted master IDs
static int sSortedCount;        // number of entries in sSortedIds after filtering
static bool sDirty;             // redraw flag for bottom screen

// Level 1: Encounter select
static int sEncSelect;          // 0 = encounter 1, 1 = encounter 2

// Level 2: Reading
static int sReadMasterId;       // which master we're reading
static int sReadEncounter;      // 0 = E1, 1 = E2
static int sReadScroll;         // text scroll offset (in lines)
static int sReadTotalLines;     // total wrapped lines of current text

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static const int VISIBLE_CARDS  = 8;
static const int CARD_HEIGHT    = 20;
static const int HEADER_H       = 14;
static const int LIST_Y         = 16;
static const int FOOTER_Y       = 176;

// Level 2 reading layout (top screen)
static const int READ_MARGIN_X  = 8;
static const int READ_TEXT_Y    = 18;
static const int READ_WIDTH     = 240;  // 256 - 2*8
static const int READ_VISIBLE   = 16;   // (183 - 18) / 10

// Colors
static const u16 COL_GOLD       = RGB15(31, 28, 0);
static const u16 COL_GOLD_DIM   = RGB15(14, 12, 0);
static const u16 COL_GRAY       = RGB15(10, 10, 10);
static const u16 COL_WHITE      = RGB15(28, 28, 24);
static const u16 COL_WHITE_DIM  = RGB15(16, 16, 16);
static const u16 COL_HINT       = RGB15(10, 10, 14);
static const u16 COL_GREEN      = RGB15(8, 24, 8);
static const u16 COL_BG_SEL     = RGB15(4, 3, 0);
static const u16 COL_BG_DARK    = RGB15(1, 1, 1);

// ---------------------------------------------------------------------------
// Helper: render master name (length-limited, from ROM pointer)
// ---------------------------------------------------------------------------

static void renderMasterName(int x, int y, int id, u16 color, bool useSub) {
    int len = sayingsMasterNameLen(id);
    if (len <= 0) return;
    char buf[MASTER_NAME_MAX];
    if (len > MASTER_NAME_MAX - 1) len = MASTER_NAME_MAX - 1;
    memcpy(buf, kMasters[id].name, len);
    buf[len] = '\0';
    if (useSub)
        renderTextSub(x, y, buf, color);
    else
        renderText(x, y, buf, color);
}

static int masterNameWidth(int id) {
    int len = sayingsMasterNameLen(id);
    if (len <= 0) return 0;
    char buf[MASTER_NAME_MAX];
    if (len > MASTER_NAME_MAX - 1) len = MASTER_NAME_MAX - 1;
    memcpy(buf, kMasters[id].name, len);
    buf[len] = '\0';
    return renderTextWidth(buf);
}

// ---------------------------------------------------------------------------
// Helper: int to string (no snprintf dependency for simple counters)
// ---------------------------------------------------------------------------

static void intToStr(int val, char* buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[8];
    int i = 0;
    bool neg = false;
    if (val < 0) { neg = true; val = -val; }
    while (val > 0 && i < 7) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int j = 0;
    if (neg) buf[j++] = '-';
    for (int k = i - 1; k >= 0; k--) buf[j++] = tmp[k];
    buf[j] = '\0';
}

// ---------------------------------------------------------------------------
// Filter rebuild
// ---------------------------------------------------------------------------

static void rebuildFilteredList() {
    sSortedCount = 0;
    for (int i = 0; i < sSortedAllCount; i++) {
        int id = sSortedAll[i];
        bool found = sayingsGetBits(id) > 0;
        if (sFilter == 0) {
            // ALL
            sSortedIds[sSortedCount++] = id;
        } else if (sFilter == 1 && found) {
            // FOUND
            sSortedIds[sSortedCount++] = id;
        } else if (sFilter == 2 && !found) {
            // NOT FOUND
            sSortedIds[sSortedCount++] = id;
        }
    }
    sCursor = 0;
    sListScroll = 0;
    sDirty = true;
}

// ---------------------------------------------------------------------------
// Enter
// ---------------------------------------------------------------------------

void sayingsViewerEnter() {
    sLevel = 0;
    sCursor = 0;
    sListScroll = 0;
    sFilter = 0;
    sEncSelect = 0;
    sReadScroll = 0;
    sReadTotalLines = 0;
    sDirty = true;

    // Build full alphabetical list (used as base for all filters)
    sayingsGetSortedIds(sSortedAll, &sSortedAllCount, false);
    // Initial filtered list = ALL
    memcpy(sSortedIds, sSortedAll, sSortedAllCount * sizeof(int));
    sSortedCount = sSortedAllCount;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

static bool updateLevel0() {
    u32 kd = keysDown();

    // B: exit viewer
    if (kd & KEY_B) {
        return true;
    }

    // L/R: cycle filter
    if (kd & KEY_R) {
        sFilter = (sFilter + 1) % 3;
        rebuildFilteredList();
    }
    if (kd & KEY_L) {
        sFilter = (sFilter + 2) % 3; // backwards: +2 mod 3
        rebuildFilteredList();
    }

    // D-pad: move cursor
    if (kd & KEY_DOWN) {
        if (sSortedCount > 0 && sCursor < sSortedCount - 1) {
            sCursor++;
            if (sCursor >= sListScroll + VISIBLE_CARDS)
                sListScroll = sCursor - VISIBLE_CARDS + 1;
            sDirty = true;
        }
    }
    if (kd & KEY_UP) {
        if (sCursor > 0) {
            sCursor--;
            if (sCursor < sListScroll)
                sListScroll = sCursor;
            sDirty = true;
        }
    }

    // A: select found master
    if (kd & KEY_A) {
        if (sSortedCount > 0 && sCursor < sSortedCount) {
            int id = sSortedIds[sCursor];
            u8 bits = sayingsGetBits(id);
            if (bits > 0) {
                // Found master
                bool hasE2Found = (bits == 3);
                if (hasE2Found) {
                    // 2 encounters found: go to encounter select
                    sLevel = 1;
                    sEncSelect = 0;
                    sDirty = true;
                } else {
                    // Only E1 found: go straight to reading
                    sLevel = 2;
                    sReadMasterId = id;
                    sReadEncounter = 0;
                    sReadScroll = 0;
                    sReadTotalLines = 0;
                    sDirty = true;
                }
            }
            // Unfound: do nothing
        }
    }

    return false;
}

static void updateLevel1() {
    u32 kd = keysDown();

    if (kd & KEY_B) {
        sLevel = 0;
        sDirty = true;
        return;
    }

    if (kd & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
        sEncSelect ^= 1;
        sDirty = true;
    }

    if (kd & KEY_A) {
        int id = sSortedIds[sCursor];
        sLevel = 2;
        sReadMasterId = id;
        sReadEncounter = sEncSelect;
        sReadScroll = 0;
        sReadTotalLines = 0;
        sDirty = true;
    }
}

static void updateLevel2() {
    u32 kd = keysDown();

    if (kd & KEY_B) {
        // Back to level 1 if master has 2 found encounters, otherwise level 0
        u8 bits = sayingsGetBits(sReadMasterId);
        if (bits == 3) {
            sLevel = 1;
        } else {
            sLevel = 0;
        }
        sDirty = true;
        return;
    }

    // Scroll text
    if (kd & KEY_DOWN) {
        int maxScroll = sReadTotalLines - READ_VISIBLE;
        if (maxScroll < 0) maxScroll = 0;
        if (sReadScroll < maxScroll) {
            sReadScroll++;
            sDirty = true;
        }
    }
    if (kd & KEY_UP) {
        if (sReadScroll > 0) {
            sReadScroll--;
            sDirty = true;
        }
    }

    // L/R: switch encounter if both found
    u8 bits = sayingsGetBits(sReadMasterId);
    if (bits == 3 && (kd & (KEY_L | KEY_R))) {
        sReadEncounter ^= 1;
        sReadScroll = 0;
        sReadTotalLines = 0;
        sDirty = true;
    }
}

bool sayingsViewerUpdate() {
    switch (sLevel) {
        case 0: return updateLevel0();
        case 1: updateLevel1(); break;
        case 2: updateLevel2(); break;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Render: Level 0 (Master List)
// ---------------------------------------------------------------------------

static void hideOamSprites() {
    for (int i = 0; i < 128; i++)
        oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, nullptr, -1, false, true, false, false, false);
    oamUpdate(&oamMain);
}

static void renderLevel0Top() {
    renderClearParticleLayer();
    hideOamSprites();

    // Title: "SAYINGS X/448"
    char title[40];
    char numBuf[8];
    intToStr(sayingsFoundCount(), numBuf);
    snprintf(title, sizeof(title), "%s %s/%d", str(kSayingsUI[SAY_TITLE]), numBuf, MASTER_COUNT);
    int tw = renderTextWidth(title);
    renderText(128 - tw / 2, 4, title, COL_GOLD);

    // Separator
    renderFilledRect(16, 22, 224, 1, COL_GOLD_DIM);

    // Preview of selected master
    if (sSortedCount > 0 && sCursor < sSortedCount) {
        int id = sSortedIds[sCursor];
        u8 bits = sayingsGetBits(id);

        if (bits > 0) {
            // Found: show name in gold
            int nw = masterNameWidth(id);
            renderMasterName(128 - nw / 2, 30, id, COL_GOLD, false);

            // Status line
            int encLevel = sayingsGetEncounterLevel(id);
            char statusBuf[32];
            char lvlBuf[4];
            intToStr(encLevel, lvlBuf);
            // "ENCOUNTERED: X/2" or "ENCOUNTERED: X/1"
            bool hasE2 = (kMasters[id].e2 != nullptr || kMasters[id].e2_de != nullptr);
            snprintf(statusBuf, sizeof(statusBuf), "%s: %s/%d", str(kSayingsUI[SAY_FOUND]), lvlBuf, hasE2 ? 2 : 1);
            int sw = renderTextWidth(statusBuf);
            renderText(128 - sw / 2, 44, statusBuf, COL_WHITE_DIM);

            // Preview: first few lines of E1 text
            const char* e1 = sayingsGetE1(id);
            if (e1) {
                int eLen = sayingsEncounterLen(e1);
                textRenderWrapped(READ_MARGIN_X, 60, e1, eLen, READ_WIDTH, COL_WHITE_DIM, 0, 12);
            }
        } else {
            // Unfound: show "???"
            const char* unk = str(kSayingsUI[SAY_UNKNOWN]);
            int uw = renderTextWidth(unk);
            renderText(128 - uw / 2, 30, unk, COL_GRAY);

            const char* nf = str(kSayingsUI[SAY_NOT_FOUND]);
            int nfw = renderTextWidth(nf);
            renderText(128 - nfw / 2, 44, nf, COL_GRAY);
        }

        // Position indicator: "MASTER X OF Y"
        char posBuf[32];
        char curBuf[8], totBuf[8];
        intToStr(sCursor + 1, curBuf);
        intToStr(sSortedCount, totBuf);
        snprintf(posBuf, sizeof(posBuf), "%s / %s", curBuf, totBuf);
        int pw = renderTextWidth(posBuf);
        renderText(128 - pw / 2, 184, posBuf, COL_HINT);
    }
}

static void renderLevel0Bottom() {
    renderClearSub();

    // Header bar: "SAYINGS  123/448  [filter]"
    {
        char hdr[48];
        char numBuf[8];
        intToStr(sayingsFoundCount(), numBuf);

        // Filter label
        const char* filterLabel;
        if (sFilter == 0) filterLabel = str(kSayingsUI[SAY_ALL]);
        else if (sFilter == 1) filterLabel = str(kSayingsUI[SAY_FOUND]);
        else filterLabel = str(kSayingsUI[SAY_NOT_FOUND]);

        snprintf(hdr, sizeof(hdr), "%s %s/%d  <%s>", str(kSayingsUI[SAY_TITLE]), numBuf, MASTER_COUNT, filterLabel);
        renderTextSub(4, 2, hdr, COL_GOLD);
    }

    // Separator line
    renderFilledRectSub(0, HEADER_H, 256, 1, COL_GOLD_DIM);

    // Master cards
    for (int i = 0; i < VISIBLE_CARDS; i++) {
        int idx = sListScroll + i;
        if (idx >= sSortedCount) break;

        int id = sSortedIds[idx];
        int cy = LIST_Y + i * CARD_HEIGHT;
        bool selected = (idx == sCursor);
        u8 bits = sayingsGetBits(id);
        bool found = (bits > 0);

        // Background
        if (selected) {
            renderFilledRectSub(0, cy, 256, CARD_HEIGHT, COL_BG_SEL);
            // Left edge highlight bar
            renderFilledRectSub(0, cy, 2, CARD_HEIGHT, COL_GOLD);
        }

        // Master name or "???"
        if (found) {
            // Small diamond indicator
            renderFilledRectSub(4, cy + 7, 4, 4, COL_GOLD);
            renderMasterName(12, cy + 4, id, COL_GOLD, true);
        } else {
            renderTextSub(12, cy + 4, str(kSayingsUI[SAY_UNKNOWN]), COL_GRAY);
        }

        // E1/E2 status badges on the right
        if (found) {
            u16 e1Color = COL_GREEN;
            renderTextSub(220, cy + 2, "E1", e1Color);

            bool hasE2 = (kMasters[id].e2 != nullptr || kMasters[id].e2_de != nullptr);
            if (hasE2) {
                u16 e2Color = (bits == 3) ? COL_GREEN : COL_GRAY;
                renderTextSub(238, cy + 2, "E2", e2Color);
            }
        }

        // Separator between cards (thin line)
        renderFilledRectSub(4, cy + CARD_HEIGHT - 1, 248, 1, RGB15(3, 3, 3));
    }

    // Scroll indicators in list area
    if (sListScroll > 0) {
        renderTextSub(124, LIST_Y - 2, "^", COL_HINT);
    }
    if (sListScroll + VISIBLE_CARDS < sSortedCount) {
        renderTextSub(124, LIST_Y + VISIBLE_CARDS * CARD_HEIGHT, "V", COL_HINT);
    }

    // Footer
    {
        const char* hint1 = str(kSayingsUI[SAY_A_READ]);
        const char* hint2 = str(kSayingsUI[SAY_B_BACK]);
        char footer[64];
        snprintf(footer, sizeof(footer), "%s  %s  %s", hint1, hint2, str(kSayingsUI[SAY_LR_FILTER]));
        renderTextSub(4, FOOTER_Y + 4, footer, COL_HINT);
    }
}

// ---------------------------------------------------------------------------
// Render: Level 1 (Encounter Select)
// ---------------------------------------------------------------------------

static void renderLevel1Top() {
    renderClearParticleLayer();
    hideOamSprites();

    if (sSortedCount == 0 || sCursor >= sSortedCount) return;
    int id = sSortedIds[sCursor];

    // Master name centered
    int nw = masterNameWidth(id);
    renderMasterName(128 - nw / 2, 10, id, COL_GOLD, false);
    renderFilledRect(16, 28, 224, 1, COL_GOLD_DIM);

    // Preview of selected encounter
    const char* text = (sEncSelect == 0) ? sayingsGetE1(id) : sayingsGetE2(id);
    if (text) {
        int tLen = sayingsEncounterLen(text);
        textRenderWrapped(READ_MARGIN_X, 36, text, tLen, READ_WIDTH, COL_WHITE_DIM, 0, 14);
    }
}

static void renderLevel1Bottom() {
    renderClearSub();

    if (sSortedCount == 0 || sCursor >= sSortedCount) return;
    int id = sSortedIds[sCursor];

    // Master name centered at top
    int nw = masterNameWidth(id);
    renderMasterName(128 - nw / 2, 10, id, COL_GOLD, true);

    // Encounter buttons
    for (int e = 0; e < 2; e++) {
        int by = 40 + e * 40;
        bool sel = (sEncSelect == e);
        u16 bg = sel ? COL_BG_SEL : COL_BG_DARK;
        renderFilledRectSub(16, by, 224, 30, bg);

        // Selection indicator and border
        if (sel) {
            renderFilledRectSub(16, by, 2, 30, COL_GOLD);
            renderTextSub(22, by + 10, ">", COL_GOLD);
        }

        // Label: "ENCOUNTER 1" or "ENCOUNTER 2"
        const char* label = (e == 0) ? str(kSayingsUI[SAY_ENCOUNTER1]) : str(kSayingsUI[SAY_ENCOUNTER2]);
        u16 textCol = sel ? COL_GOLD : COL_GRAY;
        int lw = renderTextWidth(label);
        renderTextSub(128 - lw / 2, by + 10, label, textCol);
    }

    // Footer hints
    renderTextSub(4, 170, str(kSayingsUI[SAY_A_SELECT]), COL_HINT);
    renderTextSub(4, 180, str(kSayingsUI[SAY_B_BACK]), COL_HINT);
}

// ---------------------------------------------------------------------------
// Render: Level 2 (Text Reader)
// ---------------------------------------------------------------------------

static void renderLevel2Top() {
    renderClearParticleLayer();
    hideOamSprites();

    int id = sReadMasterId;

    // Master name at top left
    renderMasterName(READ_MARGIN_X, 4, id, COL_GOLD, false);

    // E1/E2 badge top-right
    const char* badge = (sReadEncounter == 0) ? "E1" : "E2";
    renderText(240, 4, badge, COL_GOLD_DIM);

    // Separator
    renderFilledRect(READ_MARGIN_X, 16, READ_WIDTH, 1, COL_GOLD_DIM);

    // Word-wrapped encounter text with scroll
    const char* text = (sReadEncounter == 0) ? sayingsGetE1(id) : sayingsGetE2(id);
    if (text) {
        int tLen = sayingsEncounterLen(text);
        sReadTotalLines = textRenderWrapped(READ_MARGIN_X, READ_TEXT_Y, text, tLen,
                                             READ_WIDTH, COL_WHITE,
                                             sReadScroll, READ_VISIBLE);
    } else {
        sReadTotalLines = 0;
    }

    // Scroll indicators
    if (sReadScroll > 0) {
        renderText(124, 10, "^", COL_HINT);
    }
    if (sReadTotalLines > READ_VISIBLE && sReadScroll < sReadTotalLines - READ_VISIBLE) {
        renderText(124, 184, "V", COL_HINT);
    }
}

static void renderLevel2Bottom() {
    renderClearSub();

    int id = sReadMasterId;

    // Master name
    renderMasterName(4, 8, id, COL_GOLD, true);

    // Encounter label
    const char* encLabel = (sReadEncounter == 0) ? str(kSayingsUI[SAY_ENCOUNTER1])
                                                  : str(kSayingsUI[SAY_ENCOUNTER2]);
    renderTextSub(4, 24, encLabel, COL_WHITE_DIM);

    // Scroll progress
    if (sReadTotalLines > 0) {
        char progBuf[32];
        char fromBuf[8], toBuf[8], totBuf[8];
        int firstLine = sReadScroll + 1;
        int lastLine = sReadScroll + READ_VISIBLE;
        if (lastLine > sReadTotalLines) lastLine = sReadTotalLines;
        intToStr(firstLine, fromBuf);
        intToStr(lastLine, toBuf);
        intToStr(sReadTotalLines, totBuf);
        snprintf(progBuf, sizeof(progBuf), "%s %s-%s / %s", str(kSayingsUI[SAY_LINE]), fromBuf, toBuf, totBuf);
        renderTextSub(4, 40, progBuf, COL_HINT);
    }

    // Navigation hints
    renderTextSub(4, 140, str(kSayingsUI[SAY_SCROLL]), COL_HINT);
    renderTextSub(4, 152, str(kSayingsUI[SAY_B_BACK]), COL_HINT);

    // L/R hint if both encounters found
    u8 bits = sayingsGetBits(id);
    if (bits == 3) {
        renderTextSub(4, 170, str(kSayingsUI[SAY_LR_SWITCH]), COL_HINT);
    }
}

// ---------------------------------------------------------------------------
// Render dispatch
// ---------------------------------------------------------------------------

void sayingsViewerRender() {
    // Top screen: redraw every frame (double-buffered, no tearing risk)
    switch (sLevel) {
        case 0: renderLevel0Top(); break;
        case 1: renderLevel1Top(); break;
        case 2: renderLevel2Top(); break;
    }

    // Bottom screen: dirty-flag pattern (single-buffered, tearing risk)
    if (!sDirty) return;
    sDirty = false;

    switch (sLevel) {
        case 0: renderLevel0Bottom(); break;
        case 1: renderLevel1Bottom(); break;
        case 2: renderLevel2Bottom(); break;
    }
}
