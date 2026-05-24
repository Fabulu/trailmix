#include <nds.h>
#include <stdio.h>
#include "game.h"
#include "render.h"
#include "strings.h"

// Simple bouncing shape for the language select screen
struct BouncyEnemy {
    s16 x, y, dx, dy, size;
    u16 color;
};

void languageSelect() {
    // Top screen: framebuffer mode for animated bouncing enemies
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    u16* fb = VRAM_A;

    // Init 8 bouncing geometric shapes (fewer = less drawing = no tearing)
    constexpr int NUM_BOUNCERS = 8;
    BouncyEnemy bouncers[NUM_BOUNCERS];
    static const u16 colors[] = {
        RGB15(31,5,5), RGB15(5,10,31), RGB15(5,28,5), RGB15(31,28,0),
        RGB15(20,0,31), RGB15(0,28,28), RGB15(31,10,10), RGB15(10,20,31),
    };
    for (int i = 0; i < NUM_BOUNCERS; i++) {
        bouncers[i].x = 20 + (i * 37) % 216;
        bouncers[i].y = 15 + (i * 23) % 162;
        bouncers[i].dx = (i % 2 == 0) ? 1 : -1;
        bouncers[i].dy = (i % 3 == 0) ? 1 : -1;
        bouncers[i].size = 8 + (i % 3) * 4; // 8, 12, 16
        bouncers[i].color = colors[i] | BIT(15);
    }

    // Fill screen once with dark background before entering loop
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

    while (true) {
        consoleClear();
        printf("\n\n\n");
        printf("    === PILL ARMY ===\n\n");
        printf("    Select Language:\n\n");
        printf("    %s English\n", selection == 0 ? ">" : " ");
        printf("    %s Deutsch\n", selection == 1 ? ">" : " ");
        printf("\n\n    D-Pad to select\n");
        printf("    A to confirm\n");

        swiWaitForVBlank();

        // Erase each bouncer at old position, move, draw at new position
        u16 dark = RGB15(1, 1, 3) | BIT(15);
        for (int i = 0; i < NUM_BOUNCERS; i++) {
            BouncyEnemy& b = bouncers[i];

            // Erase old position (fill with dark)
            for (int py = 0; py < b.size; py++) {
                for (int px = 0; px < b.size; px++) {
                    int sx = b.x + px, sy = b.y + py;
                    if (sx >= 0 && sx < 256 && sy >= 0 && sy < 192)
                        fb[sy * 256 + sx] = dark;
                }
            }

            // Move
            b.x += b.dx;
            b.y += b.dy;
            if (b.x <= 0 || b.x + b.size >= 256) { b.dx = -b.dx; b.x += b.dx * 2; }
            if (b.y <= 0 || b.y + b.size >= 192) { b.dy = -b.dy; b.y += b.dy * 2; }
            if (b.x < 0) b.x = 0;
            if (b.y < 0) b.y = 0;

            // Draw at new position
            for (int py = 0; py < b.size; py++) {
                for (int px = 0; px < b.size; px++) {
                    bool draw;
                    if (i % 2 == 0) {
                        draw = true;
                    } else {
                        int half = b.size / 2;
                        int cx = px - half, cy = py - half;
                        draw = ((cx < 0 ? -cx : cx) + (cy < 0 ? -cy : cy)) <= half;
                    }
                    if (draw) {
                        int sx = b.x + px, sy = b.y + py;
                        if (sx >= 0 && sx < 256 && sy >= 0 && sy < 192)
                            fb[sy * 256 + sx] = b.color;
                    }
                }
            }
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

int main() {
    // Hardware power-on
    powerOn(POWER_ALL_2D);

    // Map screens: top = main engine, bottom = sub engine
    lcdMainOnTop();

    // Language selection before game init
    languageSelect();

    // Initialize game systems
    gameInit();

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
