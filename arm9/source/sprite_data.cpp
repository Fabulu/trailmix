#include "sprite_data.h"
#include <nds.h>
#include <string.h>

// ---- bin2s-generated headers for sprite tile data ----
#include "spr_player_bin.h"
#include "spr_red_1_bin.h"
#include "spr_red_2_bin.h"
#include "spr_red_3_bin.h"
#include "spr_red_4_bin.h"
#include "spr_red_5_bin.h"
#include "spr_red_6_bin.h"
#include "spr_blue_1_bin.h"
#include "spr_blue_2_bin.h"
#include "spr_blue_3_bin.h"
#include "spr_blue_4_bin.h"
#include "spr_blue_5_bin.h"
#include "spr_blue_6_bin.h"
#include "spr_green_1_bin.h"
#include "spr_green_2_bin.h"
#include "spr_green_3_bin.h"
#include "spr_green_4_bin.h"
#include "spr_green_5_bin.h"
#include "spr_green_6_bin.h"
#include "spr_yellow_1_bin.h"
#include "spr_yellow_2_bin.h"
#include "spr_yellow_3_bin.h"
#include "spr_yellow_4_bin.h"
#include "spr_yellow_5_bin.h"
#include "spr_yellow_6_bin.h"
#include "spr_purple_1_bin.h"
#include "spr_purple_2_bin.h"
#include "spr_purple_3_bin.h"
#include "spr_purple_4_bin.h"
#include "spr_purple_5_bin.h"
#include "spr_purple_6_bin.h"
#include "spr_cyan_1_bin.h"
#include "spr_cyan_2_bin.h"
#include "spr_cyan_3_bin.h"
#include "spr_cyan_4_bin.h"
#include "spr_cyan_5_bin.h"
#include "spr_cyan_6_bin.h"

// ---- bin2s-generated headers for palette data ----
#include "pal_red_bin.h"
#include "pal_blue_bin.h"
#include "pal_green_bin.h"
#include "pal_yellow_bin.h"
#include "pal_purple_bin.h"
#include "pal_cyan_bin.h"
#include "pal_player_bin.h"

// Sprite tile data pointers in load order (matches classId)
// classId 0 = player, 1-6 = red_1..red_6, 7-12 = blue_1..blue_6, etc.
static const u8* spriteData[SPRITE_CLASS_COUNT] = {
    spr_player_bin,
    spr_red_1_bin, spr_red_2_bin, spr_red_3_bin,
    spr_red_4_bin, spr_red_5_bin, spr_red_6_bin,
    spr_blue_1_bin, spr_blue_2_bin, spr_blue_3_bin,
    spr_blue_4_bin, spr_blue_5_bin, spr_blue_6_bin,
    spr_green_1_bin, spr_green_2_bin, spr_green_3_bin,
    spr_green_4_bin, spr_green_5_bin, spr_green_6_bin,
    spr_yellow_1_bin, spr_yellow_2_bin, spr_yellow_3_bin,
    spr_yellow_4_bin, spr_yellow_5_bin, spr_yellow_6_bin,
    spr_purple_1_bin, spr_purple_2_bin, spr_purple_3_bin,
    spr_purple_4_bin, spr_purple_5_bin, spr_purple_6_bin,
    spr_cyan_1_bin, spr_cyan_2_bin, spr_cyan_3_bin,
    spr_cyan_4_bin, spr_cyan_5_bin, spr_cyan_6_bin,
};

// Palette data pointers: 0=red, 1=blue, 2=green, 3=yellow, 4=purple, 5=cyan, 6=player
static const u8* paletteData[7] = {
    pal_red_bin,
    pal_blue_bin,
    pal_green_bin,
    pal_yellow_bin,
    pal_purple_bin,
    pal_cyan_bin,
    pal_player_bin,
};

// VRAM base address for each class's tile data (set during spriteLoadAll)
static u16* vramBase[SPRITE_CLASS_COUNT];

void spriteLoadAll() {
    // Copy all sprite tile data sequentially into VRAM_E
    u8* dest = (u8*)SPRITE_GFX;

    for (int i = 0; i < SPRITE_CLASS_COUNT; i++) {
        vramBase[i] = (u16*)dest;
        dmaCopy(spriteData[i], dest, SPRITE_SHEET_BYTES);
        dest += SPRITE_SHEET_BYTES;
    }

    // Load palettes into extended palette slots (or standard palette)
    // Standard OBJ palette: 7 palettes * 16 colors * 2 bytes = 224 bytes
    // SPRITE_PALETTE is u16*, each palette slot is 16 colors (32 bytes)
    for (int i = 0; i < 7; i++) {
        dmaCopy(paletteData[i], &SPRITE_PALETTE[i * 16], 32);
    }
}

u16* spriteFrameGfx(int classId, int frame) {
    if (classId < 0 || classId >= SPRITE_CLASS_COUNT) return vramBase[0];
    if (frame < 0 || frame >= SPRITE_FRAME_COUNT) frame = 0;

    // Each frame is 4 tiles (128 bytes = 64 u16s)
    // Return pointer offset into this class's VRAM region
    return vramBase[classId] + (frame * SPRITE_FRAME_BYTES / 2);
}

int spritePaletteSlot(int colorId) {
    // 0=red, 1=blue, 2=green, 3=yellow, 4=purple, 5=cyan, 6=player
    if (colorId < 0 || colorId > 6) return 0;
    return colorId;
}

const u8* spriteFrameROM(int classId, int frame) {
    if (classId < 0 || classId >= SPRITE_CLASS_COUNT) return nullptr;
    if (frame < 0 || frame >= SPRITE_FRAME_COUNT) frame = 0;
    return spriteData[classId] + frame * SPRITE_FRAME_BYTES;
}
