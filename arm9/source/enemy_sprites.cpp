#include "enemy_sprites.h"
#include "entities.h"
#include "render.h"
#include <nds.h>

// bin2s-generated headers for enemy sprite data (linear 4bpp)
// Regular enemies: 17 types x 3 sizes
#include "e_grunt_s_bin.h"
#include "e_grunt_m_bin.h"
#include "e_grunt_l_bin.h"
#include "e_charger_s_bin.h"
#include "e_charger_m_bin.h"
#include "e_charger_l_bin.h"
#include "e_brute_s_bin.h"
#include "e_brute_m_bin.h"
#include "e_brute_l_bin.h"
#include "e_spitter_s_bin.h"
#include "e_spitter_m_bin.h"
#include "e_spitter_l_bin.h"
#include "e_sniper_s_bin.h"
#include "e_sniper_m_bin.h"
#include "e_sniper_l_bin.h"
#include "e_splitter_s_bin.h"
#include "e_splitter_m_bin.h"
#include "e_splitter_l_bin.h"
#include "e_shield_s_bin.h"
#include "e_shield_m_bin.h"
#include "e_shield_l_bin.h"
#include "e_artillery_s_bin.h"
#include "e_artillery_m_bin.h"
#include "e_artillery_l_bin.h"
#include "e_nightmare_s_bin.h"
#include "e_nightmare_m_bin.h"
#include "e_nightmare_l_bin.h"
#include "e_ghost_s_bin.h"
#include "e_ghost_m_bin.h"
#include "e_ghost_l_bin.h"
#include "e_drone_s_bin.h"
#include "e_drone_m_bin.h"
#include "e_drone_l_bin.h"
#include "e_bomber_s_bin.h"
#include "e_bomber_m_bin.h"
#include "e_bomber_l_bin.h"
#include "e_medic_s_bin.h"
#include "e_medic_m_bin.h"
#include "e_medic_l_bin.h"
#include "e_anchor_s_bin.h"
#include "e_anchor_m_bin.h"
#include "e_anchor_l_bin.h"
#include "e_trapper_s_bin.h"
#include "e_trapper_m_bin.h"
#include "e_trapper_l_bin.h"
#include "e_hexer_s_bin.h"
#include "e_hexer_m_bin.h"
#include "e_hexer_l_bin.h"
#include "e_hive_s_bin.h"
#include "e_hive_m_bin.h"
#include "e_hive_l_bin.h"

// Boss enemies: 5 types (32px for original 4, 64px for Apothecary)
#include "e_boss_sentinel_bin.h"
#include "e_boss_dreadnought_bin.h"
#include "e_boss_leviathan_bin.h"
#include "e_boss_nightmare_bin.h"
#include "e_boss_apothecary_bin.h"

// Palette headers
#include "e_grunt_s_pal_bin.h"
#include "e_grunt_m_pal_bin.h"
#include "e_grunt_l_pal_bin.h"
#include "e_charger_s_pal_bin.h"
#include "e_charger_m_pal_bin.h"
#include "e_charger_l_pal_bin.h"
#include "e_brute_s_pal_bin.h"
#include "e_brute_m_pal_bin.h"
#include "e_brute_l_pal_bin.h"
#include "e_spitter_s_pal_bin.h"
#include "e_spitter_m_pal_bin.h"
#include "e_spitter_l_pal_bin.h"
#include "e_sniper_s_pal_bin.h"
#include "e_sniper_m_pal_bin.h"
#include "e_sniper_l_pal_bin.h"
#include "e_splitter_s_pal_bin.h"
#include "e_splitter_m_pal_bin.h"
#include "e_splitter_l_pal_bin.h"
#include "e_shield_s_pal_bin.h"
#include "e_shield_m_pal_bin.h"
#include "e_shield_l_pal_bin.h"
#include "e_artillery_s_pal_bin.h"
#include "e_artillery_m_pal_bin.h"
#include "e_artillery_l_pal_bin.h"
#include "e_nightmare_s_pal_bin.h"
#include "e_nightmare_m_pal_bin.h"
#include "e_nightmare_l_pal_bin.h"
#include "e_ghost_s_pal_bin.h"
#include "e_ghost_m_pal_bin.h"
#include "e_ghost_l_pal_bin.h"
#include "e_drone_s_pal_bin.h"
#include "e_drone_m_pal_bin.h"
#include "e_drone_l_pal_bin.h"
#include "e_bomber_s_pal_bin.h"
#include "e_bomber_m_pal_bin.h"
#include "e_bomber_l_pal_bin.h"
#include "e_medic_s_pal_bin.h"
#include "e_medic_m_pal_bin.h"
#include "e_medic_l_pal_bin.h"
#include "e_anchor_s_pal_bin.h"
#include "e_anchor_m_pal_bin.h"
#include "e_anchor_l_pal_bin.h"
#include "e_trapper_s_pal_bin.h"
#include "e_trapper_m_pal_bin.h"
#include "e_trapper_l_pal_bin.h"
#include "e_hexer_s_pal_bin.h"
#include "e_hexer_m_pal_bin.h"
#include "e_hexer_l_pal_bin.h"
#include "e_hive_s_pal_bin.h"
#include "e_hive_m_pal_bin.h"
#include "e_hive_l_pal_bin.h"
#include "e_boss_sentinel_pal_bin.h"
#include "e_boss_dreadnought_pal_bin.h"
#include "e_boss_leviathan_pal_bin.h"
#include "e_boss_nightmare_pal_bin.h"
#include "e_boss_apothecary_pal_bin.h"

