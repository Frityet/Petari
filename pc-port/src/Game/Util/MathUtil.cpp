#include "Game/Util/MathUtil.hpp"

#include <bit>
#include <cstdint>

namespace {
    auto sRandomSeed = std::uint32_t{0U};
}  // namespace

namespace MR {
    f32 getRandom() {
        sRandomSeed = (sRandomSeed * 0x0019660DU) + 0x3C6EF35FU;
        const auto value = (sRandomSeed >> 9U) | 0x3F800000U;
        return std::bit_cast< f32 >(value) - 1.0F;
    }

    f32 getRandom(f32 min, f32 max) {
        return min + ((max - min) * getRandom());
    }

    s32 getRandom(s32 min, s32 max) {
        return static_cast< s32 >(getRandom(static_cast< f32 >(min), static_cast< f32 >(max)));
    }

    f32 getRandomDegree() {
        return getRandom(0.0F, 360.0F);
    }
}  // namespace MR
