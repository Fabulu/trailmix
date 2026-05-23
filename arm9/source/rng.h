#ifndef RNG_H
#define RNG_H

#include <nds.h>

void rngSeed(u32 seed);
u32  rngNext();
// Returns random int in [0, max)
int  rngRange(int max);

#endif // RNG_H
