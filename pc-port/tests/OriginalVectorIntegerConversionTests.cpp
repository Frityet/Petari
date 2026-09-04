#include "JSystem/JGeometry/TVec.hpp"
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

constexpr TVec3s wrapped(32768.0f, -32769.0f, 65537.5f);
static_assert(wrapped.x == -32768 && wrapped.y == 32767 && wrapped.z == 1);
constexpr JGeometry::TVec2<u16> unsignedWrapped(-1.9f, 65536.0f);
static_assert(unsignedWrapped.x == 65535 && unsignedWrapped.y == 0);

// Independent arithmetic model of the verified fctiwz/sth/stw instruction
// sequence. fmod expresses low-halfword retention without native short casts.
static s32 word(double x) {
    const double truncated = std::trunc(x);
    if (!(truncated >= -2147483648.0)) return std::numeric_limits<s32>::min();
    if (truncated > 2147483647.0) return std::numeric_limits<s32>::max();
    return static_cast<s32>(truncated);
}
static u16 half(double x) {
    double remainder = std::fmod(static_cast<double>(word(x)), 65536.0);
    if (remainder < 0) remainder += 65536;
    return static_cast<u16>(remainder);
}
static s16 signedHalf(double x) {
    const auto low = half(x);
    return static_cast<s16>(low < 32768 ? low : static_cast<s32>(low) - 65536);
}
template <typename F> static void check(F x) {
    const TVec3s v(x, x, x);
    assert(v.x == signedHalf(x) && v.y == v.x && v.z == v.x);
    TVec3s set; set.set(x, x, x); assert(set.x == v.x && set.y == v.y && set.z == v.z);
    set.set(JGeometry::TVec3<F>(x, x, x)); assert(set.x == v.x && set.y == v.y && set.z == v.z);
    JGeometry::TVec2<u16> u(x, x); assert(u.x == half(x) && u.y == u.x);
    u.set(x, x); assert(u.x == half(x) && u.y == u.x);
    const JGeometry::TVec3<s32> w(x, x, x); assert(w.x == word(x) && w.y == w.x && w.z == w.x);
}
int main() {
    std::size_t cases = 0;
    for (double x : {-std::numeric_limits<double>::infinity(), -2147483649.0, -2147483648.0,
                     -65537.75, -65536.0, -32769.0, -32768.9, -1.9, -0.0, 0.0, 1.9,
                     32767.9, 32768.0, 65535.9, 65536.0, 2147483647.9, 2147483648.0,
                     std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        check(x); check(static_cast<float>(x)); cases += 2;
    }
    // Rotations traverse both sides of every signed-halfword boundary.
    for (int angle = -72000; angle <= 72000; ++angle) {
        check((angle / 100.0f) * 182.04445f); ++cases;
    }
    std::uint32_t bits = 0x12345678;
    for (unsigned i = 0; i < 65536; ++i) {
        bits = bits * 1664525u + 1013904223u;
        check(std::bit_cast<float>(bits)); ++cases;
    }
    JGeometry::TVec2<s16> arithmetic(20000, -20000);
    const auto scaled = arithmetic * 2.0f;
    arithmetic.scale(2.0f);
    assert(scaled.x == -25536 && scaled.y == 25536 && arithmetic.x == scaled.x && arithmetic.y == scaled.y);
    std::cout << "Original integral vector conversion cases=" << cases << " constexpr_wrap=pass arithmetic_wrap=pass\n";
}
