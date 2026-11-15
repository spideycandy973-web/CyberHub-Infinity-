#include <gtest/gtest.h>
#include "core/Pcg32.h"

TEST(Pcg32, Seed42FirstFour) {
    neon::Pcg32 rng(42);
    EXPECT_EQ(rng.next(), 0x40b29785u);
    EXPECT_EQ(rng.next(), 0x0a8b3706u);
    EXPECT_EQ(rng.next(), 0x2f091207u);
    EXPECT_EQ(rng.next(), 0x646baef9u);
}

TEST(Pcg32, Seed54FirstTwo) {
    neon::Pcg32 rng(54);
    EXPECT_EQ(rng.next(), 0x170b9df7u);
    EXPECT_EQ(rng.next(), 0x7c325805u);
}

TEST(Pcg32, NextFloatRange) {
    neon::Pcg32 rng(99);
    for (int i = 0; i < 1000; ++i) {
        float f = rng.nextFloat();
        EXPECT_GE(f, 0.0f);
        EXPECT_LT(f, 1.0f);
    }
}

TEST(Pcg32, NextBoundedRange) {
    neon::Pcg32 rng(7);
    for (int i = 0; i < 1000; ++i) {
        uint32_t v = rng.nextBounded(10);
        EXPECT_LT(v, 10u);
    }
}

TEST(Pcg32, NextBoundedZero) {
    neon::Pcg32 rng(1);
    EXPECT_EQ(rng.nextBounded(0), 0u);
    EXPECT_EQ(rng.nextBounded(1), 0u);
}

TEST(HashChunk, Deterministic) {
    uint64_t a = neon::hashChunk(12345, 10, -20);
    uint64_t b = neon::hashChunk(12345, 10, -20);
    EXPECT_EQ(a, b);
}

TEST(HashChunk, DifferentCoords) {
    uint64_t a = neon::hashChunk(12345, 0, 0);
    uint64_t b = neon::hashChunk(12345, 1, 0);
    uint64_t c = neon::hashChunk(12345, 0, 1);
    EXPECT_NE(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(b, c);
}
