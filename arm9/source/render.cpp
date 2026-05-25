#include "render.h"
#include "camera.h"
#include "player.h"
#include "entities.h"
#include "game.h"
#include "companion.h"
#include "sprite_data.h"
#include "enemy_sprites.h"
#include "shop.h"
#include "strings.h"
#include "synergy.h"
#include "rng.h"
#include <cstring>
#include <cstdio>

// Forward declarations for game queries used by visual overlays.
// These must be implemented in game.cpp.
extern u8  gameGetWaveTimerPct();
extern u8  gameGetAnnounceTimer();

// Merge flash overlay — timer counts down each frame, visual overlay when > 0
static int gMergeFlashTimer = 0;
static constexpr int MERGE_FLASH_FRAMES = 30;

static u16* particleFB = nullptr;
static u16* framebuffers[2];
static int drawBuffer = 0;
int renderFrame = 0;
static int bgParticleId = -1;

// Sub-screen (bottom) bitmap framebuffer — set up by renderInitSub(), called from uiInit()
static u16* subFB = nullptr;

// ---------------------------------------------------------------------------
// 5x7 pixel font (row-based encoding)
// Each glyph is 7 bytes; each byte represents one row (5 bits used, MSB-first).
// Index mapping: space=0, '0'-'9'=1-10, 'A'-'Z'=11-36, '!'=37, ':'=38
// ---------------------------------------------------------------------------
static const u8 FONT_5X7[][7] = {
    // Space (index 0)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    // 0-9 (indices 1-10)
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
    {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
    // A-Z (indices 11-36)
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}, // G
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // J
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
    {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E}, // S
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
    {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04}, // V
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, // W
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
    // ! (index 37)
    {0x04,0x04,0x04,0x04,0x04,0x00,0x04},
    // : (index 38)
    {0x00,0x04,0x00,0x00,0x00,0x04,0x00},
    // ( (index 39)
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02},
    // ) (index 40)
    {0x08,0x04,0x02,0x02,0x02,0x04,0x08},
    // / (index 41)
    {0x01,0x02,0x02,0x04,0x08,0x08,0x10},
    // + (index 42)
    {0x00,0x04,0x04,0x1F,0x04,0x04,0x00},
    // - (index 43)
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
    // . (index 44)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x04},
    // % (index 45)
    {0x19,0x1A,0x02,0x04,0x0B,0x13,0x13},
    // ' (index 46) — apostrophe
    {0x04,0x04,0x08,0x00,0x00,0x00,0x00},
};

static const int CHAR_W = 5;   // glyph pixel width
static const int CHAR_H = 7;   // glyph pixel height
static const int CHAR_GAP = 1; // pixels between glyphs

static int charToFontIdx(char c) {
    if (c == ' ') return 0;
    if (c >= '0' && c <= '9') return 1 + (c - '0');
    if (c >= 'A' && c <= 'Z') return 11 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 11 + (c - 'a'); // treat lowercase as upper
    if (c == '!') return 37;
    if (c == ':') return 38;
    if (c == '(') return 39;
    if (c == ')') return 40;
    if (c == '/') return 41;
    if (c == '+') return 42;
    if (c == '-') return 43;
    if (c == '.') return 44;
    if (c == '%') return 45;
    if (c == '\'') return 46;
    return 0; // space for unknown
}

// ---------------------------------------------------------------------------
// Wave announcement state
// ---------------------------------------------------------------------------
static char  annText[16]  = "";
static int   annTimer     = 0;   // frames remaining
static int   annMaxTimer  = 0;   // total duration (for fade calc)

// ---------------------------------------------------------------------------

// Palette of pill colors in RGB15
static const u16 PILL_COLORS[] = {
    RGB15(31, 0, 0),   // Red
    RGB15(0, 10, 31),  // Blue
    RGB15(0, 28, 0),   // Green
    RGB15(31, 28, 0),  // Yellow
    RGB15(20, 0, 31),  // Purple
    RGB15(0, 28, 28),  // Cyan
};

void renderInit() {
    // Top screen — Main engine
    videoSetMode(MODE_5_2D);

    // Double buffering: VRAM_A = buffer 0, VRAM_B = buffer 1
    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);  // Buffer 0 at offset 0
    vramSetBankB(VRAM_B_MAIN_BG_0x06020000);  // Buffer 1 at offset 8

    // Use VRAM_E for sprites (64KB, plenty for current usage)
    vramSetBankE(VRAM_E_MAIN_SPRITE);

    // BG3: 16-bit bitmap for particles — initially displaying buffer 0
    bgParticleId = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

    // Set up framebuffer pointers
    framebuffers[0] = (u16*)BG_BMP_RAM(0);   // VRAM_A: 0x06000000
    framebuffers[1] = (u16*)BG_BMP_RAM(8);   // VRAM_B: 0x06020000

    // Start drawing to buffer 1, displaying buffer 0
    drawBuffer = 1;
    particleFB = framebuffers[drawBuffer];

    // Set up OAM for sprites
    oamInit(&oamMain, SpriteMapping_1D_32, false);

    // Load sprite tile data and palettes into VRAM
    spriteLoadAll();

    // Initialize enemy sprite lookup tables (ROM-based, no VRAM needed)
    enemySpriteInit();

    // Bottom screen is set up by uiInit() via consoleDemoInit()
    // Don't configure sub engine here to avoid VRAM conflicts

    // Clear both buffers
    memset(framebuffers[0], 0, 256 * 192 * 2);
    memset(framebuffers[1], 0, 256 * 192 * 2);
}

void renderClearParticleLayer() {
    if (!particleFB) return;
    // Use memset instead of dmaFillHalfWords to avoid hardware DMA
    // corrupting sub-screen VRAM (caused a black bar on the bottom screen
    // that moved with the camera).
    memset(particleFB, 0, 256 * 192 * 2);
}

void renderFlip() {
    // Flip which buffer the PPU displays by writing BG3CNT map base directly.
    // Do NOT use bgUpdate() — it flushes ALL BG state including sub-engine,
    // which corrupts the console scroll registers and causes the black bar.
    int mapBase = (drawBuffer == 0) ? 0 : 8;
    REG_BG3CNT = (REG_BG3CNT & ~(0x1F << 8)) | (mapBase << 8);
    // Swap draw target
    drawBuffer ^= 1;
    particleFB = framebuffers[drawBuffer];
}

u16* renderGetActiveBuffer() { return particleFB; }

void renderPixel(int x, int y, u16 color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    particleFB[y * 256 + x] = color | BIT(15);
}

void renderFilledRect(int x, int y, int w, int h, u16 color) {
    u16 c = color | BIT(15);
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > SCREEN_W) ? SCREEN_W : x + w;
    int y1 = (y + h > SCREEN_H) ? SCREEN_H : y + h;
    int rowW = x1 - x0;
    if (rowW <= 0) return;

    for (int py = y0; py < y1; ++py) {
        u16* row = particleFB + py * 256 + x0;
        for (int i = 0; i < rowW; ++i) row[i] = c;
    }
}

void renderFilledCircle(int cx, int cy, int r, u16 color) {
    u16 c = color | BIT(15);
    int r2 = r * r;

    int y0 = (cy - r < 0) ? 0 : cy - r;
    int y1 = (cy + r >= SCREEN_H) ? SCREEN_H - 1 : cy + r;

    for (int py = y0; py <= y1; ++py) {
        int dy = py - cy;
        int dx2 = r2 - dy * dy;
        if (dx2 < 0) continue;

        // Fast integer sqrt via Newton's method (2 iterations)
        int dx = r;  // initial guess
        if (dx > 0) {
            dx = (dx + dx2 / dx) >> 1;
            dx = (dx + dx2 / dx) >> 1;
        }

        int x0 = (cx - dx < 0) ? 0 : cx - dx;
        int x1 = (cx + dx >= SCREEN_W) ? SCREEN_W - 1 : cx + dx;

        // Fill row with DMA-style u16 writes
        u16* row = particleFB + py * 256 + x0;
        int count = x1 - x0 + 1;
        while (count-- > 0) *row++ = c;
    }
}

u16 pillColorToRGB(PillColor c) {
    return PILL_COLORS[static_cast<int>(c)];
}

// ---------------------------------------------------------------------------
// Sub-screen (bottom, 256x192) bitmap drawing
// Call renderInitSub() once from uiInit() after VRAM_C is mapped to sub BG.
// ---------------------------------------------------------------------------

void renderInitSub() {
    // Sub engine: MODE_5_2D, VRAM_C as sub BG (same bank as console used, now bitmap)
    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);

    // BG3 on sub screen: 16-bit bitmap, 256x256, map/tile base 0
    bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

    // Sub BG VRAM starts at 0x06200000
    subFB = (u16*)BG_BMP_RAM_SUB(0);

    // Clear to black
    memset(subFB, 0, 256 * 192 * 2);
}

void renderClearSub() {
    if (!subFB) return;
    memset(subFB, 0, 256 * 192 * 2);
}

void renderFilledRectSub(int x, int y, int w, int h, u16 color) {
    if (!subFB) return;
    u16 c = color | BIT(15);
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > SCREEN_W) ? SCREEN_W : x + w;
    int y1 = (y + h > SCREEN_H) ? SCREEN_H : y + h;
    for (int py = y0; py < y1; ++py)
        for (int px = x0; px < x1; ++px)
            subFB[py * 256 + px] = c;
}

void renderTextSub(int x, int y, const char* text, u16 color) {
    if (!subFB) return;
    u16 c = color | BIT(15);
    for (int i = 0; text[i]; ++i) {
        int idx = charToFontIdx(text[i]);
        for (int row = 0; row < 7; ++row) {
            u8 bits = FONT_5X7[idx][row];
            for (int col = 0; col < 5; ++col) {
                if (bits & (0x10 >> col)) {
                    int px = x + i * 6 + col;
                    int py = y + row;
                    if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H)
                        subFB[py * 256 + px] = c;
                }
            }
        }
    }
}


void renderArenaFloor() {
    int cx = camX();
    int cy = camY();
    u16 lineColor = RGB15(4, 4, 6) | BIT(15);

    // Vertical grid lines every 32px
    // Find the first grid line visible on screen
    int firstVert = (cx >= 0) ? (cx / 32) * 32 : ((cx - 31) / 32) * 32;
    for (int wx = firstVert; wx <= cx + SCREEN_W; wx += 32) {
        int sx = wx - cx;
        if (sx < 0 || sx >= SCREEN_W) continue;
        if (wx < 0 || wx > ARENA_W) continue;
        for (int sy = 0; sy < SCREEN_H; ++sy) {
            particleFB[sy * 256 + sx] = lineColor;
        }
    }

    // Horizontal grid lines every 32px
    int firstHoriz = (cy >= 0) ? (cy / 32) * 32 : ((cy - 31) / 32) * 32;
    for (int wy = firstHoriz; wy <= cy + SCREEN_H; wy += 32) {
        int sy = wy - cy;
        if (sy < 0 || sy >= SCREEN_H) continue;
        if (wy < 0 || wy > ARENA_H) continue;
        for (int sx = 0; sx < SCREEN_W; ++sx) {
            particleFB[sy * 256 + sx] = lineColor;
        }
    }
}

