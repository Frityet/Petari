#include "JSystem/JGeometry/TVec.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
float value(unsigned int bits) { return std::bit_cast<float>(bits); }
unsigned int bits(float number) { return std::bit_cast<unsigned int>(number); }
void near(float actual, float expected, float tolerance, const char* message) {
    require(std::fabs(actual - expected) <= tolerance, message);
}

void fused_dot_and_original_forwarding() {
    // (1 + 2^-23)(1 - 2^-23) - 1 = -2^-46. Rounding the
    // first product separately would incorrectly erase this result.
    const TVec3f a(value(0x3f800001), 1.0f, 0.0f);
    const TVec3f b(value(0x3f7ffffe), -1.0f, 0.0f);
    require(bits(PSVECDotProduct(&a, &b)) == 0xa8800000, "paired dot lost fused cancellation");
    require(bits(a.dot(b)) == 0xa8800000, "TVec dot did not use original paired accumulation");
#ifdef MTX_USE_PS
    require(bits(VECDotProduct(&a, &b)) == 0xa8800000, "MTX_USE_PS dot selected the C implementation");
#endif
    const TVec3f axis(1.0f, 0.0f, 0.0f);
    require(bits(axis.length()) == 0x3f7fffff, "length skipped the original reciprocal-square-root refinement");
    near(TVec3f(3.0f, 4.0f, 0.0f).length(), 5.0f, 0.000001f, "length changed geometric magnitude");
    require(TVec3f().length() == 0.0f && TVec3f().squared() == 0.0f, "zero magnitude changed");
}

void cross_alias_and_orientation() {
    const TVec3f a(0.0f, value(0x3f800001), 1.0f);
    const TVec3f b(0.0f, 1.0f, value(0x3f7ffffe));
    const auto expected = a.cross(b);
    require(bits(expected.x) == 0xa8800000, "paired cross lost fused cancellation");
    require(bits(expected.y) == 0x80000000 && bits(expected.z) == 0x80000000,
            "paired cross lost original post-subtraction negation");
    auto left = a;
    PSVECCrossProduct(&left, &b, &left);
    auto right = b;
    PSVECCrossProduct(&a, &right, &right);
    for (int axis = 0; axis < 3; ++axis) {
        require(bits((&left.x)[axis]) == bits((&expected.x)[axis]), "cross overwrote aliased left input");
        require(bits((&right.x)[axis]) == bits((&expected.x)[axis]), "cross overwrote aliased right input");
    }
    require(TVec3f(1.0f, 0.0f, 0.0f).orientation(a, b), "orientation discarded negative fused cross");
    require(!TVec3f(-1.0f, 0.0f, 0.0f).orientation(a, b), "orientation reversed original handedness");
    require(!TVec3f().orientation(a, b), "zero reference must not acquire an orientation");
}

void scale_add_order_and_alias() {
    const TVec3f source(value(0x3f800001), 2.0f, 3.0f);
    const TVec3f base(-1.0f, 4.0f, 5.0f);
    const float scale = value(0x3f7ffffe);
    TVec3f result;
    JMAVECScaleAdd(&source, &base, &result, scale);
    require(bits(result.x) == 0xa8800000, "scale/add separated the original fused operation");
    auto left = source;
    JMAVECScaleAdd(&left, &base, &left, scale);
    auto right = base;
    JMAVECScaleAdd(&source, &right, &right, scale);
    require(left.x == result.x && left.y == result.y && left.z == result.z, "scale/add left alias failed");
    require(right.x == result.x && right.y == result.y && right.z == result.z, "scale/add base alias failed");

    // The original compiler schedules all inputs before its stores, even though
    // the root inline-assembly text places the XY store ahead of the Z loads.
    alignas(Vec) std::array<float, 6> storage{1, 2, 3, 4, 5, 6};
    const Vec offset{10, 20, 30};
    JMAVECScaleAdd(reinterpret_cast<const Vec*>(storage.data()), &offset,
                   reinterpret_cast<Vec*>(storage.data() + 1), 2.0f);
    require(storage == std::array<float, 6>{1, 12, 24, 36, 5, 6}, "scale/add lost original retail load/store order");
}

void projection_and_angle() {
    TVec3f source(1.0f, 2.0f, 0.0f);
    source.orthogonalize(TVec3f(2.0f, 0.0f, 0.0f));
    require(source.x == -3.0f && source.y == 2.0f && source.z == 0.0f,
            "orthogonalize normalized the original raw kill direction");
    source.orthogonalize(TVec3f());
    require(source.x == -3.0f && source.y == 2.0f, "zero kill direction changed source");
    TVec3f same(2.0f, 0.0f, 0.0f);
    same.orthogonalize(same);
    require(same.x == -6.0f, "orthogonalize did not support self direction");
    const TVec3f x(1.0f, 0.0f, 0.0f);
    near(x.angle(TVec3f(0.0f, 2.0f, 0.0f)), 1.5707964f, 0.0f, "angle must use original radian table");
    near(x.angle(TVec3f(-3.0f, 0.0f, 0.0f)), 3.1415927f, 0.0f, "antiparallel angle changed");
    require(x.angle(x) == 0.0f && x.angle(TVec3f()) == 0.0f, "parallel/degenerate angle changed");
}

void quantized_store_boundary() {
    const Vec tiny{value(0x00800000), value(0x80800000), value(0x00800000)};
    const Vec zero{};
    Vec result;
    JMAVECScaleAdd(&tiny, &zero, &result, 0.5f);
    require(bits(result.x) == 0 && bits(result.y) == 0x80000000 && bits(result.z) == 0,
            "scale/add did not flush signed subnormal stores");
    const Vec a{0.0f, value(0x00800000), 0.0f};
    const Vec b{0.0f, 0.0f, -0.5f};
    PSVECCrossProduct(&a, &b, &result);
    require(bits(result.x) == 0x80000000, "cross did not flush negative subnormal store");
}
}

int main() {
    try {
        fused_dot_and_original_forwarding(); std::puts("PASS paired dot and original TVec magnitude");
        cross_alias_and_orientation(); std::puts("PASS paired cross aliases and original orientation");
        scale_add_order_and_alias(); std::puts("PASS original scale/add fusion and paired overlap order");
        projection_and_angle(); std::puts("PASS original raw-direction projection and radian angles");
        quantized_store_boundary(); std::puts("PASS original signed quantized-store boundary");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL %s\n", error.what());
        return 1;
    }
}
