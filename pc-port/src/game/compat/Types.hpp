#pragma once

#include <cstdint>

using s8 = std::int8_t;
using u8 = std::uint8_t;
using s16 = std::int16_t;
using u16 = std::uint16_t;
using s32 = std::int32_t;
using u32 = std::uint32_t;
using s64 = std::int64_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;
using OSTime = s64;

struct TVec2f {
    f32 x {};
    f32 y {};

    TVec2f() = default;

    TVec2f(f32 x_, f32 y_)
        : x(x_), y(y_) {
    }

    void set(f32 x_, f32 y_) {
        x = x_;
        y = y_;
    }
};
