#ifndef ENEMY_SPRITES_H
#define ENEMY_SPRITES_H

#include <nds.h>

// Initialize enemy sprite lookup tables (call once at startup)
void enemySpriteInit();

// Blit an enemy sprite to the top-screen bitmap framebuffer.
// type: ETYPE_* constant (0-15)
// sizeClass: SIZE_SMALL/MEDIUM/LARGE (0-2), ignored for bosses
// frame: animation frame (0 or 1)
// screenX, screenY: top-left corner in screen space
// tintMode: 0=normal, 1=frozen (blue shift), 2=slowed (darkened)
// alpha: 0-4, 4=fully opaque, 0=invisible (for ghost flicker)
void enemySpriteBlitMain(u8 type, u8 sizeClass, u8 frame,
                         int screenX, int screenY,
                         u8 tintMode = 0, u8 alpha = 4);

// Blit to an arbitrary framebuffer (for language select screen etc.)
void enemySpriteBlitTo(u16* fb, u8 type, u8 sizeClass, u8 frame,
                       int screenX, int screenY);

#endif
