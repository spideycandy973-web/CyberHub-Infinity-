#pragma once
#include <cstdint>

namespace neon {

inline uint64_t mix64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

inline uint64_t hashCombine(uint64_t seed, uint64_t value) {
    return mix64(seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2)));
}

inline uint64_t hashChunk(uint64_t seed, int cx, int cz) {
    uint64_t h = seed;
    h = hashCombine(h, static_cast<uint64_t>(cx));
    h = hashCombine(h, static_cast<uint64_t>(cz));
    return h;
}

class Pcg32 {
public:
    explicit Pcg32(uint64_t seed = 42) {
        mState = 0;
        mInc = (seed << 1u) | 1u;
        next();
        mState += seed;
        next();
    }

    uint32_t next() {
        uint64_t old = mState;
        mState = old * 6364136223846793005ULL + mInc;
        uint32_t xorshifted = static_cast<uint32_t>(((old >> 18u) ^ old) >> 27u);
        uint32_t rot = static_cast<uint32_t>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31u));
    }

    float nextFloat() {
        return static_cast<float>(next() >> 8) * (1.0f / 16777216.0f);
    }

    uint32_t nextBounded(uint32_t bound) {
        if (bound <= 1) return 0;
        uint32_t threshold = -bound % bound;
        for (;;) {
            uint32_t r = next();
            if (r >= threshold) return r % bound;
        }
    }

private:
    uint64_t mState;
    uint64_t mInc;
};

} // namespace neon
