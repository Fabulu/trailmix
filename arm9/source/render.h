#ifndef RENDER_H
#define RENDER_H

#include "types.h"

void renderInit();
void renderGameplay();
void renderMenu();
void renderGameOver();

// Low-level drawing into the particle framebuffer
void renderClearParticleLayer();
void renderPixel(int x, int y, u16 color);
void renderFilledRect(int x, int y, int w, int h, u16 color);
void renderFilledCircle(int cx, int cy, int r, u16 color);

// Color helpers
u16 pillColorToRGB(PillColor c);
u16 tierGlow(PillColor c, PillTier t);

#endif // RENDER_H
