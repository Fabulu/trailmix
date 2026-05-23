#include "render.h"
#include "player.h"
#include "entities.h"

static u16* particleFB = nullptr;

// Palette of pill colors in RGB15
static const u16 PILL_COLORS[] = {
    RGB15(31, 0, 0),   // Red
    RGB15(0, 10, 31),  // Blue
    RGB15(0, 28, 0),   // Green
    RGB15(31, 28, 0),  // Yellow
};

void renderInit() {
    // Top screen — Main engine
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankB(VRAM_B_MAIN_SPRITE);

    // BG2: tiled arena background (placeholder — solid color for now)
    // BG3: 16-bit bitmap for particles
    int bgParticle = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    particleFB = bgGetGfxPtr(bgParticle);

    // Set up OAM for sprites
    oamInit(&oamMain, SpriteMapping_1D_32, false);

    // Bottom screen — Sub engine
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    vramSetBankD(VRAM_D_SUB_SPRITE);

    oamInit(&oamSub, SpriteMapping_1D_32, false);

    // Clear particle layer
    renderClearParticleLayer();
}

void renderClearParticleLayer() {
    if (!particleFB) return;
    dmaFillHalfWords(0, particleFB, 256 * 192 * 2);
}

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

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            particleFB[py * 256 + px] = c;
        }
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

        // Integer square root approximation
        int dx = 0;
        while ((dx + 1) * (dx + 1) <= dx2) dx++;

        int x0 = (cx - dx < 0) ? 0 : cx - dx;
        int x1 = (cx + dx >= SCREEN_W) ? SCREEN_W - 1 : cx + dx;

        for (int px = x0; px <= x1; ++px) {
            particleFB[py * 256 + px] = c;
        }
    }
}

u16 pillColorToRGB(PillColor c) {
    return PILL_COLORS[static_cast<int>(c)];
}

u16 tierGlow(PillColor c, PillTier t) {
    u16 base = pillColorToRGB(c);
    // Brighten based on tier
    int boost = static_cast<int>(t) * 4;
    int r = (base & 0x1F) + boost;
    int g = ((base >> 5) & 0x1F) + boost;
    int b = ((base >> 10) & 0x1F) + boost;
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (b > 31) b = 31;
    return RGB15(r, g, b);
}

void renderGameplay() {
    renderClearParticleLayer();

    // Draw player (capsule shape — rect with rounded ends for now)
    int px = gPlayer.pos.pixelX();
    int py = gPlayer.pos.pixelY();
    renderFilledRect(px - 4, py - 6, 8, 12, RGB15(31, 31, 31));
    renderFilledCircle(px, py - 6, 4, RGB15(31, 31, 31));
    renderFilledCircle(px, py + 5, 4, RGB15(31, 31, 31));

    // Draw enemies as geometric shapes
    for (const auto& e : gEnemies) {
        if (!e.active) continue;
        int ex = e.pos.pixelX();
        int ey = e.pos.pixelY();
        int half = e.size / 2;
        // Different shapes per type — all just rects/circles for now
        if (e.type % 2 == 0) {
            renderFilledRect(ex - half, ey - half, e.size, e.size, RGB15(31, 10, 10));
        } else {
            renderFilledCircle(ex, ey, half, RGB15(20, 10, 31));
        }
    }

    // Draw bullets
    for (const auto& b : gBullets) {
        if (!b.active) continue;
        renderFilledRect(b.pos.pixelX() - 1, b.pos.pixelY() - 1, 3, 3, RGB15(31, 31, 20));
    }

    // Draw pill drops
    for (const auto& p : gPills) {
        if (!p.active) continue;
        u16 color = tierGlow(p.color, p.tier);
        renderFilledCircle(p.pos.pixelX(), p.pos.pixelY(), 4, color);
    }

    // Draw particles
    for (const auto& p : gParticles) {
        if (!p.active) continue;
        // Fade by life ratio
        int brightness = p.maxLife > 0 ? (static_cast<int>(p.life) * 31) / p.maxLife : 0;
        if (brightness < 1) brightness = 1;
        u16 color = RGB15(brightness, brightness, brightness);
        renderPixel(p.pos.pixelX(), p.pos.pixelY(), color);
        renderPixel(p.pos.pixelX() + 1, p.pos.pixelY(), color);
        renderPixel(p.pos.pixelX(), p.pos.pixelY() + 1, color);
    }

    oamUpdate(&oamMain);
}

void renderMenu() {
    renderClearParticleLayer();
    // Title text drawn via sub screen tiles (TODO)
    renderFilledRect(60, 70, 136, 30, RGB15(10, 10, 31));
    oamUpdate(&oamMain);
}

void renderGameOver() {
    renderClearParticleLayer();
    renderFilledRect(60, 80, 136, 30, RGB15(31, 5, 5));
    oamUpdate(&oamMain);
}