// ---------------------------------------------------------------------------
// Lookup table: [type][sizeClass] -> sprite data pointer and palette
// For bosses (type >= ETYPE_BOSS_SENTINEL), sizeClass is ignored (always use index 0)
// ---------------------------------------------------------------------------

struct EnemySpriteEntry {
    const u8* pixels;    // linear 4bpp data, N frames sequential
    const u8* palette;   // 16-color RGB555 palette (32 bytes)
    u8 frameW;           // pixel width of one frame
    u8 frameH;           // pixel height of one frame
    u8 frameCount;       // number of animation frames (2 for regular, 8 for Apothecary)
};

// ETYPE_COUNT types x 3 sizes (bosses only use slot 0)
static EnemySpriteEntry sSpriteTable[ETYPE_COUNT][3];

void enemySpriteInit() {
    // Helper macro: populate table entry
    #define ENTRY(t, sc, dat, pal, w, h, fc) \
        sSpriteTable[t][sc] = { dat, pal, w, h, fc }

    // Regular enemies: all 3 sizes, 2 animation frames each
    #define REG(t, name) \
        ENTRY(t, 0, e_##name##_s_bin, e_##name##_s_pal_bin, 8, 8, 2); \
        ENTRY(t, 1, e_##name##_m_bin, e_##name##_m_pal_bin, 16, 16, 2); \
        ENTRY(t, 2, e_##name##_l_bin, e_##name##_l_pal_bin, 24, 24, 2)

    REG(ETYPE_GRUNT,       grunt);
    REG(ETYPE_CHARGER,     charger);
    REG(ETYPE_BRUTE,       brute);
    REG(ETYPE_SPITTER,     spitter);
    REG(ETYPE_SNIPER,      sniper);
    REG(ETYPE_SPLITTER,    splitter);
    REG(ETYPE_SHIELD,      shield);
    REG(ETYPE_ARTILLERY,   artillery);
    REG(ETYPE_NIGHTMARE,   nightmare);
    REG(ETYPE_GHOST,       ghost);
    REG(ETYPE_SWARM_DRONE, drone);
    REG(ETYPE_BOMBER,      bomber);
    REG(ETYPE_MEDIC,       medic);
    REG(ETYPE_ANCHOR,      anchor);
    REG(ETYPE_TRAPPER,     trapper);
    REG(ETYPE_HEXER,       hexer);
    REG(ETYPE_HIVE,        hive);

    // Bosses: 32px, stored in slot 0, 2 animation frames
    ENTRY(ETYPE_BOSS_SENTINEL,    0, e_boss_sentinel_bin,    e_boss_sentinel_pal_bin,    32, 32, 2);
    ENTRY(ETYPE_BOSS_DREADNOUGHT, 0, e_boss_dreadnought_bin, e_boss_dreadnought_pal_bin, 32, 32, 2);
    ENTRY(ETYPE_BOSS_LEVIATHAN,   0, e_boss_leviathan_bin,   e_boss_leviathan_pal_bin,   32, 32, 2);
    ENTRY(ETYPE_BOSS_NIGHTMARE_B, 0, e_boss_nightmare_bin,   e_boss_nightmare_pal_bin,   32, 32, 2);

    // Apothecary final boss: 64px, 8 animation frames
    sSpriteTable[ETYPE_BOSS_APOTHECARY][0] = { e_boss_apothecary_bin, e_boss_apothecary_pal_bin, 64, 64, 8 };

    #undef REG
    #undef ENTRY
}

// ---------------------------------------------------------------------------
// Software blitter: draw linear 4bpp sprite data to the active framebuffer
// ---------------------------------------------------------------------------

// Access the active framebuffer pointer from render.cpp
extern u16* renderGetActiveBuffer();

void enemySpriteBlitMain(u8 type, u8 sizeClass, u8 frame,
                         int screenX, int screenY,
                         u8 tintMode, u8 alpha) {
    if (type >= ETYPE_COUNT) return;
    u8 sc = (type >= ETYPE_BOSS_SENTINEL) ? 0 : sizeClass;
    if (sc > 2) sc = 1;

    const EnemySpriteEntry& entry = sSpriteTable[type][sc];
    if (!entry.pixels) return;

    int w = entry.frameW;
    int h = entry.frameH;

    // Clamp frame to valid range
    if (frame >= entry.frameCount) frame = 0;

    // Early reject: fully off-screen
    if (screenX + w <= 0 || screenX >= 256 || screenY + h <= 0 || screenY >= 192)
        return;

    u16* fb = renderGetActiveBuffer();
    if (!fb) return;

    int frameBytes = w * h / 2;
    const u8* src = entry.pixels + frame * frameBytes;

    // Pre-build palette with BIT(15) baked in + tint applied once
    const u16* rawPal = reinterpret_cast<const u16*>(entry.palette);
    u16 pal[16];
    if (tintMode == 1) {
        for (int i = 0; i < 16; i++) {
            u16 c = rawPal[i];
            int r = ((c & 0x1F) + 20) / 3;
            int g = (((c >> 5) & 0x1F) + 25) / 3;
            int b = (((c >> 10) & 0x1F) * 2 + 31) / 3;
            pal[i] = (u16)(r | (g << 5) | (b << 10)) | BIT(15);
        }
    } else if (tintMode == 2) {
        for (int i = 0; i < 16; i++) {
            u16 c = rawPal[i];
            int r = ((c & 0x1F) * 2) / 3;
            int g = (((c >> 5) & 0x1F) * 2) / 3;
            int b = (((c >> 10) & 0x1F) * 2) / 3;
            pal[i] = (u16)(r | (g << 5) | (b << 10)) | BIT(15);
        }
    } else if (tintMode == 3) {
        // White flash: shift all channels toward white
        for (int i = 0; i < 16; i++) {
            u16 c = rawPal[i];
            int r = ((c & 0x1F) + 31) >> 1;
            int g = (((c >> 5) & 0x1F) + 31) >> 1;
            int b = (((c >> 10) & 0x1F) + 31) >> 1;
            pal[i] = (u16)(r | (g << 5) | (b << 10)) | BIT(15);
        }
    } else {
        for (int i = 0; i < 16; i++)
            pal[i] = rawPal[i] | BIT(15);
    }
    pal[0] = 0; // index 0 stays transparent (sentinel for skip)

    // Clamp Y range to visible rows
    int y0 = (screenY < 0) ? -screenY : 0;
    int y1 = (screenY + h > 192) ? 192 - screenY : h;

    // Clamp X range
    int x0 = (screenX < 0) ? -screenX : 0;
    int x1 = (screenX + w > 256) ? 256 - screenX : w;
    // Align x0 to even for 4bpp byte boundaries
    x0 &= ~1;

    // Ghost alpha path (rare — only ETYPE_GHOST when not frozen)
    if (alpha < 4 && alpha > 0) {
        const u8* rowSrc = src + y0 * (w / 2);
        for (int y = y0; y < y1; y++) {
            u16* row = fb + (screenY + y) * 256;
            const u8* lineSrc = rowSrc;
            for (int x = 0; x < w; x += 2) {
                u8 packed = *lineSrc++;
                int sx0 = screenX + x;
                if (sx0 >= x0 + screenX && sx0 < x1 + screenX) {
                    u8 lo = packed & 0x0F;
                    if (lo) {
                        u16 fg = pal[lo]; u16 bg = row[sx0];
                        int r = ((fg & 0x1F) * alpha + (bg & 0x1F) * (4 - alpha)) >> 2;
                        int g = (((fg>>5)&0x1F) * alpha + ((bg>>5)&0x1F) * (4-alpha)) >> 2;
                        int b = (((fg>>10)&0x1F) * alpha + ((bg>>10)&0x1F) * (4-alpha)) >> 2;
                        row[sx0] = (u16)(r | (g<<5) | (b<<10)) | BIT(15);
                    }
                    u8 hi = packed >> 4;
                    if (hi) {
                        u16 fg = pal[hi]; u16 bg = row[sx0+1];
                        int r = ((fg & 0x1F) * alpha + (bg & 0x1F) * (4 - alpha)) >> 2;
                        int g = (((fg>>5)&0x1F) * alpha + ((bg>>5)&0x1F) * (4-alpha)) >> 2;
                        int b = (((fg>>10)&0x1F) * alpha + ((bg>>10)&0x1F) * (4-alpha)) >> 2;
                        row[sx0+1] = (u16)(r | (g<<5) | (b<<10)) | BIT(15);
                    }
                }
            }
            rowSrc += w / 2;
        }
        return;
    }

    // Fast opaque path (most enemies most of the time)
    // Process only visible rows, skip to correct source offset
    const u8* rowSrc = src + y0 * (w / 2);
    for (int y = y0; y < y1; y++) {
        u16* row = fb + (screenY + y) * 256 + screenX;
        const u8* lineSrc = rowSrc;

        // Skip clipped left columns
        lineSrc += x0 / 2;

        for (int x = x0; x < x1; x += 2) {
            u8 packed = *lineSrc++;
            // Unpack both pixels — pal[0] == 0, writing 0 is harmless but
            // we still skip it to avoid overwriting background
            u16 c0 = pal[packed & 0x0F];
            u16 c1 = pal[packed >> 4];
            if (c0) row[x]     = c0;
            if (c1) row[x + 1] = c1;
        }
        rowSrc += w / 2;
    }
}

// Simple blit to an arbitrary framebuffer (no tint, no alpha — for title screen)
void enemySpriteBlitTo(u16* fb, u8 type, u8 sizeClass, u8 frame,
                       int screenX, int screenY) {
    if (!fb || type >= ETYPE_COUNT) return;
    u8 sc = (type >= ETYPE_BOSS_SENTINEL) ? 0 : sizeClass;
    if (sc > 2) sc = 1;

    const EnemySpriteEntry& entry = sSpriteTable[type][sc];
    if (!entry.pixels) return;

    int w = entry.frameW;
    int h = entry.frameH;

    // Clamp frame to valid range
    if (frame >= entry.frameCount) frame = 0;

    int frameBytes = w * h / 2;
    const u8* src = entry.pixels + frame * frameBytes;
    const u16* rawPal = reinterpret_cast<const u16*>(entry.palette);

    int srcIdx = 0;
    for (int y = 0; y < h; y++) {
        int sy = screenY + y;
        if (sy < 0 || sy >= 192) { srcIdx += w / 2; continue; }
        u16* row = fb + sy * 256;
        for (int x = 0; x < w; x += 2) {
            u8 packed = src[srcIdx++];
            u8 lo = packed & 0x0F;
            u8 hi = (packed >> 4) & 0x0F;
            int sx0 = screenX + x;
            if (lo != 0 && sx0 >= 0 && sx0 < 256)
                row[sx0] = rawPal[lo] | BIT(15);
            if (hi != 0 && sx0 + 1 >= 0 && sx0 + 1 < 256)
                row[sx0 + 1] = rawPal[hi] | BIT(15);
        }
    }
}