void renderArenaWalls() {
    int cx = camX();
    int cy = camY();
    u16 wallColor = RGB15(8, 4, 2);

    // Top wall: world y [0, WALL_THICK)
    {
        int wy0 = 0;
        int wy1 = WALL_THICK;
        int sy0 = wy0 - cy;
        int sy1 = wy1 - cy;
        if (sy0 < SCREEN_H && sy1 > 0) {
            renderFilledRect(0 - cx, sy0, ARENA_W, sy1 - sy0, wallColor);
        }
    }

    // Bottom wall: world y [ARENA_H - WALL_THICK, ARENA_H)
    {
        int wy0 = ARENA_H - WALL_THICK;
        int wy1 = ARENA_H;
        int sy0 = wy0 - cy;
        int sy1 = wy1 - cy;
        if (sy0 < SCREEN_H && sy1 > 0) {
            renderFilledRect(0 - cx, sy0, ARENA_W, sy1 - sy0, wallColor);
        }
    }

    // Left wall: world x [0, WALL_THICK)
    {
        int wx0 = 0;
        int wx1 = WALL_THICK;
        int sx0 = wx0 - cx;
        int sx1 = wx1 - cx;
        if (sx0 < SCREEN_W && sx1 > 0) {
            renderFilledRect(sx0, 0 - cy, sx1 - sx0, ARENA_H, wallColor);
        }
    }

    // Right wall: world x [ARENA_W - WALL_THICK, ARENA_W)
    {
        int wx0 = ARENA_W - WALL_THICK;
        int wx1 = ARENA_W;
        int sx0 = wx0 - cx;
        int sx1 = wx1 - cx;
        if (sx0 < SCREEN_W && sx1 > 0) {
            renderFilledRect(sx0, 0 - cy, sx1 - sx0, ARENA_H, wallColor);
        }
    }
}

