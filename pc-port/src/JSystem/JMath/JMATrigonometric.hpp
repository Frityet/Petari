#pragma once

#include <math_types.hpp>
#include <revolution.h>

#include <algorithm>
#include <cmath>

// Host-side subset of JMath's trigonometric surface. Original Game translation
// units use this header for the shared radian constants and lookup-table entry
// points; the PC runtime delegates those operations to the standard library.
inline f32 JMACosRadian(f32 angle) {
    return std::cos(angle);
}

inline f32 JMASinRadian(f32 angle) {
    return std::sin(angle);
}

inline f32 JMACosShort(s16 angle) {
    constexpr auto cTableLength = 16384U;
    const auto tableIndex = static_cast<u16>(angle) >> 2U;
    return std::cos(static_cast<f32>(tableIndex) * (TWO_PI / static_cast<f32>(cTableLength)));
}

inline f32 JMASinShort(s16 angle) {
    constexpr auto cTableLength = 16384U;
    const auto tableIndex = static_cast<u16>(angle) >> 2U;
    return std::sin(static_cast<f32>(tableIndex) * (TWO_PI / static_cast<f32>(cTableLength)));
}

inline f32 JMASCos(s16 angle) {
    return JMACosShort(angle);
}

inline f32 JMASSin(s16 angle) {
    return JMASinShort(angle);
}

inline f32 JMACosDegree(f32 angle) {
    return std::cos(angle * (PI / 180.0F));
}

inline f32 JMASinDegree(f32 angle) {
    return std::sin(angle * (PI / 180.0F));
}

inline f32 JMAAcosRadian(f32 ratio) {
    return std::acos(std::clamp(ratio, -1.0F, 1.0F));
}

inline f32 JMAAsinRadian(f32 ratio) {
    return std::asin(std::clamp(ratio, -1.0F, 1.0F));
}

inline f32 JMAATan2(f32 y, f32 x) {
    return std::atan2(y, x);
}
