#pragma once

// The original JMath umbrella exposes the SDK math/vector surface used by
// Game translation units. Aurora provides that surface through revolution.h.
#include <revolution.h>

#include <cmath>
#include <cstring>

inline f32 JMAFastSqrt(f32 input) {
    return input > 0.0F ? std::sqrt(input) : input;
}

namespace JMath {
    [[nodiscard]] inline f32 fastReciprocal(f32 value) {
        return 1.0F / value;
    }

    template <typename T>
    [[nodiscard]] inline T fastSqrt(T value) {
        return value > static_cast<T>(0) ? static_cast<T>(std::sqrt(value)) : value;
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
