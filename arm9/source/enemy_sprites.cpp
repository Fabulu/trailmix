#include "enemy_sprites.h"
#include "entities.h"
#include "render.h"
#include <nds.h>

// bin2s-generated headers for enemy sprite data (linear 4bpp)
// Regular enemies: 12 types x 3 sizes
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

// Boss enemies: 4 types (32px only)
#include "e_boss_sentinel_bin.h"
#include "e_boss_dreadnought_bin.h"
#include "e_boss_leviathan_bin.h"
#include "e_boss_nightmare_bin.h"

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
#include "e_boss_sentinel_pal_bin.h"
#include "e_boss_dreadnought_pal_bin.h"
#include "e_boss_leviathan_pal_bin.h"
#include "e_boss_nightmare_pal_bin.h"

// ---------------------------------------------------------------------------
// Lookup table: [type][sizeClass] -> sprite data pointer and palette
// For bosses (type >= 12), sizeClass is ignored (always use index 0)
// ---------------------------------------------------------------------------

struct EnemySpriteEntry {
    const u8* pixels;    // linear 4bpp data, 2 frames sequential
    const u8* palette;   // 16-color RGB555 palette (32 bytes)
    u8 frameW;           // pixel width of one frame
    u8 frameH;           // pixel height of one frame
};

// 16 types x 3 sizes (bosses only use slot 0)
static EnemySpriteEntry sSpriteTable[16][3];

void enemySpriteInit() {
    // Helper macro: populate table entry
    #define ENTRY(t, sc, dat, pal, w, h) \
        sSpriteTable[t][sc] = { dat, pal, w, h }

    // Regular enemies: all 3 sizes
    #define REG(t, name) \
        ENTRY(t, 0, e_##name##_s_bin, e_##name##_s_pal_bin, 8, 8); \
        ENTRY(t, 1, e_##name##_m_bin, e_##name##_m_pal_bin, 16, 16); \
        ENTRY(t, 2, e_##name##_l_bin, e_##name##_l_pal_bin, 24, 24)

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

    // Bosses: 32px, stored in slot 0
    ENTRY(ETYPE_BOSS_SENTINEL,    0, e_boss_sentinel_bin,    e_boss_sentinel_pal_bin,    32, 32);
    ENTRY(ETYPE_BOSS_DREADNOUGHT, 0, e_boss_dreadnought_bin, e_boss_dreadnought_pal_bin, 32, 32);
    ENTRY(ETYPE_BOSS_LEVIATHAN,   0, e_boss_leviathan_bin,   e_boss_leviathan_pal_bin,   32, 32);
    ENTRY(ETYPE_BOSS_NIGHTMARE_B, 0, e_boss_nightmare_bin,   e_boss_nightmare_pal_bin,   32, 32);

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
    if (type > 15) return;
    u8 sc = (type >= 12) ? 0 : sizeClass;
    if (sc > 2) sc = 1;

    const EnemySpriteEntry& entry = sSpriteTable[type][sc];
    if (!entry.pixels) return;

    int w = entry.frameW;
    int h = entry.frameH;
    int frameBytes = w * h / 2;  // 4bpp: 2 pixels per byte

    // Select frame data (frame 0 or 1)
    const u8* src = entry.pixels + (frame ? frameBytes : 0);

    // Build tinted palette lookup (16 RGB555 colors)
    const u16* rawPal = reinterpret_cast<const u16*>(entry.palette);
    u16 pal[16];
    for (int i = 0; i < 16; i++) {
        u16 c = rawPal[i];
        if (tintMode == 1) {
            // Frozen: shift toward blue
            int r = (c & 0x1F);
            int g = (c >> 5) & 0x1F;
            int b = (c >> 10) & 0x1F;
            r = (r + 20) / 3;      // wash out red
            g = (g + 25) / 3;      // wash out green
            b = (b * 2 + 31) / 3;  // boost blue
            pal[i] = (u16)(r | (g << 5) | (b << 10));
        } else if (tintMode == 2) {
            // Slowed: darken
            int r = ((c & 0x1F) * 2) / 3;
            int g = (((c >> 5) & 0x1F) * 2) / 3;
            int b = (((c >> 10) & 0x1F) * 2) / 3;
            pal[i] = (u16)(r | (g << 5) | (b << 10));
        } else {
            pal[i] = c;
        }
    }

    // Get framebuffer pointer (256px wide, RGB555)
    u16* fb = renderGetActiveBuffer();
    if (!fb) return;

    // Blit pixel by pixel with transparency and clipping
    int srcIdx = 0;
    for (int y = 0; y < h; y++) {
        int sy = screenY + y;
        if (sy < 0 || sy >= 192) {
            srcIdx += w / 2;
            continue;
        }
        u16* row = fb + sy * 256;
        for (int x = 0; x < w; x += 2) {
            u8 packed = src[srcIdx++];
            u8 lo = packed & 0x0F;
            u8 hi = (packed >> 4) & 0x0F;

            int sx0 = screenX + x;
            int sx1 = sx0 + 1;

            if (lo != 0 && sx0 >= 0 && sx0 < 256) {
                if (alpha >= 4) {
                    row[sx0] = pal[lo];
                } else if (alpha > 0) {
                    // Alpha blend for ghost flicker
                    u16 bg = row[sx0];
                    u16 fg = pal[lo];
                    int r = ((fg & 0x1F) * alpha + (bg & 0x1F) * (4 - alpha)) >> 2;
                    int g = (((fg >> 5) & 0x1F) * alpha + ((bg >> 5) & 0x1F) * (4 - alpha)) >> 2;
                    int b = (((fg >> 10) & 0x1F) * alpha + ((bg >> 10) & 0x1F) * (4 - alpha)) >> 2;
                    row[sx0] = (u16)(r | (g << 5) | (b << 10));
                }
            }

            if (hi != 0 && sx1 >= 0 && sx1 < 256) {
                if (alpha >= 4) {
                    row[sx1] = pal[hi];
                } else if (alpha > 0) {
                    u16 bg = row[sx1];
                    u16 fg = pal[hi];
                    int r = ((fg & 0x1F) * alpha + (bg & 0x1F) * (4 - alpha)) >> 2;
                    int g = (((fg >> 5) & 0x1F) * alpha + ((bg >> 5) & 0x1F) * (4 - alpha)) >> 2;
                    int b = (((fg >> 10) & 0x1F) * alpha + ((bg >> 10) & 0x1F) * (4 - alpha)) >> 2;
                    row[sx1] = (u16)(r | (g << 5) | (b << 10));
                }
            }
        }
    }
}

// Simple blit to an arbitrary framebuffer (no tint, no alpha — for title screen)
void enemySpriteBlitTo(u16* fb, u8 type, u8 sizeClass, u8 frame,
                       int screenX, int screenY) {
    if (!fb || type > 15) return;
    u8 sc = (type >= 12) ? 0 : sizeClass;
    if (sc > 2) sc = 1;

    const EnemySpriteEntry& entry = sSpriteTable[type][sc];
    if (!entry.pixels) return;

    int w = entry.frameW;
    int h = entry.frameH;
    int frameBytes = w * h / 2;
    const u8* src = entry.pixels + (frame ? frameBytes : 0);
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