void renderGameplay() {
    renderFrame++;

    renderClearParticleLayer();

    int cx = camX();
    int cy = camY();

    // Draw arena background and walls
    renderArenaFloor();
    renderArenaWalls();

    // Dash afterimage (draw before the player so it appears behind)
    if (gPlayer.isDashing) {
        int apx = gPlayer.prevPos.pixelX() - cx;
        int apy = gPlayer.prevPos.pixelY() - cy;
        renderFilledRect(apx - 4, apy - 6, 8, 12, RGB15(12, 12, 20));
        renderFilledCircle(apx, apy - 6, 4, RGB15(12, 12, 20));
        renderFilledCircle(apx, apy + 5, 4, RGB15(12, 12, 20));
    }

    // Speed lines behind player when speed bonus active
    if (gPassive.speedBonus > 0) {
        int spx = gPlayer.pos.pixelX() - cx;
        int spy = gPlayer.pos.pixelY() - cy;
        u16 lineCol = RGB15(20, 20, 28);
        for (int ln = 0; ln < 3; ln++) {
            int lx = spx - 5 + (renderFrame * 3 + ln * 7) % 10;
            int ly = spy + 4 + ln * 3;
            renderFilledRect(lx, ly, 4, 1, lineCol);
        }
    }

    // Player sprite via OAM
    int px = gPlayer.pos.pixelX() - cx;
    int py = gPlayer.pos.pixelY() - cy;
    oamSet(&oamMain, 0, px - 8, py - 8, 0,
           spritePaletteSlot(6), // player palette
           SpriteSize_16x16, SpriteColorFormat_16Color,
           spriteFrameGfx(0, (renderFrame >> 4) & 1), // player classId=0, alternate frames
           -1, false, false, false, false, false);

    // Passive damage boost: red power bar under player
    if (gPassive.damageBoostPct > 0) {
        int w = 3 + gPassive.damageBoostPct / 20;
        if (w > 12) w = 12;
        renderFilledRect(px - w / 2, py + 8, w, 2, RGB15(31, 4, 4));
    }

    // ── Red T3 synergy: fire aura ring around player (8px, pulsing) ─────
    if (gSynergy.tier[static_cast<int>(PillColor::Red)] >= 3) {
        u16 auraCol = (renderFrame & 4) ? RGB15(31, 12, 0) : RGB15(24, 6, 0);
        // Draw 8 points on a ring at radius 8-9 (octagonal approximation)
        static const s8 kAX[8] = { 9, 6,  0, -6, -9, -6,  0,  6 };
        static const s8 kAY[8] = { 0, -6, -9, -6,  0,  6,  9,  6 };
        for (int k = 0; k < 8; ++k) {
            renderPixel(px + kAX[k], py + kAY[k], auraCol);
            renderPixel(px + kAX[k] + 1, py + kAY[k], auraCol);
        }
    }

    // ── Green T0 synergy: player heal flash (8-frame green pulse) ────────
    if (gSynergy.healFlashTimer > 0) {
        // Two rising green pixels — one each side of the player, drifting up
        int riseOffset = 8 - static_cast<int>(gSynergy.healFlashTimer);
        renderPixel(px - 3, py - riseOffset,     RGB15(0, 31, 8));
        renderPixel(px + 3, py - riseOffset + 2, RGB15(0, 28, 4));
    }

    // ── Green T2 synergy: shield ring around player ───────────────────────
    if (gSynergy.tier[static_cast<int>(PillColor::Green)] >= 2) {
        // Faint blue circle (radius 14) — bright when ready, dim/strobing when recharging
        u16 shieldColor;
        bool drawShield = true;
        if (gSynergy.greenShieldReady) {
            shieldColor = RGB15(4, 16, 28);  // solid faint blue
        } else {
            // Recharging: strobe every 8 frames, brighter as recharge nears completion
            int t = static_cast<int>(gSynergy.greenShieldTimer);
            drawShield = ((renderFrame & 8) != 0) || t < 120;
            shieldColor = (t < 120) ? RGB15(2, 8, 18) : RGB15(1, 4, 10);
        }
        if (drawShield) {
            static const s8 kSX[8] = { 14, 10,  0,-10,-14,-10,  0, 10 };
            static const s8 kSY[8] = {  0,-10,-14,-10,  0, 10, 14, 10 };
            for (int k = 0; k < 8; ++k)
                renderPixel(px + kSX[k], py + kSY[k], shieldColor);
        }
    }

    // ── Cyan T2 synergy: EMP charge ring (grows toward next pulse) ───────
    if (gSynergy.tier[static_cast<int>(PillColor::Cyan)] >= 2) {
        int empR = static_cast<int>(gSynergy.cyanEmpTimer) * 16 / 60 + 4;
        if (empR > 20) empR = 20;
        u8 empBright = static_cast<u8>(gSynergy.cyanEmpTimer * 24 / 60);
        if (empBright < 4) empBright = 4;
        if (empBright > 24) empBright = 24;
        // Octagon ring approximation (8 points scaled to empR)
        // Only draw every other 4-frame block when faint; always when near fire
        if ((renderFrame & 4) || gSynergy.cyanEmpTimer > 48) {
            static const s8 kEOX[8] = { 1, 1, 0,-1,-1,-1, 0, 1 };
            static const s8 kEOY[8] = { 0,-1,-1,-1, 0, 1, 1, 1 };
            for (int k = 0; k < 8; ++k)
                renderPixel(px + kEOX[k] * empR, py + kEOY[k] * empR,
                            RGB15(0, empBright, empBright));
        }
    }

    // Companion sprites via OAM
    for (int i = 0; i < MAX_COMPANIONS; ++i) {
        const Companion& c = gCompanions[i];
        if (!c.active) {
            oamSet(&oamMain, 1 + i, 0, 0, 0, 0, SpriteSize_16x16,
                   SpriteColorFormat_16Color, spriteFrameGfx(0, 0),
                   -1, false, true, false, false, false); // hidden
            continue;
        }
        int sx = c.pos.pixelX() - cx - 8;
        int sy = c.pos.pixelY() - cy - 8;
        int fullClassId = companionFullClassId(c);
        // classId for spriteFrameGfx: 1 + fullClassId (0=player, 1-36=companions)
        int sprClassId = 1 + fullClassId;
        // Frame: tier*2 + animation toggle
        int frame = c.tier * 2 + ((renderFrame >> 4) & 1);
        int palSlot = spritePaletteSlot(static_cast<int>(c.color));

        // Iframe flicker: hide sprite on odd 4-frame cycles when invincible
        bool hidden = (c.iframes > 0) && ((renderFrame >> 2) & 1);

        oamSet(&oamMain, 1 + i, sx, sy, 0,
               palSlot, SpriteSize_16x16, SpriteColorFormat_16Color,
               spriteFrameGfx(sprClassId, frame),
               -1, false, hidden, false, false, false);
    }

    // --- Companion HP bars and hit-flash overlays ---
    for (int i = 0; i < MAX_COMPANIONS; ++i) {
        const Companion& c = gCompanions[i];
        if (!c.active) continue;
        int sx = c.pos.pixelX() - cx;
        int sy = c.pos.pixelY() - cy;

        // Hit flash: 16x16 white rect drawn into framebuffer for 3 frames
        if (c.hitFlashTimer > 0) {
            renderFilledRect(sx - 8, sy - 8, 16, 16, RGB15(31, 31, 31));
        }

        // HP bar: 1px tall, 12px max width, 2px below sprite feet
        if (c.maxHp > 0) {
            int barW = (static_cast<int>(c.hp) * 12) / c.maxHp;
            if (barW < 0) barW = 0;
            int hpPct = (static_cast<int>(c.hp) * 100) / c.maxHp;
            u16 barColor;
            if (c.tier == 0) {
                // T1: standard health-based colour (green → yellow → red)
                barColor = (hpPct > 50) ? RGB15(0, 25, 0)
                         : (hpPct > 25) ? RGB15(31, 28, 0)
                                        : RGB15(31, 4, 4);
            } else if (c.tier == 1) {
                // T2: gold bar when healthy, falls back to red when critical
                barColor = (hpPct > 25) ? RGB15(31, 24, 0) : RGB15(31, 4, 4);
            } else {
                // T3: prismatic cycling — six-step hue rotation (period 60f)
                static const u16 kPrism[6] = {
                    RGB15(31,  0,  0), RGB15(31, 20,  0),
                    RGB15(12, 31,  0), RGB15(0,  28, 20),
                    RGB15(0,  10, 31), RGB15(20,  0, 31)
                };
                barColor = (hpPct > 25) ? kPrism[(renderFrame / 10) % 6] : RGB15(31, 4, 4);
            }
            // Background trough (dark, full 12px)
            renderFilledRect(sx - 6, sy + 9, 12, 1, RGB15(6, 6, 6));
            // Filled portion
            if (barW > 0)
                renderFilledRect(sx - 6, sy + 9, barW, 1, barColor);
        }

        // Heal particle: 2-3 green pixels rising from companion for healFlashTimer frames
        if (c.healFlashTimer > 0) {
            // Stagger 2 rising pixels offset by frame parity
            int riseY = sy - 8 - (12 - c.healFlashTimer);
            renderPixel(sx - 2, riseY,     RGB15(0, 31, 8));
            renderPixel(sx + 2, riseY + 2, RGB15(0, 28, 4));
            renderPixel(sx,     riseY - 2, RGB15(0, 24, 4));
        }
    }

    // Tick companion visual timers
    for (int i = 0; i < MAX_COMPANIONS; ++i) {
        Companion& c = gCompanions[i];
        if (!c.active) continue;
        if (c.hitFlashTimer > 0) --c.hitFlashTimer;
        if (c.iframes > 0)       --c.iframes;
        if (c.healFlashTimer > 0) --c.healFlashTimer;
    }

    // --- Companion aura visual effects ---
    // Helper: brighten a 15-bit colour by companion tier using shifts (no division)
    // T1=1.0x, T2=+25%, T3=+50%
    static auto scaleBrightness = [](u16 color, u8 tier) -> u16 {
        if (tier == 0) return color;
        int r = (color >> 0) & 0x1F;
        int g = (color >> 5) & 0x1F;
        int b = (color >> 10) & 0x1F;
        if (tier >= 2) { r += r >> 1; g += g >> 1; b += b >> 1; }
        else           { r += r >> 2; g += g >> 2; b += b >> 2; }
        if (r > 31) r = 31; if (g > 31) g = 31; if (b > 31) b = 31;
        return static_cast<u16>(r | (g << 5) | (b << 10));
    };
    static const s8 kOrbitX[8] = { 12, 8, 0, -8, -12, -8, 0, 8 };
    static const s8 kOrbitY[8] = { 0, -8, -12, -8, 0, 8, 12, 8 };

    for (int i = 0; i < MAX_COMPANIONS; ++i) {
        const Companion& c = gCompanions[i];
        if (!c.active) continue;
        int sx = c.pos.pixelX() - cx;
        int sy = c.pos.pixelY() - cy;
        int fci = companionFullClassId(c);

        // Tier-scaled aura pulse period: T1=40f, T2=24f, T3=14f (faster = more energetic)
        int auraPeriod = (c.tier == 0) ? 40 : (c.tier == 1) ? 24 : 14;

        // Damage boost aura: red companions pulse faintly (except Pyromaniac who has fire aura)
        if (fci <= 5 && fci != 1) { // Red class, non-Pyromaniac
            if ((renderFrame + i * 7) % auraPeriod < 3) {
                renderPixel(sx - 1, sy - 9, scaleBrightness(RGB15(31, 8, 0), c.tier));
                renderPixel(sx + 1, sy - 9, scaleBrightness(RGB15(31, 8, 0), c.tier));
                // T3: extra ring pixel for a larger pulse crown
                if (c.tier >= 2) renderPixel(sx, sy - 11, scaleBrightness(RGB15(31, 4, 0), c.tier));
            }
        }

        // Pyromaniac (fci=1) burn aura: orange ring around companion
        if (fci == 1) {
            u32 phase = (renderFrame + i * 5) % 24;
            if (phase < 6) {
                u16 fireCol = scaleBrightness(RGB15(31, 12 + phase * 2, 0), c.tier);
                renderPixel(sx - 6, sy, fireCol);
                renderPixel(sx + 6, sy, fireCol);
                renderPixel(sx, sy - 6, fireCol);
                renderPixel(sx, sy + 6, fireCol);
                renderPixel(sx - 4, sy - 4, fireCol);
                renderPixel(sx + 4, sy - 4, fireCol);
                renderPixel(sx - 4, sy + 4, fireCol);
                renderPixel(sx + 4, sy + 4, fireCol);
            }
        }

        // Slow aura: Warden (fci=6), blue ring pulse
        if (fci == 6) {
            u32 phase = renderFrame % 30;
            if (phase < 5) {
                u16 col = scaleBrightness(RGB15(4 + phase * 3, 8 + phase * 2, 28), c.tier);
                renderPixel(sx, sy - 32, col);
                renderPixel(sx, sy + 32, col);
                renderPixel(sx - 32, sy, col);
                renderPixel(sx + 32, sy, col);
            }
        }

        // Cooldown buff: Channeler (fci=7), orbiting dots
        if (fci == 7) {
            int slot0 = (renderFrame >> 2) & 7;
            int slot1 = (slot0 + 4) & 7;
            renderPixel(sx + kOrbitX[slot0], sy + kOrbitY[slot0], RGB15(20, 20, 31));
            renderPixel(sx + kOrbitX[slot1], sy + kOrbitY[slot1], RGB15(20, 20, 31));
        }

        // Regen: Green companions, green sparkle synced with heal tick
        if (fci >= 12 && fci <= 17) {
            // Sync sparkle with regen passive intervals (300f for G1, 300f for G3)
            u16 interval = 300;  // default regen interval
            if (renderFrame % interval < 4) {
                renderPixel(sx - 1, sy - 10, scaleBrightness(RGB15(0, 31, 0), c.tier));
                renderPixel(sx, sy - 10, scaleBrightness(RGB15(0, 31, 0), c.tier));
                renderPixel(sx + 1, sy - 10, scaleBrightness(RGB15(0, 28, 0), c.tier));
                renderPixel(sx, sy - 11, scaleBrightness(RGB15(0, 24, 0), c.tier));
            }
        }

        // Gold bonus: Yellow companions, gold shimmer (rate scales with tier)
        if (fci >= 18 && fci <= 23) {
            if ((renderFrame + i * 13) % auraPeriod < 2) {
                renderPixel(sx, sy - 10, scaleBrightness(RGB15(31, 28, 0), c.tier));
                if (c.tier >= 1) renderPixel(sx - 1, sy - 10, scaleBrightness(RGB15(31, 24, 0), c.tier));
                if (c.tier >= 2) renderPixel(sx + 1, sy - 10, scaleBrightness(RGB15(31, 20, 0), c.tier));
            }
        }

        // Purple aura: debuff extend, purple flicker (rate scales with tier)
        if (fci >= 24 && fci <= 29) {
            if ((renderFrame + i * 9) % auraPeriod < 2) {
                renderPixel(sx, sy - 10, scaleBrightness(RGB15(20, 0, 31), c.tier));
                if (c.tier >= 2) renderPixel(sx, sy - 12, scaleBrightness(RGB15(16, 0, 28), c.tier));
            }
        }

        // Cyan aura: tech shimmer (rate scales with tier)
        if (fci >= 30 && fci <= 35) {
            if ((renderFrame + i * 15) % auraPeriod < 2) {
                renderPixel(sx - 1, sy - 10, scaleBrightness(RGB15(0, 28, 28), c.tier));
                renderPixel(sx + 1, sy - 10, scaleBrightness(RGB15(0, 28, 28), c.tier));
            }
        }
    }

    // Draw persistent damage zones (ground-level clouds, rendered under enemies)
    for (const auto& z : gZones) {
        if (!z.active) continue;
        int zx = z.pos.pixelX() - cx;
        int zy = z.pos.pixelY() - cy;

        // Outer ring: dim haze at zone boundary — 8 evenly spaced pixels
        static const s8 kRingCos[8] = { 16, 11,  0,-11,-16,-11,  0, 11 };
        static const s8 kRingSin[8] = {  0,-11,-16,-11,  0, 11, 16, 11 };
        u16 hazeCol = (z.type == ZONE_POISON) ? RGB15(6, 14, 6) : RGB15(14, 6, 0);
        for (int k = 0; k < 8; k++) {
            int rx = zx + (kRingCos[k] * z.radius) / 16;
            int ry = zy + (kRingSin[k] * z.radius) / 16;
            renderPixel(rx, ry, hazeCol);
            renderPixel(rx + 1, ry, hazeCol);
        }

        // Interior scatter: 4 pixels per frame inside the cloud
        u32 zoneIdx = static_cast<u32>(&z - gZones);
        u32 rbase = renderFrame * 7 + zoneIdx * 97u;
        u16 cloudCol = (z.type == ZONE_POISON) ? RGB15(8, 20, 8) : RGB15(20, 10, 0);
        int r2 = z.radius * 2;
        for (int k = 0; k < 4; k++) {
            u32 h = (rbase + k * 13) * 2654435761u;
            // Use AND mask instead of modulo (approximate, but fast)
            int ox = static_cast<int>((h >> 24) & (r2 > 16 ? 31 : 15)) - z.radius;
            int oy = static_cast<int>((h >> 16) & (r2 > 16 ? 31 : 15)) - z.radius;
            renderPixel(zx + ox, zy + oy, cloudCol);
        }

        // Bright centre puff (fades with remaining lifetime)
        u16 puffCol = (z.type == ZONE_POISON) ? RGB15(10, 28, 10) : RGB15(28, 16, 0);
        if (z.lifetime > 30 || (renderFrame & 4))
            renderPixel(zx, zy, puffCol);
    }

    // Draw enemies using sprite data with status-effect overlays
    for (auto& e : gEnemies) {
        if (!e.active) continue;
        int ex = e.pos.pixelX() - cx;
        int ey = e.pos.pixelY() - cy;
        int half = e.spriteSize / 2;

        // Decrement hurt timer each frame
        if (e.hurtTimer > 0) e.hurtTimer--;

        // Determine tint mode: 0=normal, 1=frozen, 2=slowed, 3=white flash
        u8 tintMode = 0;
        if (e.freezeTimer > 0) tintMode = 1;
        else if (e.slowTimer > 0) tintMode = 2;

        // Per-type hurt visuals (override tint when hurtTimer active)
        int drawX = ex - half;
        int drawY = ey - half;
        if (e.hurtTimer > 0) {
            switch (e.type) {
                // Light enemies: white flash for full duration
                case ETYPE_GRUNT:
                case ETYPE_SWARM_DRONE:
                case ETYPE_CHARGER:
                    tintMode = 3;
                    drawY -= 1; // 1px knockback nudge
                    break;

                // Heavy enemies: brief clang (white flash only on first frame)
                case ETYPE_BRUTE:
                case ETYPE_SHIELD:
                case ETYPE_ANCHOR:
                    if (e.hurtTimer >= 3) tintMode = 3; // only first frame of 4
                    break;

                // Organic enemies: green particles + white flash
                case ETYPE_SPITTER:
                case ETYPE_TRAPPER:
                    tintMode = 3;
                    if (e.hurtTimer == 3) { // spawn particles once
                        spawnParticle(e.pos, {0, -64}, 10, static_cast<u8>(PillColor::Green));
                        spawnParticle(e.pos, {0,  64}, 10, static_cast<u8>(PillColor::Green));
                    }
                    break;

                // Dark enemies: white flash (purple tint would fight freeze/slow)
                case ETYPE_NIGHTMARE:
                case ETYPE_HEXER:
                    tintMode = 3;
                    break;

                // Ghost: disappear for 2 frames on hit
                case ETYPE_GHOST:
                    if (e.hurtTimer >= 2) { drawX = -999; } // hide off-screen
                    else tintMode = 3;
                    break;

                // Bomber: red-orange particles + white flash
                case ETYPE_BOMBER:
                    tintMode = 3;
                    if (e.hurtTimer == 3) {
                        spawnParticle(e.pos, {-48, -48}, 10, static_cast<u8>(PillColor::Red));
                        spawnParticle(e.pos, { 48, -48}, 10, static_cast<u8>(PillColor::Red));
                    }
                    break;

                // Stationary heavy: sprite shake
                case ETYPE_ARTILLERY:
                case ETYPE_HIVE:
                    tintMode = 3;
                    drawX += (e.hurtTimer & 1) ? 1 : -1; // alternating 1px shake
                    break;

                // All other enemies: white flash
                default:
                    tintMode = 3;
                    break;
            }

            // Bosses: white flash + camera shake on first hurt frame
            if (e.type >= ETYPE_BOSS_SENTINEL && e.hurtTimer == 3) {
                tintMode = 3;
                cameraShake(1, 3);
            }
        }

        // Animation frame: alternate every 16 frames
        u8 animFrame = (renderFrame >> 4) & 1;

        // Alpha for ghost flicker (visible 3/4 of the time)
        u8 alpha = 4;
        if (e.type == ETYPE_GHOST && tintMode != 1 && e.hurtTimer == 0) {
            bool vis = ((renderFrame >> 2) & 3) != 0;
            alpha = vis ? 3 : 0;
        }

        // Blit the sprite centered on the enemy position
        enemySpriteBlitMain(e.type, e.sizeClass, animFrame,
                            drawX, drawY, tintMode, alpha);

        // Fear indicator: purple flicker above head
        if (e.fearTimer > 0 && (renderFrame & 4)) {
            renderPixel(ex, ey - half - 3, RGB15(20, 0, 31));
            renderPixel(ex, ey - half - 5, RGB15(20, 0, 31));
        }

        // Freeze border: 1px pale-blue rectangle
        if (e.freezeTimer > 0) {
            u16 border = RGB15(25, 28, 31);
            renderFilledRect(ex - half - 1, ey - half - 1, e.spriteSize + 2, 1, border);
            renderFilledRect(ex - half - 1, ey + half,     e.spriteSize + 2, 1, border);
            renderFilledRect(ex - half - 1, ey - half,     1, e.spriteSize,  border);
            renderFilledRect(ex + half,     ey - half,     1, e.spriteSize,  border);
        }

        // HP bar for bosses and brutes
        if (e.type >= ETYPE_BRUTE) {
            int barW = e.spriteSize;
            int barX = ex - half;
            int barY = ey - half - 3;
            int fillW = (e.maxHp > 0) ? (barW * e.hp / e.maxHp) : 0;
            renderFilledRect(barX, barY, barW, 2, RGB15(12, 0, 0));
            if (fillW > 0) renderFilledRect(barX, barY, fillW, 2, RGB15(31, 0, 0));
        }
    }

    // Draw bullets — appearance driven by visualType
    for (const auto& b : gBullets) {
        if (!b.active) continue;
        int bx = b.pos.pixelX() - cx;
        int by = b.pos.pixelY() - cy;
        switch (b.visualType) {
            default:
            case BVIS_NORMAL:    // 4x3 white-yellow — slightly larger to distinguish player bullets
                renderFilledRect(bx - 2, by - 1, 4, 3, RGB15(31, 31, 22));
                break;
            case BVIS_FIRE:      // 3x3 orange-red + 1px trailing ember
                renderFilledRect(bx - 1, by - 1, 3, 3, RGB15(31, 16, 0));
                renderPixel(bx - (b.vel.x > 0 ? 2 : -2), by, RGB15(20, 8, 0));
                break;
            case BVIS_ICE:       // 3x3 pale blue + frost pixel above
                renderFilledRect(bx - 1, by - 1, 3, 3, RGB15(14, 22, 31));
                renderPixel(bx, by - 2, RGB15(22, 28, 31));
                break;
            case BVIS_PIERCE:    // 5x2 elongated bright white
                renderFilledRect(bx - 2, by, 5, 2, RGB15(31, 31, 31));
                break;
            case BVIS_EXPLOSIVE: // 4x4 with dark 2x2 centre
                renderFilledRect(bx - 2, by - 2, 4, 4, RGB15(31, 18, 0));
                renderPixel(bx - 1, by - 1, RGB15(4, 2, 0));
                renderPixel(bx,     by - 1, RGB15(4, 2, 0));
                renderPixel(bx - 1, by,     RGB15(4, 2, 0));
                renderPixel(bx,     by,     RGB15(4, 2, 0));
                break;
            case BVIS_POISON:    // 3x3 purple-green with bright centre pixel
                renderFilledRect(bx - 1, by - 1, 3, 3, RGB15(18, 8, 20));
                renderPixel(bx, by, RGB15(8, 20, 4));
                break;
            case BVIS_ELECTRIC:  // 3x3 cyan, flickers every 4 frames
                renderFilledRect(bx - 1, by - 1, 3, 3,
                    (renderFrame & 4) ? RGB15(0, 28, 31) : RGB15(16, 31, 31));
                break;
            case BVIS_HEAVY:     // 5x5 dark grey-brown
                renderFilledRect(bx - 2, by - 2, 5, 5, RGB15(14, 12, 10));
                break;
            case BVIS_NORMAL_T2: // 4x4 white-yellow, slightly larger
                renderFilledRect(bx - 2, by - 2, 4, 4, RGB15(31, 31, 22));
                break;
            case BVIS_NORMAL_T3: // 5x5 bright + trailing glow
                renderFilledRect(bx - 2, by - 2, 5, 5, RGB15(31, 31, 26));
                renderPixel(bx - (b.vel.x > 0 ? 3 : -3), by, RGB15(31, 28, 8));
                break;
        }
        // ── Red T0 synergy: orange fire trail on every bullet ──────────────
        // Draws a 1px ember dot at the bullet's trailing edge (behind travel dir).
        if (synergyFireTrails()) {
            int trailX = bx - (b.vel.x > 0 ? 3 : (b.vel.x < 0 ? -3 : 0));
            int trailY = by - (b.vel.y > 0 ? 3 : (b.vel.y < 0 ? -3 : 0));
            renderPixel(trailX, trailY, RGB15(20, 8, 0));  // dim orange ember
        }
    }

    // Draw gold drops (pulsing with sparkle, radius scales with gold bonus)
    for (const auto& g : gGold) {
        if (!g.active) continue;
        int gx = g.pos.pixelX() - cx;
        int gy = g.pos.pixelY() - cy;
        int goldRadius = 3 + gPassive.goldBonusPct / 25;
        if (goldRadius > 6) goldRadius = 6;
        u16 goldColor = (renderFrame & 8) ? RGB15(31, 28, 0) : RGB15(22, 18, 0);
        renderFilledCircle(gx, gy, goldRadius, goldColor);
        if (renderFrame & 8) renderPixel(gx - 1, gy - 1, RGB15(31, 31, 31));
    }

    // ── Green T1 synergy: Healing orbs (green pulsing circles) ─────────────
    for (const auto& h : gHealOrbs) {
        if (!h.active) continue;
        int hx = h.pos.pixelX() - cx;
        int hy = h.pos.pixelY() - cy;
        u16 orbColor = (renderFrame & 8) ? RGB15(0, 31, 8) : RGB15(0, 20, 4);
        renderFilledCircle(hx, hy, 4, orbColor);
        if (renderFrame & 8) renderPixel(hx, hy - 1, RGB15(12, 31, 12)); // sparkle
    }

    // Draw zone effects (poison clouds, burn zones) — base circle + particle emission
    for (auto& z : gZones) {
        if (!z.active) continue;

        int zx = z.pos.pixelX() - cx;
        int zy = z.pos.pixelY() - cy;

        // Fade-out: as lifetime decreases below 30 frames use progressively darker color.
        // Three purple steps for poison, three orange steps for burn.
        u16 baseColor;
        if (z.color == static_cast<u8>(PillColor::Purple)) {
            // Poison cloud: dim purple base, darkens in last 30 frames
            baseColor = (z.lifetime > 60) ? RGB15(12, 0, 16)
                      : (z.lifetime > 30) ? RGB15(8,  0, 11)
                                          : RGB15(4,  0, 6);
        } else {
            // Burn zone (Red/Pyromaniac): dim orange base
            baseColor = (z.lifetime > 60) ? RGB15(16, 8, 0)
                      : (z.lifetime > 30) ? RGB15(11, 5, 0)
                                          : RGB15(6,  3, 0);
        }
        renderFilledCircle(zx, zy, z.radius, baseColor);

        // Spawn 3-4 drifting particles every 8 frames
        if ((renderFrame & 7) == 0) {
            int count = 3 + (rngRange(2)); // 3 or 4
            for (int i = 0; i < count; ++i) {
                // Random offset within the zone radius
                int ox = rngRange(z.radius * 2 + 1) - z.radius;
                int oy = rngRange(z.radius * 2 + 1) - z.radius;
                Vec2 ppos = {
                    static_cast<Fixed>(z.pos.x + toFixed(ox)),
                    static_cast<Fixed>(z.pos.y + toFixed(oy))
                };
                // Drift slowly upward; burn rises faster than poison
                s8 riseSpeed = (z.color == static_cast<u8>(PillColor::Purple)) ? -1 : -2;
                Vec2 pvel = { static_cast<Fixed>(rngRange(3) - 1), static_cast<Fixed>(riseSpeed) };
                u8 life = static_cast<u8>(20 + rngRange(11)); // 20-30 frames
                spawnParticle(ppos, pvel, life, z.color);
            }
        }

        // Tick zone lifetime
        if (z.lifetime > 0) --z.lifetime;
        else z.active = false;
    }

    // Draw particles — white for default (color==0), tinted for zone particles
    for (const auto& p : gParticles) {
        if (!p.active) continue;
        // Fade by life ratio
        int b = p.maxLife > 0 ? (static_cast<int>(p.life) * 31) / p.maxLife : 0;
        if (b < 1) b = 1;

        u16 color;
        if (p.color == static_cast<u8>(PillColor::Purple)) {
            // Purple poison particle: tinted purple, fading
            color = RGB15(b / 3, 0, b);
        } else if (p.color == static_cast<u8>(PillColor::Red)) {
            // Orange burn particle: red-orange tint
            color = RGB15(b, b / 2, 0);
        } else {
            // Default: white
            color = RGB15(b, b, b);
        }

        int ppx = p.pos.pixelX() - cx;
        int ppy = p.pos.pixelY() - cy;
        renderPixel(ppx, ppy, color);
        renderPixel(ppx + 1, ppy, color);
        renderPixel(ppx, ppy + 1, color);
    }

    // ── Cyan T1 synergy: chain-lightning arc visuals ─────────────────────
    // Render active chain arcs as a segmented line of cyan pixels from
    // the killed enemy to the chained target. Fades as timer counts down.
    for (int i = 0; i < MAX_CHAIN_ARCS; ++i) {
        const ChainArc& arc = gSynergy.chainArcs[i];
        if (arc.timer == 0) continue;
        // Lerp 8 steps from -> to, draw a 1px dot at each step
        int ax = arc.from.pixelX() - cx;
        int ay = arc.from.pixelY() - cy;
        int bx2 = arc.to.pixelX()   - cx;
        int by2 = arc.to.pixelY()   - cy;
        u8 bright = static_cast<u8>(arc.timer * 28 / 12);
        if (bright > 28) bright = 28;
        u16 arcColor = RGB15(0, bright, bright);
        for (int step = 0; step <= 8; ++step) {
            int lx = ax + (bx2 - ax) * step / 8;
            int ly = ay + (by2 - ay) * step / 8;
            // Add a small jitter for a "lightning bolt" look
            if (step > 0 && step < 8) {
                lx += (step & 1) ? 1 : -1;
            }
            renderPixel(lx, ly, arcColor);
        }
    }

    // ── Blue T1 synergy: freeze flash overlay (white flash on player hit) ──
    if (gSynergy.blueFreezeFlash > 0) {
        u8 flashBright = static_cast<u8>(gSynergy.blueFreezeFlash * 20 / 8);
        u16 flashCol = RGB15(flashBright, flashBright, 31);
        // Two horizontal bars at top and bottom edges
        renderFilledRect(0, 0, SCREEN_W, 2, flashCol);
        renderFilledRect(0, SCREEN_H - 2, SCREEN_W, 2, flashCol);
    }

    // Wave timer bar (3px tall at top of screen)
    u8 pct = gameGetWaveTimerPct();
    int barW = (pct * SCREEN_W) / 255;
    u16 barColor = (pct > 128) ? RGB15(0,28,0) : (pct > 64) ? RGB15(31,28,0) : RGB15(31,5,5);
    renderFilledRect(0, 0, barW, 3, barColor);

    // Wave announcement overlay
    u8 ann = gameGetAnnounceTimer();
    if (ann > 0) {
        renderFilledRect(48, 145, 160, 30, RGB15(2, 2, 8));
        char buf[16];
        snprintf(buf, sizeof(buf), "%s %d", str(kUI[37]), gameGetWave());
        int textW = strlen(buf) * 6;
        renderText(128 - textW/2, 155, buf, RGB15(31, 31, 31));
    }

    // Decrement merge flash timer
    if (gMergeFlashTimer > 0) --gMergeFlashTimer;

    // "MERGE!" flash overlay — shown for MERGE_FLASH_FRAMES after any merge.
    // Fades from gold (first half) to white (second half).
    if (gMergeFlashTimer > 0) {
        bool bright = (gMergeFlashTimer & 4) != 0;  // strobe every 4 frames
        if (bright) {
            const char* mergeStr = "MERGE!";
            int mw = renderTextWidth(mergeStr);
            // Draw drop-shadow for readability
            renderText(128 - mw/2 + 1, 101, mergeStr, RGB15(8, 4, 0));
            // Main text: gold-to-white based on timer
            int r = 31;
            int g = (gMergeFlashTimer > MERGE_FLASH_FRAMES / 2) ? 20 : 31;
            int b = (gMergeFlashTimer > MERGE_FLASH_FRAMES / 2) ? 0  : 20;
            renderText(128 - mw/2, 100, mergeStr, RGB15(r, g, b));
        }
    }

    oamUpdate(&oamMain);
}

