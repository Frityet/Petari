#pragma once

#include <revolution.h>

#include <cmath>

// Host-side subset of JMath's trigonometric surface. Original Game translation
// units use this header for the shared radian constants and lookup-table entry
// points; the PC runtime delegates those operations to the standard library.
inline constexpr f32 HALF_PI = 1.5707964F;
inline constexpr f32 PI = 3.1415927F;
inline constexpr f32 TWO_PI = 6.2831855F;

inline f32 JMACosRadian(f32 angle) {
    return std::cos(angle);
}

inline f32 JMASinRadian(f32 angle) {
    return std::sin(angle);
}

inline f32 JMACosDegree(f32 angle) {
    return std::cos(angle * (PI / 180.0F));
}

inline f32 JMASinDegree(f32 angle) {
    return std::sin(angle * (PI / 180.0F));
}
