#ifndef RENDER_H
#define RENDER_H

#include "types.h"

void renderInit();
void renderFlip();
void renderGameplay();
void renderMenu();
void renderGameOver();
void renderVictory();

// Low-level drawing into the particle framebuffer (top screen)
void renderClearParticleLayer();
void renderPixel(int x, int y, u16 color);
void renderFilledRect(int x, int y, int w, int h, u16 color);
void renderFilledCircle(int cx, int cy, int r, u16 color);

// Sub-screen (bottom) bitmap framebuffer setup — call once from uiInit()
void renderInitSub();

// Low-level drawing into the sub-screen bitmap framebuffer (bottom screen)
void renderClearSub();
void renderFilledRectSub(int x, int y, int w, int h, u16 color);
void renderTextSub(int x, int y, const char* text, u16 color);

// Arena drawing
void renderArenaFloor();
void renderArenaWalls();

// Color helpers
u16 pillColorToRGB(PillColor c);

// ---- Pixel font / text overlay ----
// Each glyph is 5 wide x 7 tall; call after other render calls so it draws on top.
// color should be an RGB15 value (BIT(15) is added internally).
void renderText(int x, int y, const char* text, u16 color);
int  renderTextWidth(const char* text);   // pixel width of string (for centering)

// Wave-announcement overlay: call once per frame from renderPlayState.
// Manages its own timer; driven by waveAnnouncementStart / waveAnnouncementClear.
void waveAnnouncementStart(int waveNum);
void waveAnnouncementClear();
void waveAnnouncementTick();        // call every frame to decrement timers
void waveAnnouncementRender();      // call after renderGameplay()

void renderTriggerMergeFlash();

// Top-screen detail views (shop phase)
void renderSpriteScaled(int destX, int destY, int classId, int frame, int colorId, int scale);
void renderShopDetail(int selectedCard);
void renderArmySummary();

// Top-screen detail view for an owned companion (inventory bar tap)
void renderCompanionDetail(int companionSlot);

// Top-screen perk info overlay (all owned perks)
void renderPerkInfo();

// Get the active top-screen framebuffer pointer (for enemy sprite blitting)
u16* renderGetActiveBuffer();

// Current render frame counter (for animation timing)
extern int renderFrame;

#endif // RENDER_H
