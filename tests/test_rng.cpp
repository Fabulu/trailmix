// Tests for RNG determinism and range bounds.
#include "../arm9/source/rng.h"
#include "../arm9/source/rng.cpp"   // pull in implementation directly
#include "test_framework.h"

TEST(rng_deterministic) {
    rngSeed(42);
    u32 a1 = rngNext();
    u32 a2 = rngNext();
    u32 a3 = rngNext();

    rngSeed(42);
    ASSERT_EQ(rngNext(), a1);
    ASSERT_EQ(rngNext(), a2);
    ASSERT_EQ(rngNext(), a3);
}

TEST(rng_different_seeds_differ) {
    rngSeed(1);
    u32 a = rngNext();
    rngSeed(2);
    u32 b = rngNext();
    ASSERT(a != b);
}

TEST(rng_seed_zero_becomes_one) {
    // Seed 0 should be treated as 1 to avoid degenerate state
    rngSeed(0);
    u32 a = rngNext();
    rngSeed(1);
    u32 b = rngNext();
    ASSERT_EQ(a, b);
}

TEST(rngRange_bounds) {
    rngSeed(12345);
    for (int i = 0; i < 1000; ++i) {
        int v = rngRange(10);
        ASSERT(v >= 0);
        ASSERT(v < 10);
    }
}

TEST(rngRange_max_one) {
    rngSeed(99);
    for (int i = 0; i < 100; ++i) {
        ASSERT_EQ(rngRange(1), 0);
    }
}

TEST(rngRange_zero_returns_zero) {
    rngSeed(1);
    ASSERT_EQ(rngRange(0), 0);
    ASSERT_EQ(rngRange(-5), 0);
}

int main() {
    std::cout << "=== test_rng ===\n";
    return runAllTests();
}
