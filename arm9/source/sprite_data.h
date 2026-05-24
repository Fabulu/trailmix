#ifndef SPRITE_DATA_H
#define SPRITE_DATA_H
#include <nds.h>

// Total number of sprite classes (1 player + 36 companions)
#define SPRITE_CLASS_COUNT 37
// Frames per sprite sheet
#define SPRITE_FRAME_COUNT 6
// Bytes per frame (4 tiles * 32 bytes each)
#define SPRITE_FRAME_BYTES 128
// Bytes per sprite sheet (6 frames)
#define SPRITE_SHEET_BYTES 768

// Load all sprite tile data into VRAM_E and palettes into SPRITE_PALETTE
void spriteLoadAll();

// Get the VRAM tile pointer for a given class sprite and animation frame
// classId: 0-36 (0=player, 1-6=red, 7-12=blue, 13-18=green,
//                19-24=yellow, 25-30=purple, 31-36=cyan)
// frame: 0-5 (T1 idle1, T1 idle2, T2 idle1, T2 idle2, T3 idle1, T3 idle2)
u16* spriteFrameGfx(int classId, int frame);

// Palette slot for a color (0=red, 1=blue, 2=green, 3=yellow, 4=purple, 5=cyan, 6=player)
int spritePaletteSlot(int colorId);

// Get a pointer to the raw 4bpp tile data in ROM for a given classId+frame
// (does not go through VRAM — useful for software scaling)
const u8* spriteFrameROM(int classId, int frame);

#endif
