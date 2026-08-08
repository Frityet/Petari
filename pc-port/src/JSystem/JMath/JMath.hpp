#pragma once

// The original JMath umbrella exposes the SDK math/vector surface used by
// Game translation units. Aurora provides that surface through revolution.h.
#include <revolution.h>

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
