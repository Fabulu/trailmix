// Tests for Vec2 operations, fixed-point math, and Rect::overlaps.
// nds/nds.h on the include path redirects <nds.h> to our stub.
#include "../arm9/source/types.h"
#include "test_framework.h"

// --- Fixed-point math ---

TEST(toFixed_basic) {
    ASSERT_EQ(toFixed(1), FP_ONE);
    ASSERT_EQ(toFixed(0), 0);
    ASSERT_EQ(toFixed(3), 3 << FP_SHIFT);
}

TEST(toInt_rounds_down) {
    ASSERT_EQ(toInt(toFixed(5)), 5);
    // sub-pixel value rounds toward zero
    ASSERT_EQ(toInt(toFixed(3) + 1), 3);
}

TEST(fixMul_basic) {
    Fixed a = toFixed(3);
    Fixed b = toFixed(4);
    ASSERT_EQ(toInt(fixMul(a, b)), 12);
}

TEST(fixMul_fractional) {
    // 1.5 * 2 = 3
    Fixed half = FP_ONE + (FP_ONE / 2);  // 1.5 in fixed
    Fixed two  = toFixed(2);
    ASSERT_EQ(toInt(fixMul(half, two)), 3);
}

TEST(fixDiv_basic) {
    Fixed a = toFixed(12);
    Fixed b = toFixed(4);
    ASSERT_EQ(toInt(fixDiv(a, b)), 3);
}

// --- Vec2 ---

TEST(Vec2_add) {
    Vec2 a = {toFixed(1), toFixed(2)};
    Vec2 b = {toFixed(3), toFixed(4)};
    Vec2 c = a + b;
    ASSERT_EQ(toInt(c.x), 4);
    ASSERT_EQ(toInt(c.y), 6);
}

TEST(Vec2_sub) {
    Vec2 a = {toFixed(5), toFixed(7)};
    Vec2 b = {toFixed(2), toFixed(3)};
    Vec2 c = a - b;
    ASSERT_EQ(toInt(c.x), 3);
    ASSERT_EQ(toInt(c.y), 4);
}

TEST(Vec2_scale) {
    Vec2 a = {toFixed(3), toFixed(4)};
    Vec2 c = a * toFixed(2);
    ASSERT_EQ(toInt(c.x), 6);
    ASSERT_EQ(toInt(c.y), 8);
}

TEST(Vec2_pluseq) {
    Vec2 a = {toFixed(1), toFixed(1)};
    Vec2 b = {toFixed(2), toFixed(3)};
    a += b;
    ASSERT_EQ(toInt(a.x), 3);
    ASSERT_EQ(toInt(a.y), 4);
}

TEST(Vec2_pixel) {
    Vec2 a = {toFixed(10), toFixed(20)};
    ASSERT_EQ(a.pixelX(), 10);
    ASSERT_EQ(a.pixelY(), 20);
}

// --- Rect::overlaps ---

TEST(Rect_overlaps_true) {
    Rect a = {0, 0, 10, 10};
    Rect b = {5, 5, 10, 10};
    ASSERT(a.overlaps(b));
    ASSERT(b.overlaps(a));
}

TEST(Rect_overlaps_false_adjacent) {
    Rect a = {0, 0, 10, 10};
    Rect b = {10, 0, 10, 10};  // exactly touching edge — no overlap
    ASSERT(!a.overlaps(b));
}

TEST(Rect_overlaps_false_apart) {
    Rect a = {0, 0, 5, 5};
    Rect b = {100, 100, 5, 5};
    ASSERT(!a.overlaps(b));
}

TEST(Rect_overlaps_contained) {
    Rect big   = {0, 0, 100, 100};
    Rect small = {10, 10, 5, 5};
    ASSERT(big.overlaps(small));
    ASSERT(small.overlaps(big));
}

// --- main ---

int main() {
    std::cout << "=== test_types ===\n";
    return runAllTests();
}