void renderMenu() {
    static u32 menuFrame = 0;
    menuFrame++;

    renderClearParticleLayer();

    // Fill the screen with all 36 class sprites in a 6x6 grid
    // Each cell is ~42x32px, sprite centered with slight animated wobble
    constexpr int COLS = 6;
    constexpr int ROWS = 6;
    constexpr int CELL_W = 256 / COLS;  // 42px
    constexpr int CELL_H = 192 / ROWS;  // 32px

    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            int classIdx = row * COLS + col; // 0-35
            int spriteClassId = classIdx + 1; // +1 because 0=player in spriteFrameGfx
            int colorId = classIdx / 6; // 0-5

            // Animate: alternate idle frames
            int frame = ((menuFrame + classIdx * 7) / 20) % 2;

            // Subtle wobble: each sprite moves ±1px, offset by its index
            int wobbleX = ((menuFrame + classIdx * 11) % 60 < 30) ? 0 : 1;
            int wobbleY = ((menuFrame + classIdx * 13) % 50 < 25) ? 0 : 1;

            // Center sprite in cell
            int cx = col * CELL_W + (CELL_W - 16) / 2 + wobbleX;
            int cy = row * CELL_H + (CELL_H - 16) / 2 + wobbleY;

            renderSpriteScaled(cx, cy, spriteClassId, frame, colorId, 1);
        }
    }

    // Dark semi-transparent overlay for title readability
    renderFilledRect(30, 56, 196, 56, RGB15(2, 2, 8));

    // Title text
    renderText(128 - renderTextWidth("TRAIL MIX") / 2, 66, "TRAIL MIX", RGB15(31, 31, 31));
    renderText(128 - renderTextWidth(str(kUI[22])) / 2, 90, str(kUI[22]), RGB15(20, 20, 31));

    // Hide OAM sprites on menu
    for (int i = 0; i < 8; i++) {
        oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, spriteFrameGfx(0, 0),
               -1, false, true, false, false, false);
    }
    oamUpdate(&oamMain);
}

