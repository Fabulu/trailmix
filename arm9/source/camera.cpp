#include "camera.h"
#include "rng.h"

static Vec2 gCamera;
static u8 shakeIntensity;
static u8 shakeFrames;
static int shakeOffX;
static int shakeOffY;

static Fixed clampFixed(Fixed v, Fixed lo, Fixed hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void cameraInit() {
    gCamera = {0, 0};
    shakeIntensity = 0;
    shakeFrames = 0;
    shakeOffX = 0;
    shakeOffY = 0;
}

void cameraUpdate(Vec2 target) {
    gCamera.x = clampFixed(
        static_cast<Fixed>(target.x - toFixed(SCREEN_W / 2)),
        toFixed(0),
        toFixed(ARENA_W - SCREEN_W)
    );
    gCamera.y = clampFixed(
        static_cast<Fixed>(target.y - toFixed(SCREEN_H / 2)),
        toFixed(0),
        toFixed(ARENA_H - SCREEN_H)
    );

    // Update shake
    if (shakeFrames > 0) {
        int range = shakeIntensity * 2 + 1;
        shakeOffX = rngRange(range) - shakeIntensity;
        shakeOffY = rngRange(range) - shakeIntensity;
        --shakeFrames;
    } else {
        shakeOffX = 0;
        shakeOffY = 0;
    }
}

int camX() {
    return toInt(gCamera.x) + shakeOffX;
}

int camY() {
    return toInt(gCamera.y) + shakeOffY;
}

void cameraShake(u8 intensity, u8 frames) {
    shakeIntensity = intensity;
    shakeFrames = frames;
}
