#pragma once

#include <cfloat>
#include <cmath>

#include <revolution/types.h>

namespace JGeometry {
    template <typename T>
    class TUtil {
    public:
        [[nodiscard]] static constexpr bool epsilonEquals(T lhs, T rhs, T epsilon) {
            const T difference = lhs - rhs;
            return difference >= -epsilon && difference <= epsilon;
        }

        [[nodiscard]] static T sqrt(T value) {
            return value <= static_cast<T>(0) ? value : static_cast<T>(std::sqrt(value));
        }

        [[nodiscard]] static T inv_sqrt(T value) {
            return value <= static_cast<T>(0) ? value : static_cast<T>(1) / static_cast<T>(std::sqrt(value));
        }

        [[nodiscard]] static T acos(T value) {
            return static_cast<T>(std::acos(clamp(value, static_cast<T>(-1), static_cast<T>(1))));
        }

        [[nodiscard]] static constexpr T clamp(T value, T min, T max) {
            if (value < min) {
                return min;
            }

            if (value > max) {
                return max;
            }

            return value;
        }

        [[nodiscard]] static constexpr T epsilon() {
            return static_cast<T>(32) * static_cast<T>(FLT_EPSILON);
        }
    };
}  // namespace JGeometry
