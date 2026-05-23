// Minimal <nds.h> stub for host-side testing.
// Provides only the typedefs, macros, and constants that the game logic headers need.
#ifndef NDS_STUB_H
#define NDS_STUB_H

#include <cstdint>

// --- Integer typedefs that libnds provides ---
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef volatile uint16_t vuint16;
typedef volatile uint32_t vuint32;

// --- Section attributes (no-ops on host) ---
#define DTCM_BSS
#define DTCM_DATA
#define ITCM_CODE

// --- Utility macros ---
#ifndef BIT
#define BIT(n) (1 << (n))
#endif

#define RGB15(r,g,b) ((r) | ((g)<<5) | ((b)<<10))

// --- Key defines used in player.cpp ---
#define KEY_UP     BIT(6)
#define KEY_DOWN   BIT(7)
#define KEY_LEFT   BIT(5)
#define KEY_RIGHT  BIT(4)
#define KEY_A      BIT(0)
#define KEY_B      BIT(1)

#endif // NDS_STUB_H