void renderGameOver() {
    renderClearParticleLayer();
    renderFilledRect(40, 65, 176, 50, RGB15(4, 4, 16));
    renderFilledRect(50, 75, 156, 30, RGB15(28, 4, 4));
    renderText(128 - renderTextWidth(str(kUI[23])) / 2, 72, str(kUI[23]), RGB15(31, 10, 10));
    renderText(128 - renderTextWidth(str(kUI[22])) / 2, 92, str(kUI[22]), RGB15(20, 20, 20));
    oamUpdate(&oamMain);
}

static int victoryFrame = 0;

void renderVictory() {
    renderClearParticleLayer();
    victoryFrame++;

    // Hide OAM sprites
    for (int i = 0; i < 128; i++)
        oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, nullptr, -1, false, true, false, false, false);
    oamUpdate(&oamMain);

    // Deep dark background
    renderFilledRect(0, 0, 256, 192, RGB15(1, 1, 4));

    // Firework particles — 6 colors cycling, burst from random positions
    static const u16 kFireColors[] = {
        RGB15(31, 6, 6), RGB15(6, 10, 31), RGB15(6, 28, 6),
        RGB15(31, 28, 0), RGB15(20, 0, 31), RGB15(0, 28, 28)
    };
    // Spawn firework bursts every 20 frames
    if ((victoryFrame % 20) == 0) {
        int fx = 30 + rngRange(196);
        int fy = 20 + rngRange(100);
        u16 col = kFireColors[rngRange(6)];
        for (int p = 0; p < 8; p++) {
            Vec2 pvel = {static_cast<Fixed>(rngRange(40) - 20), static_cast<Fixed>(rngRange(40) - 20)};
            Vec2 ppos = {toFixed(fx), toFixed(fy)};
            spawnParticle(ppos, pvel, 20, static_cast<u8>(rngRange(6)));
        }
    }

    // Render active particles as colored dots
    for (int pi = 0; pi < MAX_PARTICLES; pi++) {
        Particle& p = gParticles[pi];
        if (!p.active) continue;
        p.pos += p.vel;
        p.vel.y = static_cast<Fixed>(p.vel.y + 1); // gravity
        p.life--;
        if (p.life == 0) { p.active = false; continue; }
        int px = p.pos.pixelX();
        int py = p.pos.pixelY();
        u16 col = kFireColors[p.color % 6];
        // Fade with life
        int bright = p.life * 31 / p.maxLife;
        if (bright < 1) bright = 1;
        int r = ((col & 0x1F) * bright) >> 5;
        int g = (((col >> 5) & 0x1F) * bright) >> 5;
        int b = (((col >> 10) & 0x1F) * bright) >> 5;
        renderPixel(px, py, RGB15(r, g, b));
        renderPixel(px+1, py, RGB15(r, g, b));
    }

    // Glowing banner background — pulsing gold
    int pulse = 4 + ((victoryFrame >> 2) & 3);
    renderFilledRect(16, 40, 224, 24, RGB15(pulse, pulse - 2, 0));
    renderFilledRect(18, 42, 220, 20, RGB15(2, 2, 8));

    // "VICTORY!" — large, gold, centered, pulsing brightness
    int vBright = 24 + ((victoryFrame >> 1) & 7);
    if (vBright > 31) vBright = 62 - vBright; // ping-pong
    const char* victoryStr = str(kUI[24]);
    int vw = renderTextWidth(victoryStr);
    renderText(128 - vw / 2, 47, victoryStr, RGB15(vBright, vBright - 4, 0));

    // Boss name
    renderFilledRect(30, 70, 196, 28, RGB15(3, 3, 12));
    const char* bossName = str(kUI[25]);
    const char* defeated = str(kUI[26]);
    renderText(128 - renderTextWidth(bossName) / 2, 73, bossName, RGB15(20, 16, 28));
    renderText(128 - renderTextWidth(defeated) / 2, 84, defeated, RGB15(16, 12, 24));

    // Stats display — show wave count and gold
    char statBuf[24];
    snprintf(statBuf, sizeof(statBuf), "%s 30  GOLD: %d", str(kUI[37]), gPlayer.gold);
    int sw = renderTextWidth(statBuf);
    renderText(128 - sw / 2, 108, statBuf, RGB15(20, 20, 20));

    // Companion roster — show how many survived
    snprintf(statBuf, sizeof(statBuf), "COMPANIONS: %d", gCompanionCount);
    sw = renderTextWidth(statBuf);
    renderText(128 - sw / 2, 120, statBuf, RGB15(14, 24, 14));

    // Options
    renderFilledRect(24, 145, 208, 14, RGB15(2, 10, 2));
    const char* endless = str(kUI[27]);
    renderText(128 - renderTextWidth(endless) / 2, 148, endless, RGB15(10, 31, 10));

    renderFilledRect(24, 164, 208, 14, RGB15(6, 6, 6));
    const char* menu = str(kUI[28]);
    renderText(128 - renderTextWidth(menu) / 2, 167, menu, RGB15(18, 18, 18));
}

// ---------------------------------------------------------------------------
// Pixel font rendering
// ---------------------------------------------------------------------------

int renderTextWidth(const char* text) {
    int len = 0;
    while (*text++) len++;
    return len * (CHAR_W + CHAR_GAP) - CHAR_GAP;
}

