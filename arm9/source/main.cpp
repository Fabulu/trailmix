#include <nds.h>
#include <stdio.h>
#include "game.h"
#include "render.h"
#include "strings.h"
#include "save.h"
#include <filesystem.h>
#include "enemy_sprites.h"
#include "entities.h"

// Bouncing enemy sprite for the language select screen
struct BouncySprite {
    s16 x, y, dx, dy;
    u8 type;      // ETYPE_* (0-11, no bosses)
    u8 sizeClass; // 0=small, 1=medium, 2=large (rarely)
    u8 frame;     // animation frame (0 or 1)
    u8 spriteSize; // pixel size (8, 16, or 24)
};

void languageSelect() {
    // Init enemy sprite lookup table (just ROM pointers, no VRAM needed)
    enemySpriteInit();

    // Top screen: framebuffer mode for animated bouncing enemies
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    u16* fb = VRAM_A;

    // Init 8 bouncing enemy sprites — random types, mostly small/medium, rarely large
    constexpr int NUM_BOUNCERS = 8;
    BouncySprite bouncers[NUM_BOUNCERS];

    // Deterministic "random" type selection — pick a variety of regular enemies
    static const u8 typePool[] = {
        ETYPE_GRUNT, ETYPE_CHARGER, ETYPE_SPITTER, ETYPE_BOMBER,
        ETYPE_SWARM_DRONE, ETYPE_GHOST, ETYPE_BRUTE, ETYPE_SPLITTER,
    };
    // Size distribution: mostly small/medium, one large
    static const u8 sizePool[] = { 0, 1, 0, 1, 1, 0, 1, 2 };
    static const u8 pxSizes[]  = { 8, 16, 24 };

    for (int i = 0; i < NUM_BOUNCERS; i++) {
        bouncers[i].type = typePool[i];
        bouncers[i].sizeClass = sizePool[i];
        bouncers[i].spriteSize = pxSizes[sizePool[i]];
        bouncers[i].frame = 0;
        bouncers[i].x = 20 + (i * 37) % 216;
        bouncers[i].y = 15 + (i * 23) % 162;
        bouncers[i].dx = (i % 2 == 0) ? 1 : -1;
        bouncers[i].dy = (i % 3 == 0) ? 1 : -1;
    }

    // Fill screen with dark background
    {
        u16 dark = RGB15(1, 1, 3) | BIT(15);
        for (int i = 0; i < 256 * 192; i++) fb[i] = dark;
    }

    // Set up sub screen console
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    PrintConsole langConsole;
    consoleInit(&langConsole, 0, BgType_Text4bpp, BgSize_T_256x256, 22, 0, false, true);
    consoleSelect(&langConsole);

    int selection = 0;
    int frameCount = 0;

    while (true) {
        consoleClear();
        printf("\n\n\n");
        printf("    === TRAIL MIX ===\n\n");
        printf("    Select Language:\n\n");
        printf("    %s English\n", selection == 0 ? ">" : " ");
        printf("    %s Deutsch\n", selection == 1 ? ">" : " ");
        printf("\n\n    D-Pad to select\n");
        printf("    A to confirm\n");

        swiWaitForVBlank();
        frameCount++;

        u16 dark = RGB15(1, 1, 3) | BIT(15);
        for (int i = 0; i < NUM_BOUNCERS; i++) {
            BouncySprite& b = bouncers[i];
            int sz = b.spriteSize;

            // Erase old position
            for (int py = 0; py < sz; py++) {
                for (int px = 0; px < sz; px++) {
                    int sx = b.x + px, sy = b.y + py;
                    if (sx >= 0 && sx < 256 && sy >= 0 && sy < 192)
                        fb[sy * 256 + sx] = dark;
                }
            }

            // Move
            b.x += b.dx;
            b.y += b.dy;
            if (b.x <= 0 || b.x + sz >= 256) { b.dx = -b.dx; b.x += b.dx * 2; }
            if (b.y <= 0 || b.y + sz >= 192) { b.dy = -b.dy; b.y += b.dy * 2; }
            if (b.x < 0) b.x = 0;
            if (b.y < 0) b.y = 0;

            // Toggle animation frame every 16 frames
            b.frame = (frameCount >> 4) & 1;

            // Draw sprite at new position
            enemySpriteBlitTo(fb, b.type, b.sizeClass, b.frame, b.x, b.y);
        }

        scanKeys();
        u32 down = keysDown();

        if (down & KEY_UP)   selection = 0;
        if (down & KEY_DOWN) selection = 1;
        if (down & KEY_A) {
            gActiveLang = (selection == 0) ? StrLang::EN : StrLang::DE;
            break;
        }
    }
}

int main(int argc, char** argv) {
    (void)argc;
    // Hardware power-on
    powerOn(POWER_ALL_2D);

    // Map screens: top = main engine, bottom = sub engine
    lcdMainOnTop();

    // Store argv globally for NitroFS init (needed by sayingsInit)
    extern char** gArgv;
    gArgv = argv;

    // Language selection before game init
    languageSelect();

    // Initialize game systems (includes sayingsInit + saveInit)
    gameInit();

    // Language is set by languageSelect() — the player's fresh choice takes priority.
    // The save file will store whatever language they picked on next saveWrite().

    // Main loop — VBlank FIRST, then flip buffers during blanking period
    while (true) {
        swiWaitForVBlank();
        renderFlip();    // flip buffers during VBlank — eliminates bus contention
        scanKeys();
        gameUpdate();
        gameRender();    // draws to back buffer (not displayed) — safe anytime
    }

    return 0;
}
