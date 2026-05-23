#ifndef TYPES_H
#define TYPES_H

#include <nds.h>

// 12.4 fixed-point math
using Fixed = s16;

constexpr int FP_SHIFT = 4;
constexpr Fixed FP_ONE = 1 << FP_SHIFT;

constexpr Fixed toFixed(int v) { return static_cast<Fixed>(v << FP_SHIFT); }
constexpr int toInt(Fixed v) { return v >> FP_SHIFT; }
constexpr Fixed fixMul(Fixed a, Fixed b) { return static_cast<Fixed>((static_cast<s32>(a) * b) >> FP_SHIFT); }
constexpr Fixed fixDiv(Fixed a, Fixed b) { return static_cast<Fixed>((static_cast<s32>(a) << FP_SHIFT) / b); }

struct Vec2 {
    Fixed x, y;

    Vec2 operator+(Vec2 o) const { return {static_cast<Fixed>(x + o.x), static_cast<Fixed>(y + o.y)}; }
    Vec2 operator-(Vec2 o) const { return {static_cast<Fixed>(x - o.x), static_cast<Fixed>(y - o.y)}; }
    Vec2 operator*(Fixed s) const { return {fixMul(x, s), fixMul(y, s)}; }

    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }

    int pixelX() const { return toInt(x); }
    int pixelY() const { return toInt(y); }
};

// Screen dimensions
constexpr int SCREEN_W = 256;
constexpr int SCREEN_H = 192;

// Pill tiers
enum class PillTier : u8 {
    Normal = 0,
    Super  = 1,
    Ultra  = 2,
    Mega   = 3,
};

// Pill / synergy colors
enum class PillColor : u8 {
    Red    = 0,  // damage
    Blue   = 1,  // speed
    Green  = 2,  // regen
    Yellow = 3,  // fire rate
    COUNT
};

constexpr int PILL_COLOR_COUNT = static_cast<int>(PillColor::COUNT);
constexpr int PILL_TIER_COUNT  = 4;
constexpr int MERGE_THRESHOLD  = 3;

// AABB for collision
struct Rect {
    s16 x, y, w, h;

    bool overlaps(const Rect& o) const {
        return x < o.x + o.w && x + w > o.x &&
               y < o.y + o.h && y + h > o.y;
    }
};

#endif // TYPES_H