void renderText(int x, int y, const char* text, u16 color) {
    if (!particleFB) return;
    u16 c = color | BIT(15);
    for (int i = 0; text[i]; ++i) {
        int idx = charToFontIdx(text[i]);
        for (int row = 0; row < 7; ++row) {
            u8 bits = FONT_5X7[idx][row];
            for (int col = 0; col < 5; ++col) {
                if (bits & (0x10 >> col)) {
                    int px = x + i * 6 + col;
                    int py = y + row;
                    if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
                        particleFB[py * 256 + px] = c;
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Wave announcement
// ---------------------------------------------------------------------------

void waveAnnouncementStart(int waveNum) {
    // Build localized "WAVE X" / "WELLE X"
    char prefixBuf[12];
    snprintf(prefixBuf, sizeof(prefixBuf), "%s ", str(kUI[37]));
    const char* prefix = prefixBuf;
    int i = 0;
    while (prefix[i]) { annText[i] = prefix[i]; i++; }
    // Write digits (supports up to 99)
    if (waveNum >= 10) annText[i++] = static_cast<char>('0' + waveNum / 10);
    annText[i++] = static_cast<char>('0' + waveNum % 10);
    annText[i]   = '\0';
    annTimer    = 90;
    annMaxTimer = 90;
}

void waveAnnouncementClear() {
    const char* msg = str(kGameplay[0]);
    int i = 0;
    while (msg[i]) { annText[i] = msg[i]; i++; }
    annText[i]  = '\0';
    annTimer    = 60;
    annMaxTimer = 60;
}

void waveAnnouncementTick() {
    if (annTimer > 0) annTimer--;
}

void renderTriggerMergeFlash() {
    gMergeFlashTimer = 60;
}

void waveAnnouncementRender() {
    if (annTimer <= 0) return;

    // Fade: full brightness for first 2/3, then linearly dim to 0
    int fadeStart = annMaxTimer / 3;
    int bright;
    if (annTimer > fadeStart) {
        bright = 31;
    } else {
        // annTimer goes from fadeStart down to 0 → bright from 31 to 1
        bright = 1 + (annTimer * 30) / (fadeStart > 0 ? fadeStart : 1);
    }

    // Scale brightness into R, G, B channels (white → dim white)
    u16 color = RGB15(bright, bright, bright);

    // Center text horizontally; lower third of screen to avoid player
    int tw = renderTextWidth(annText);
    int tx = (SCREEN_W - tw) / 2;
    int ty = 155;

    // Dark shadow one pixel down-right for readability
    renderText(tx + 1, ty + 1, annText, RGB15(0, 0, 0));
    renderText(tx,     ty,     annText, color);
}

// ---------------------------------------------------------------------------
// Sprite scaling (software render from ROM data into framebuffer)
// ---------------------------------------------------------------------------

void renderSpriteScaled(int destX, int destY, int classId, int frame, int colorId, int scale) {
    const u8* tiles = spriteFrameROM(classId, frame);
    if (!tiles) return;

    int palSlot = spritePaletteSlot(colorId);
    const u16* pal = &SPRITE_PALETTE[palSlot * 16];

    for (int py = 0; py < 16; py++) {
        int tileRow = py / 8;
        int rowInTile = py % 8;
        for (int px = 0; px < 16; px++) {
            int tileCol = px / 8;
            int colInTile = px % 8;
            int tileIdx = tileRow * 2 + tileCol;
            int byteOff = tileIdx * 32 + rowInTile * 4 + colInTile / 2;
            u8 byte = tiles[byteOff];
            u8 nibble = (colInTile & 1) ? (byte >> 4) : (byte & 0xF);
            if (nibble == 0) continue; // transparent
            u16 color = pal[nibble] | BIT(15);
            int fx = destX + px * scale;
            int fy = destY + py * scale;
            renderFilledRect(fx, fy, scale, scale, color);
        }
    }
}

// ---------------------------------------------------------------------------
// Shot preview — data-driven from kClassDefs, bullets fire from enlarged sprite
// ---------------------------------------------------------------------------

constexpr int MUZ_X = 76;    // sprite right edge (8+64+4)
constexpr int MUZ_Y = 48;    // sprite vertical center (16+32)
constexpr int TGT_X = 200;   // target position
constexpr int TGT_Y = 48;

static void renderShotPreview(int frame, int fci, u16 bulletColor, int tier) {
    int ph = frame % 120;
    const ClassDef& cd = kClassDefs[fci];
    u16 bc = bulletColor | BIT(15);

    // Target dummy (always visible)
    u16 tgtColor = RGB15(16, 8, 8);
    renderFilledRect(TGT_X-5, TGT_Y-5, 10, 10, tgtColor);

    // Tier-scaled bullet count
    int count = cd.bulletCount + (tier >= 1 ? 1 : 0) + (tier >= 2 ? 1 : 0);
    bool pierce = (cd.flags & BFLAG_PIERCE) || (tier >= 2);

    // --- Exotic classes first ---
    switch (fci) {
        case 6: { // Warden — expanding pulse ring from sprite
            if (ph < 60) {
                int radius = ph * (48 + tier * 16) / 60;
                int bright = 31 - ph * 20 / 60;
                if (bright < 4) bright = 4;
                u16 col = RGB15(bright/2, bright/2, bright) | BIT(15);
                // Draw ring as 8 points
                for (int a = 0; a < 8; a++) {
                    static const s8 dx[8]={10,7,0,-7,-10,-7,0,7};
                    static const s8 dy[8]={0,-7,-10,-7,0,7,10,7};
                    renderPixel(MUZ_X-4 + dx[a]*radius/10, MUZ_Y + dy[a]*radius/10, col);
                }
            }
            return;
        }
        case 12: { // Thornshot — wall of thorns moving right slowly
            int thornCount = 5 + tier * 2;
            int halfN = thornCount / 2;
            if (ph < 80) {
                int bx = MUZ_X + ph * (TGT_X - MUZ_X) / 160; // slow travel
                for (int t = -halfN; t <= halfN; t++) {
                    renderFilledRect(bx-1, MUZ_Y + t*8 - 1, 3, 3, bc);
                }
            }
            return;
        }
        case 13: { // Seedling — mine drops, sits, explodes
            int mineX = MUZ_X + 30, mineY = MUZ_Y + 4;
            if (ph < 70) {
                // Mine sitting, pulsing
                u16 mc = (ph & 8) ? bc : (bc >> 1) & 0x3DEF;
                renderFilledCircle(mineX, mineY, 4, mc);
            } else if (ph < 95) {
                // Explosion expanding
                int r = (ph - 70) * 16 / 25 + 2;
                renderFilledCircle(mineX, mineY, r, RGB15(31, 20, 0));
            }
            return;
        }
        case 23: { // Trickster — radial burst
            int dirs = 8 + tier * 4;
            if (ph < 50) {
                for (int d = 0; d < dirs; d++) {
                    int angle = d * 360 / dirs;
                    // Simple 8-direction approximation
                    int dx = (angle < 45 || angle > 315) ? 1 : (angle > 135 && angle < 225) ? -1 : 0;
                    int dy = (angle > 45 && angle < 135) ? -1 : (angle > 225 && angle < 315) ? 1 : 0;
                    if (dx == 0 && dy == 0) dx = 1;
                    int bx = MUZ_X - 4 + dx * ph * 2;
                    int by = MUZ_Y + dy * ph * 2;
                    if (bx >= 0 && bx < 256 && by >= 0 && by < 192)
                        renderFilledRect(bx-1, by-1, 3, 3, bc);
                }
            }
            return;
        }
        case 26: { // Plague Doctor — poison cloud zone
            u16 projCol = RGB15(16, 0, 28) | BIT(15);
            u16 cloudCol = RGB15(10, 0, 18) | BIT(15);
            int landX = MUZ_X + 60, landY = MUZ_Y;
            if (ph < 30) {
                // Parabolic arc: rises then falls
                int bx = MUZ_X + ph * 60 / 30;
                int by = MUZ_Y - (15 * ph - ph * ph / 2) * 2 / 30;
                renderFilledRect(bx-1, by-1, 3, 3, projCol);
            } else if (ph < 60) {
                // Cloud expands from landing point
                int r = (ph - 30) * 14 / 30 + 2;
                renderFilledCircle(landX, landY, r, cloudCol);
            }
            return;
        }
        case 18: { // Gambler — single bullet, flickering size
            if (ph < 60) {
                int bx = MUZ_X + ph * (TGT_X - MUZ_X) / 60;
                int sz = 3 + (frame * 7 % 4); // random-ish size
                renderFilledRect(bx-sz/2, MUZ_Y-sz/2, sz, sz, bc);
            } else if (ph < 80) {
                renderFilledRect(TGT_X-5, TGT_Y-5, 10, 10, RGB15(31,31,31));
            }
            return;
        }
        case 14: { // Apothecary — green heal dart going LEFT toward friendly
            if (ph < 50) {
                int bx = MUZ_X - ph * 40 / 50;
                renderFilledRect(bx-2, MUZ_Y-1, 4, 3, RGB15(0,31,0));
            }
            // Draw a friendly companion icon on the left
            renderFilledRect(20, MUZ_Y-4, 8, 8, RGB15(0,20,0));
            return;
        }
        case 15: { // Vinecaller — 3 slow tendrils spreading right
            u16 vineCol = RGB15(0, 24, 0) | BIT(15);
            if (ph < 60) {
                for (int n = -1; n <= 1; n++) {
                    int bx = MUZ_X + ph / 2;
                    int by = MUZ_Y + n * ph / 6;
                    renderFilledRect(bx-1, by-1, 3, 3, vineCol);
                }
            }
            return;
        }
        case 17: { // Lifeshaper — green heal pulse ring
            if (ph < 50) {
                int r = ph * 40 / 50;
                for (int a = 0; a < 8; a++) {
                    static const s8 dx[8]={10,7,0,-7,-10,-7,0,7};
                    static const s8 dy[8]={0,-7,-10,-7,0,7,10,7};
                    renderPixel(MUZ_X-4 + dx[a]*r/10, MUZ_Y + dy[a]*r/10, RGB15(0,31,0));
                }
            }
            return;
        }
        case 19: { // Scavenger — backward fan (3 bullets going LEFT)
            if (ph < 60) {
                for (int n = -1; n <= 1; n++) {
                    int bx = MUZ_X - ph * 2;
                    int by = MUZ_Y + n * ph / 4;
                    if (bx >= 0) renderFilledRect(bx-1, by-1, 3, 3, bc);
                }
            }
            return;
        }
        case 20: { // Jester — ricochet pierce
            u16 ricoCol = RGB15(31, 28, 0) | BIT(15);
            // Bullet moves right, bounces off top wall, then off bottom wall
            int bx, by;
            if (ph < 20) {
                // Phase 1: travel right toward wall
                bx = MUZ_X + ph * (TGT_X + 20 - MUZ_X) / 20;
                by = MUZ_Y;
            } else if (ph < 40) {
                // Phase 2: bounce diagonally down-left
                int t2 = ph - 20;
                bx = TGT_X + 20 - t2 * 3;
                by = MUZ_Y + t2 * 2;
            } else {
                // Phase 3: bounce right again
                int t3 = ph - 40;
                bx = TGT_X + 20 - 20 * 3 + t3 * 3;
                by = MUZ_Y + 20 * 2 - t3 * 2;
            }
            if (bx >= 0 && bx < 256 && by >= 0 && by < 192)
                renderFilledRect(bx-1, by-1, 3, 3, ricoCol);
            return;
        }
        case 21: { // Mimic — copies nearest ally, color cycles
            static const u16 mimicCols[4] = {
                RGB15(31,6,6) | BIT(15),   // red
                RGB15(6,10,31) | BIT(15),  // blue
                RGB15(0,28,0) | BIT(15),   // green
                RGB15(31,28,0) | BIT(15)   // yellow
            };
            int colorIdx = (ph / 15) % 4;
            if (ph < 60) {
                int bx = MUZ_X + ph * (TGT_X - MUZ_X) / 60;
                renderFilledRect(bx-1, MUZ_Y-1, 3, 3, mimicCols[colorIdx]);
            }
            return;
        }
        case 29: { // Nightmare — fear pulse ring (purple)
            if (ph < 50) {
                int r = ph * (56 + tier * 16) / 50;
                for (int a = 0; a < 8; a++) {
                    static const s8 dx[8]={10,7,0,-7,-10,-7,0,7};
                    static const s8 dy[8]={0,-7,-10,-7,0,7,10,7};
                    renderPixel(MUZ_X-4 + dx[a]*r/10, MUZ_Y + dy[a]*r/10, RGB15(20,0,31));
                }
            }
            // Target runs away
            if (ph > 30 && ph < 80) {
                int tx = TGT_X + (ph-30)*2;
                renderFilledRect(tx-5, TGT_Y-5, 10, 10, RGB15(20,8,8));
            }
            return;
        }
        case 30: { // Drone Pilot — 3 bullets at different targets
            int targets = 3 + (tier >= 1 ? 1 : 0) + (tier >= 2 ? 1 : 0);
            if (targets > 5) targets = 5;
            if (ph < 60) {
                for (int t = 0; t < targets && t < 3; t++) {
                    int ty = TGT_Y - 20 + t * 20;
                    int bx = MUZ_X + ph * (TGT_X - MUZ_X) / 60;
                    int by = MUZ_Y + (ty - MUZ_Y) * ph / 60;
                    renderFilledRect(bx-1, by-1, 3, 3, bc);
                }
            }
            return;
        }
        case 32: { // Tesla Coil — primary + chain arcs
            int chains = 2 + tier;
            if (ph < 50) {
                int bx = MUZ_X + ph * (TGT_X - MUZ_X) / 50;
                renderFilledRect(bx-1, MUZ_Y-1, 3, 3, bc);
            } else if (ph < 80) {
                // Chain arcs from target to secondary positions
                for (int c = 0; c < chains && c < 3; c++) {
                    int arcY = TGT_Y - 15 + c * 15;
                    int arcX = TGT_X + 10 + (ph-50) * 2;
                    renderPixel(arcX, arcY, RGB15(0,28,31));
                    renderPixel(arcX+1, arcY-1, RGB15(0,28,31));
                }
            }
            return;
        }
    }

    // --- Data-driven path for non-exotic classes ---
    bool isAoE = (cd.flags & BFLAG_EXPLODE) && cd.aoeRadius > 0;
    bool isSlow = cd.flags & BFLAG_SLOW;
    bool isFreeze = cd.flags & BFLAG_FREEZE;
    bool isErase = cd.flags & BFLAG_ERASE;
    (void)isErase;

    if (isAoE) {
        // Bullet travels, then explosion circle
        if (ph < 50) {
            int bx = MUZ_X + ph * (TGT_X - MUZ_X) / 50;
            renderFilledRect(bx-2, MUZ_Y-2, 4, 4, bc);
        } else if (ph < 90) {
            int r = (ph-50) * cd.aoeRadius / 40 + 2;
            renderFilledCircle(TGT_X, TGT_Y, r, (bc >> 1) & 0x3DEF);
        }
    } else if (isSlow || isFreeze) {
        // Bullet hits, target changes color
        if (ph < 50) {
            int bx = MUZ_X + ph * (TGT_X - MUZ_X) / 50;
            renderFilledRect(bx-1, MUZ_Y-1, 3, 3, RGB15(14,22,31));
        }
        if (ph >= 50) {
            u16 statusCol = isFreeze ? RGB15(20,25,31) : RGB15(10,10,20);
            renderFilledRect(TGT_X-5, TGT_Y-5, 10, 10, statusCol);
            if (isFreeze) {
                renderFilledRect(TGT_X-6, TGT_Y-6, 12, 1, RGB15(25,28,31));
                renderFilledRect(TGT_X-6, TGT_Y+5, 12, 1, RGB15(25,28,31));
            }
        }
    } else if (count > 1) {
        // Multi-bullet spread
        if (ph < 60) {
            int spacing = 8;
            int startY = MUZ_Y - (count-1) * spacing / 2;
            for (int n = 0; n < count; n++) {
                int bx = MUZ_X + ph * (TGT_X - MUZ_X) / 60;
                int by = startY + n * spacing + (n - count/2) * ph / 8;
                renderFilledRect(bx-1, by-1, 3, 3, bc);
            }
        }
    } else {
        // Single bullet
        int endX = pierce ? 248 : TGT_X;
        if (ph < 60) {
            int bx = MUZ_X + ph * (endX - MUZ_X) / 60;
            int w = pierce ? 5 : 4;
            renderFilledRect(bx-w/2, MUZ_Y-1, w, 3, bc);
        }
        if (!pierce && ph >= 50 && ph < 65) {
            renderFilledRect(TGT_X-5, TGT_Y-5, 10, 10, RGB15(31,31,31));
        }
    }
}

// ---------------------------------------------------------------------------
// Upgrade path panel — drawn into the top-screen framebuffer.
//
// Layout (all coordinates relative to the panel origin at (panelX, panelY)):
//   Row 0 (+0 px)  — tier name for each of the 3 tiers, center-aligned in column
//   Row 1 (+10 px) — merge requirement: "3 to merge" / "2 to merge" / "MAX"
//   Row 2 (+20 px) — what changes at that tier vs T1 base
//
// Each of the three columns is colW pixels wide; arrows between columns are 6px.
// Current tier name is drawn bright white; the others are dim grey.
// currentTier: -1 means "none selected" (shop card, no owned instance yet) →
//              all three tiers are drawn dim so only the names are readable.
// ---------------------------------------------------------------------------

static void renderUpgradePath(int panelX, int panelY,
                              int colorIdx, int classId, int currentTier)
{
    // Column geometry: three slots, fit within 256 - panelX
    int availW = 256 - panelX - 4;  // leave 4px margin
    constexpr int ARROW_W = 6;
    int COL_W = (availW - 2 * ARROW_W) / 3;

    static const u16 kBright = RGB15(31, 31, 31);
    static const u16 kDim    = RGB15(12, 12, 12);
    static const u16 kGold   = RGB15(31, 28, 0);

    // Generic per-tier change labels (same for all classes in this build)
    static const char* kTierChange[3] = {
        "",              // T1: base, nothing extra
        "+1 shot",       // T2 bonus over T1
        "+2 shots",      // T3 bonus over T1 (second line drawn separately)
    };
    // T3 pierce annotation drawn as a second sub-line
    static const char* kT3Extra = "pierce all";

    // Merge requirement text per tier (the cost to reach the NEXT tier from here)
    static const char* kMergeReq[3] = {
        "3 to merge",   // T1 → T2
        "2 to merge",   // T2 → T3
        "MAX",          // T3: already at top
    };

    for (int t = 0; t < 3; ++t) {
        int colX = panelX + t * (COL_W + ARROW_W);

        // Color: bright if this is the current tier, dim otherwise.
        // If currentTier == -1 (shop view, not owned), all dim.
        u16 nameCol   = (t == currentTier) ? kBright : kDim;
        u16 detailCol = (t == currentTier) ? kGold   : kDim;

        // ── Tier name (truncated to fit column width) ──────────────────
        const char* fullName = str(kClassNames[colorIdx][classId][t]);
        // Truncate name to fit column: each char is 6px (5+1 gap)
        int maxChars = COL_W / 6;
        if (maxChars > 12) maxChars = 12;
        char nameBuf[14];
        int ni = 0;
        while (fullName[ni] && ni < maxChars) { nameBuf[ni] = fullName[ni]; ni++; }
        nameBuf[ni] = '\0';

        int nameW = renderTextWidth(nameBuf);
        int nameX = colX + (COL_W - nameW) / 2;
        if (nameX < colX) nameX = colX;
        renderText(nameX, panelY, nameBuf, nameCol);

        // ── Merge requirement (row 1, +11 px) ───────────────────────────
        const char* mergeStr = kMergeReq[t];
        int mergeW = renderTextWidth(mergeStr);
        int mergeX = colX + (COL_W - mergeW) / 2;
        if (mergeX < colX) mergeX = colX;
        renderText(mergeX, panelY + 11, mergeStr, detailCol);

        // ── What changes at this tier (row 2, +22 px) ───────────────────
        if (kTierChange[t][0] != '\0') {
            int changeW = renderTextWidth(kTierChange[t]);
            int changeX = colX + (COL_W - changeW) / 2;
            if (changeX < colX) changeX = colX;
            renderText(changeX, panelY + 22, kTierChange[t], detailCol);
        }
        // T3 second line: "pierce all" at +30 px
        if (t == 2) {
            int extraW = renderTextWidth(kT3Extra);
            int extraX = colX + (COL_W - extraW) / 2;
            if (extraX < colX) extraX = colX;
            renderText(extraX, panelY + 30, kT3Extra, detailCol);
        }

        // ── Arrow separator between columns ─────────────────────────────
        if (t < 2) {
            renderText(colX + COL_W, panelY, ">", kDim);
        }
    }
}

// ---------------------------------------------------------------------------
// Top-screen shop detail view
// ---------------------------------------------------------------------------

void renderShopDetail(int selectedCard) {
    renderClearParticleLayer();

    if (selectedCard < 0) {
        renderArmySummary();
        return;
    }

    const ShopCard& card = gShop.cards[selectedCard];

    // --- Perk detail view ---
    if (card.isPerk) {
        // Gold-themed background
        renderFilledRect(0, 0, 256, 192, RGB15(8, 6, 2) | BIT(15));

        // "PERK" label
        renderText(128 - renderTextWidth("PERK") / 2, 10, "PERK", RGB15(31, 31, 31));

        // Perk name large centered
        const char* perkName = str(kPerks[card.perkId].name);
        renderText(128 - renderTextWidth(perkName) / 2, 30, perkName, RGB15(31, 28, 0));

        // Description
        const char* desc = str(kPerks[card.perkId].desc);
        renderText(16, 60, desc, RGB15(28, 28, 20));

        // Price
        char priceBuf[16];
        snprintf(priceBuf, sizeof(priceBuf), "%dG", card.price);
        renderText(128 - renderTextWidth(priceBuf) / 2, 140, priceBuf, RGB15(31, 28, 0));

        // "PERMANENT" label
        renderText(128 - renderTextWidth("PERMANENT") / 2, 160, "PERMANENT", RGB15(20, 20, 20));

        // Hide OAM sprites during detail view
        for (int i = 0; i < 8; i++) {
            oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
                   SpriteColorFormat_16Color, spriteFrameGfx(0, 0),
                   -1, false, true, false, false, false);
        }
        oamUpdate(&oamMain);
        return;
    }

    // --- Companion detail view ---
    int colorIdx = static_cast<int>(card.color);
    int fullClassId = colorIdx * 6 + card.classId + 1; // +1 because classId 0 = player
    int colorId = colorIdx;

    // Background tint (darkened pill color)
    u16 tint = (pillColorToRGB(card.color) >> 2) & 0x39CE;
    renderFilledRect(0, 0, 256, 192, tint | BIT(15));

    // Large sprite (4x scale = 64x64) — left-aligned at x=8 to free right half
    // Preview shoot animation
    static int previewShootFrame = 0;
    previewShootFrame++;

    // Shot pattern frame counter (drives both shot pattern and idle bob)
    static int shotFrame = 0;
    shotFrame = (shotFrame + 1) % 120;

    // Recoil: shift sprite 2px left for first 4 frames of each 60-frame shoot cycle
    int phase = previewShootFrame % 60;
    int spriteOffsetX = (phase < 4) ? -2 : 0;

    // Idle bob: 1px down on the second half of the 120-frame shot cycle
    int spriteOffsetY = (shotFrame < 60) ? 0 : 1;

    // Muzzle flash: bright white rect for first 3 frames of each shoot cycle
    bool muzzleFlash = (phase < 3);

    renderSpriteScaled(8 + spriteOffsetX, 16 + spriteOffsetY, fullClassId, 0, colorId, 4);

    if (muzzleFlash) {
        // Muzzle point: right edge of 64px sprite (x=8+64=72) plus a couple pixels gap, vertically centred
        renderFilledRect(74, 44, 4, 4, RGB15(31, 31, 31) | BIT(15));
    }
    int fci = colorIdx * 6 + card.classId;
    renderShotPreview(shotFrame, fci, pillColorToRGB(card.color), 0); // tier 0 for shop cards

    // Class name centered above sprite area
    const char* name = str(kClassNames[colorIdx][card.classId][0]);
    int nameW = renderTextWidth(name);
    renderText(128 - nameW / 2, 4, name, RGB15(31, 31, 31));

    // Shot pattern
    renderText(8, 90, str(kUI[29]), RGB15(20, 20, 31));
    const char* shotDesc = str(kClassAbility[colorIdx][card.classId].shot);
    renderText(8, 100, shotDesc, RGB15(28, 28, 31));

    // Passive
    renderText(8, 115, str(kUI[30]), RGB15(20, 31, 20));
    const char* passiveDesc = str(kClassAbility[colorIdx][card.classId].passive);
    renderText(8, 125, passiveDesc, RGB15(28, 31, 28));

    // Synergy info — count all companions of this color owned
    int ownedCount = 0;
    for (int i = 0; i < MAX_COMPANIONS; i++) {
        if (gCompanions[i].active && gCompanions[i].color == card.color)
            ownedCount++;
    }
    const u16 synColor = pillColorToRGB(card.color);
    const char* colorAbbr = str(kColorAbbr[colorIdx]);
    // Helper: tier index from companion count (activates at 2, +1 per extra)
    auto synTierFor = [](int cnt) -> int {
        int ti = cnt - 2;
        if (ti < 0) ti = -1;
        if (ti >= SYN_TIERS) ti = SYN_TIERS - 1;
        return ti;
    };
    {
        // Current synergy line: "RED: 3 companions -> BLAZE active"
        char synBuf[48];
        int ti = synTierFor(ownedCount);
        if (ti >= 0) {
            const char* tname = str(kSynergyNames[colorIdx][ti]);
            snprintf(synBuf, sizeof(synBuf), "%s: %d -> %s active", colorAbbr, ownedCount, tname);
        } else {
            snprintf(synBuf, sizeof(synBuf), "%s: %d -", colorAbbr, ownedCount);
        }
        renderText(8, 135, synBuf, synColor);
    }
    {
        // Buy preview: "-> 4 companions -> DETONATION unlocks!"
        int afterCount = ownedCount + 1;
        int ti = synTierFor(afterCount);
        char buyBuf[48];
        if (ti >= 0) {
            const char* tname = str(kSynergyNames[colorIdx][ti]);
            int prevTi = synTierFor(ownedCount);
            if (ti > prevTi) {
                snprintf(buyBuf, sizeof(buyBuf), "->%d -> %s unlocks!", afterCount, tname);
            } else {
                snprintf(buyBuf, sizeof(buyBuf), "->%d -> %s", afterCount, tname);
            }
        } else {
            snprintf(buyBuf, sizeof(buyBuf), "->%d -", afterCount);
        }
        renderText(8, 143, buyBuf, RGB15(28, 28, 20));
    }

    // ── Upgrade path (y=152..192) ───────────────────────────────────────────
    // Pushed down by 7px to clear the two-line synergy block above.
    // Determine which tier is currently active for an owned copy of this class,
    // so we can highlight it.  If none owned, pass -1 (all dim).
    {
        int ownedTier = -1;
        for (int i = 0; i < MAX_COMPANIONS; i++) {
            if (gCompanions[i].active &&
                gCompanions[i].color   == card.color &&
                gCompanions[i].classId == card.classId) {
                ownedTier = gCompanions[i].tier;
                break;
            }
        }
        // Label row above the path
        renderText(8, 152, str(kUI[36]), RGB15(18, 18, 18));
        // Panel origin: x=8, y=162 — uses full 240px width, split into 3 cols of 54+6
        renderUpgradePath(8, 162, colorIdx, card.classId, ownedTier);
    }

    // Price — below the upgrade path (path ends at y=162+30=192; put price at y=183)
    char priceBuf[16];
    snprintf(priceBuf, sizeof(priceBuf), "%dG", card.price);
    int priceW = renderTextWidth(priceBuf);
    renderText(248 - priceW, 183, priceBuf, RGB15(31, 28, 0));

    // Hide OAM sprites during detail view
    for (int i = 0; i < 8; i++) {
        oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, spriteFrameGfx(0, 0),
               -1, false, true, false, false, false);
    }
    oamUpdate(&oamMain);
}

// ---------------------------------------------------------------------------
// Top-screen army summary (shown when no card selected in shop)
// ---------------------------------------------------------------------------

void renderArmySummary() {
    // Dark background
    renderFilledRect(0, 0, 256, 192, RGB15(2, 2, 4) | BIT(15));

    // Title
    const char* title = "SELECT A UNIT";
    int tw = renderTextWidth(title);
    renderText(128 - tw / 2, 4, title, RGB15(20, 20, 31));

    // List owned companions
    int y = 20;
    for (int i = 0; i < MAX_COMPANIONS; i++) {
        if (!gCompanions[i].active) continue;
        const Companion& c = gCompanions[i];
        int ci = static_cast<int>(c.color);
        const char* cname = str(kClassNames[ci][c.classId][c.tier]);
        char buf[40];
        snprintf(buf, sizeof(buf), "%s T%d", cname, c.tier + 1);
        renderText(8, y, buf, pillColorToRGB(c.color));
        y += 12;
    }

    // Gold
    char goldBuf[16];
    snprintf(goldBuf, sizeof(goldBuf), "GOLD: %d", gPlayer.gold);
    renderText(8, 150, goldBuf, RGB15(31, 28, 0));

    // Wave
    char waveBuf[16];
    snprintf(waveBuf, sizeof(waveBuf), "WAVE: %d", gameGetWave());
    renderText(8, 165, waveBuf, RGB15(20, 20, 31));

    // Synergy counts per color (new threshold: activates at 2 companions)
    int synY = 20;
    for (int ci = 0; ci < PILL_COLOR_COUNT; ci++) {
        int count = 0;
        for (int j = 0; j < MAX_COMPANIONS; j++) {
            if (gCompanions[j].active && static_cast<int>(gCompanions[j].color) == ci)
                count++;
        }
        if (count == 0) continue;
        int ti = count - 2;
        if (ti >= SYN_TIERS) ti = SYN_TIERS - 1;
        char sBuf[32];
        const char* abbr = str(kColorAbbr[ci]);
        if (ti >= 0) {
            const char* tname = str(kSynergyNames[ci][ti]);
            snprintf(sBuf, sizeof(sBuf), "%s:%d %s", abbr, count, tname);
        } else {
            snprintf(sBuf, sizeof(sBuf), "%s:%d", abbr, count);
        }
        renderText(170, synY, sBuf, PILL_COLORS[ci]);
        synY += 12;
    }

    // Hide OAM sprites
    for (int i = 0; i < 8; i++) {
        oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, spriteFrameGfx(0, 0),
               -1, false, true, false, false, false);
    }
    oamUpdate(&oamMain);
}

// ---------------------------------------------------------------------------
// Top-screen detail view for an OWNED companion (inventory bar tap)
// Layout (256x192):
//   Top-left  (x=8,  y=8..72)  — 4x scaled sprite (64x64) at current tier frame
//   Top-right (x=80, y=8..72)  — class name (tier-highlighted) + HP bar
//   Bottom    (y=82..192)      — shot / passive descriptions + tier progression
// ---------------------------------------------------------------------------

void renderCompanionDetail(int companionSlot) {
    renderClearParticleLayer();

    if (companionSlot < 0 || companionSlot >= MAX_COMPANIONS ||
        !gCompanions[companionSlot].active) {
        renderArmySummary();
        return;
    }

    const Companion& c = gCompanions[companionSlot];
    int colorIdx  = static_cast<int>(c.color);
    int classId   = c.classId;
    int tier      = c.tier;           // 0=T1, 1=T2, 2=T3
    int sprClassId = 1 + companionFullClassId(c);  // +1: slot 0 = player

    // Background — darkened pill tint, same convention as shop detail
    u16 tint = (pillColorToRGB(c.color) >> 2) & 0x39CE;
    renderFilledRect(0, 0, 256, 192, tint | BIT(15));

    // ── Scaled sprite (4x = 64x64), current tier frame ─────────────────────
    // tier*2+0 = first idle frame for that tier (matches renderGameplay logic)
    renderSpriteScaled(8, 8, sprClassId, tier * 2, colorIdx, 4);

    // ── Class name at current tier ──────────────────────────────────────────
    const char* tierName = str(kClassNames[colorIdx][classId][tier]);
    int nameW = renderTextWidth(tierName);
    renderText(80 + (168 - nameW) / 2, 8, tierName, RGB15(31, 31, 31));

    // ── HP bar (x=80..248, y=22..30) ────────────────────────────────────────
    {
        constexpr int BAR_X = 80, BAR_Y = 22, BAR_W = 168, BAR_H = 6;
        // Trough
        renderFilledRect(BAR_X, BAR_Y, BAR_W, BAR_H, RGB15(6, 6, 6));
        // Filled segment
        if (c.maxHp > 0) {
            int filled = (static_cast<int>(c.hp) * BAR_W) / c.maxHp;
            if (filled < 0) filled = 0;
            if (filled > BAR_W) filled = BAR_W;
            int hpPct = (static_cast<int>(c.hp) * 100) / c.maxHp;
            u16 barColor = (hpPct > 50) ? RGB15(0, 25, 0)
                         : (hpPct > 25) ? RGB15(31, 28, 0)
                                        : RGB15(31, 4, 4);
            if (filled > 0)
                renderFilledRect(BAR_X, BAR_Y, filled, BAR_H, barColor);
        }
        // HP numbers "HP: X/Y"
        char hpBuf[20];
        snprintf(hpBuf, sizeof(hpBuf), "HP: %d/%d", (int)c.hp, (int)c.maxHp);
        renderText(BAR_X, BAR_Y + BAR_H + 2, hpBuf, RGB15(28, 28, 28));
    }

    // ── Upgrade path (x=80, y=40..72) ──────────────────────────────────────
    // Three columns (T1/T2/T3) showing name, merge cost, and tier bonus.
    // Current tier is bright; others dim.
    renderUpgradePath(80, 40, colorIdx, classId, tier);

    // ── Divider ─────────────────────────────────────────────────────────────
    renderFilledRect(4, 76, 248, 1, RGB15(18, 18, 18));

    // ── Shot description (y=82) ─────────────────────────────────────────────
    renderText(8, 82, str(kUI[29]), RGB15(20, 20, 31));
    const char* shotDesc = str(kClassAbility[colorIdx][classId].shot);
    renderText(8, 92, shotDesc, RGB15(28, 28, 31));

    // ── Passive description (y=107) ─────────────────────────────────────────
    renderText(8, 107, str(kUI[30]), RGB15(20, 31, 20));
    const char* passiveDesc = str(kClassAbility[colorIdx][classId].passive);
    renderText(8, 117, passiveDesc, RGB15(28, 31, 28));

    // ── Shot pattern animation (right column, x=168..248, y=82..135) ────────
    static int cdShotFrame = 0;
    cdShotFrame = (cdShotFrame + 1) % 120;
    int fci = companionFullClassId(c);
    renderShotPreview(cdShotFrame, fci, pillColorToRGB(c.color), c.tier);

    // ── Color synergy (y=140..150) ──────────────────────────────────────────
    {
        int ownedColor = 0;
        for (int i = 0; i < MAX_COMPANIONS; ++i) {
            if (gCompanions[i].active && gCompanions[i].color == c.color)
                ownedColor++;
        }
        // Tier activates at 2 companions, +1 tier per extra (tier index = count - 2)
        int ti = ownedColor - 2;
        if (ti >= SYN_TIERS) ti = SYN_TIERS - 1;
        char synBuf[48];
        const char* abbr = str(kColorAbbr[colorIdx]);
        if (ti >= 0) {
            const char* tname = str(kSynergyNames[colorIdx][ti]);
            snprintf(synBuf, sizeof(synBuf), "%s: %d -> %s active", abbr, ownedColor, tname);
        } else {
            snprintf(synBuf, sizeof(synBuf), "%s: %d -", abbr, ownedColor);
        }
        renderText(8, 140, synBuf, pillColorToRGB(c.color));
    }

    // Hide OAM sprites during detail view
    for (int i = 0; i < 8; i++) {
        oamSet(&oamMain, i, 0, 0, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, spriteFrameGfx(0, 0),
               -1, false, true, false, false, false);
    }
    oamUpdate(&oamMain);
}
