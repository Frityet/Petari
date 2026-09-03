#pragma once

// The original JMath umbrella exposes the SDK math/vector surface used by
// Game translation units. Aurora provides that surface through revolution.h.
#include <revolution.h>

#include <cmath>
#include <cstring>
#include <dolphin/ppc_math.h>

void JMAEulerToQuat(s16, s16, s16, Quaternion*);
void JMAQuatLerp(const Quaternion*, const Quaternion*, f32, Quaternion*);

void JMAMTXApplyScale(const Mtx, Mtx, f32, f32, f32);

template < typename T >
inline T JMAMax(T param_0, T param_1) {
    T ret;
    if (param_0 > param_1) {
        ret = param_0;
    } else {
        ret = param_1;
    }
    return ret;
}

inline f32 JMAFastSqrt(f32 input) {
    return input > 0.0f ? static_cast< f32 >(frsqrte(static_cast< double >(input)) * input) : input;
}

// Scalar form of the original JMA instruction order, including its fused
// multiply/add operations and final negated multiply/subtract.
inline f32 JMAHermiteInterpolation(f32 p1, f32 p2, f32 p3, f32 p4, f32 p5, f32 p6,
                                   f32 p7) {
    f32 ff31 = p1 - p2;
    f32 ff30 = p5 - p2;
    f32 ff29 = ff31 / ff30;
    f32 ff28 = ff29 * ff29;
    f32 ff25 = ff29 + ff29;
    f32 ff27 = ff28 - ff29;
    ff30 = p3 - p6;
    f32 ff26 = std::fma(ff25, ff27, -ff28);
    ff25 = std::fma(p4, ff27, p4);
    ff26 = std::fma(ff26, ff30, p3);
    ff25 = std::fma(p7, ff27, ff25);
    ff25 = std::fma(ff29, p4, -ff25);
    return -std::fma(ff31, ff25, -ff26);
}

namespace JMath {
    [[nodiscard]] f32 fastReciprocal(f32 value);

    template <typename T>
    [[nodiscard]] inline T fastSqrt(T value) {
        return JMAFastSqrt(value);
    }

    inline void gekko_ps_copy12(void* destination, const void* source) {
        std::memcpy(destination, source, 12U * sizeof(f32));
    }

    inline void gekko_ps_copy16(void* destination, const void* source) {
        std::memcpy(destination, source, 16U * sizeof(f32));
    }
}  // namespace JMath

namespace JMathInlineVEC {
    inline void PSVECCopy(const Vec* source, Vec* destination) {
        *destination = *source;
    }

    inline void PSVECAdd(const Vec* lhs, const Vec* rhs, Vec* destination) {
        ::PSVECAdd(lhs, rhs, destination);
    }

    inline void PSVECSubtract(const Vec* lhs, const Vec* rhs, Vec* destination) {
        ::PSVECSubtract(lhs, rhs, destination);
    }

    inline f32 PSVECDotProduct(const Vec* lhs, const Vec* rhs) {
        return ::PSVECDotProduct(lhs, rhs);
    }

    inline f32 PSVECSquareMag(const Vec* source) {
        return ::PSVECSquareMag(source);
    }

    inline void PSVECNegate(const Vec* source, Vec* destination) {
        destination->x = -source->x;
        destination->y = -source->y;
        destination->z = -source->z;
    }

    inline f32 PSVECSquareDistance(const Vec* lhs, const Vec* rhs) {
        return ::PSVECSquareDistance(lhs, rhs);
    }

    inline void PSVECMultiply(const Vec* lhs, const Vec* rhs, Vec* destination) {
        destination->x = lhs->x * rhs->x;
        destination->y = lhs->y * rhs->y;
        destination->z = lhs->z * rhs->z;
    }
}  // namespace JMathInlineVEC
