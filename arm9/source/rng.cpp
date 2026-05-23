#include "rng.h"

DTCM_BSS static u32 rngState;

void rngSeed(u32 seed) {
    rngState = seed ? seed : 1;
}

u32 rngNext() {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return rngState;
}

int rngRange(int max) {
    if (max <= 0) return 0;
    return static_cast<int>(rngNext() % static_cast<u32>(max));
}
