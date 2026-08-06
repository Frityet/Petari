#pragma once

#include "JSystem/JGeometry/TUtil.hpp"

#include <cmath>
#include <type_traits>

#include <revolution/types.h>

namespace JGeometry {
    template <typename T>
    struct TVec3 {
        T x{};
        T y{};
        T z{};

        constexpr TVec3() = default;

        constexpr TVec3(T newX, T newY, T newZ) : x(newX), y(newY), z(newZ) {
        }

        template <typename U>
        constexpr TVec3(U newX, U newY, U newZ)
            : x(static_cast<T>(newX)), y(static_cast<T>(newY)), z(static_cast<T>(newZ)) {
        }

        template <typename U>
        void set(U newX, U newY, U newZ) {
            x = static_cast<T>(newX);
            y = static_cast<T>(newY);
            z = static_cast<T>(newZ);
        }

        template <typename U>
        void set(const TVec3<U> &value) {
            x = static_cast<T>(value.x);
            y = static_cast<T>(value.y);
            z = static_cast<T>(value.z);
        }
    };

    template <>
    struct TVec3<f32> : public Vec {
        constexpr TVec3() : Vec{0.0F, 0.0F, 0.0F} {
        }

        constexpr TVec3(const Vec &value) : Vec{value.x, value.y, value.z} {
        }

        constexpr TVec3(f32 newX, f32 newY, f32 newZ) : Vec{newX, newY, newZ} {
        }

        template <typename T>
        constexpr TVec3(T newX, T newY, T newZ)
            : Vec{static_cast<f32>(newX), static_cast<f32>(newY), static_cast<f32>(newZ)} {
        }

        constexpr TVec3(f32 value) : Vec{value, value, value} {
        }

        operator Vec *() {
            return this;
        }

        operator const Vec *() const {
            return this;
        }

        template <typename T>
        void set(T newX, T newY, T newZ) {
            x = static_cast<f32>(newX);
            y = static_cast<f32>(newY);
            z = static_cast<f32>(newZ);
        }

        template <typename T>
        void set(const TVec3<T> &value) {
            x = static_cast<f32>(value.x);
            y = static_cast<f32>(value.y);
            z = static_cast<f32>(value.z);
        }

        void set(const Vec &value) {
            x = value.x;
            y = value.y;
            z = value.z;
        }

        void set(f32 value) {
            x = value;
            y = value;
            z = value;
        }

        void add(const TVec3 &value) {
            x += value.x;
            y += value.y;
            z += value.z;
        }

        void add(const TVec3 &lhs, const TVec3 &rhs) {
            const f32 newX = lhs.x + rhs.x;
            const f32 newY = lhs.y + rhs.y;
            const f32 newZ = lhs.z + rhs.z;
            set(newX, newY, newZ);
        }

        void sub(const TVec3 &value) {
            x -= value.x;
            y -= value.y;
            z -= value.z;
        }

        void sub(const TVec3 &lhs, const TVec3 &rhs) {
            const f32 newX = lhs.x - rhs.x;
            const f32 newY = lhs.y - rhs.y;
            const f32 newZ = lhs.z - rhs.z;
            set(newX, newY, newZ);
        }

        void scale(f32 value) {
            x *= value;
            y *= value;
            z *= value;
        }

        void scale(f32 value, const TVec3 &source) {
            const f32 newX = value * source.x;
            const f32 newY = value * source.y;
            const f32 newZ = value * source.z;
            set(newX, newY, newZ);
        }

        void scaleAdd(f32 scaleValue, const TVec3 &scaled, const TVec3 &base) {
            const f32 newX = scaleValue * scaled.x + base.x;
            const f32 newY = scaleValue * scaled.y + base.y;
            const f32 newZ = scaleValue * scaled.z + base.z;
            set(newX, newY, newZ);
        }

        [[nodiscard]] constexpr f32 dot(const TVec3 &value) const {
            return x * value.x + y * value.y + z * value.z;
        }

        [[nodiscard]] constexpr f32 squared() const {
            return dot(*this);
        }

        [[nodiscard]] constexpr f32 squared(const TVec3 &value) const {
            const f32 dx = x - value.x;
            const f32 dy = y - value.y;
            const f32 dz = z - value.z;
            return dx * dx + dy * dy + dz * dz;
        }

        [[nodiscard]] constexpr f32 squareDistance(const TVec3 &value) const {
            return squared(value);
        }

        [[nodiscard]] f32 length() const {
            return std::sqrt(squared());
        }

        [[nodiscard]] f32 distance(const TVec3 &value) const {
            return std::sqrt(squared(value));
        }

        [[nodiscard]] constexpr bool epsilonEquals(const TVec3 &value, f32 epsilon) const {
            return TUtil<f32>::epsilonEquals(x, value.x, epsilon) && TUtil<f32>::epsilonEquals(y, value.y, epsilon) &&
                   TUtil<f32>::epsilonEquals(z, value.z, epsilon);
        }

        f32 normalize() {
            const f32 magnitude = length();
            if (magnitude > TUtil<f32>::epsilon()) {
                scale(1.0F / magnitude);
            }

            return magnitude;
        }

        f32 normalize(const TVec3 &source) {
            set(source);
            return normalize();
        }

        f32 setLength(f32 newLength) {
            const f32 oldSquaredLength = squared();
            if (oldSquaredLength <= TUtil<f32>::epsilon()) {
                return 0.0F;
            }

            const f32 inverseLength = TUtil<f32>::inv_sqrt(oldSquaredLength);
            scale(inverseLength * newLength);
            return inverseLength * oldSquaredLength;
        }

        void cross(const TVec3 &lhs, const TVec3 &rhs) {
            const f32 newX = lhs.y * rhs.z - lhs.z * rhs.y;
            const f32 newY = lhs.z * rhs.x - lhs.x * rhs.z;
            const f32 newZ = lhs.x * rhs.y - lhs.y * rhs.x;
            set(newX, newY, newZ);
        }

        [[nodiscard]] TVec3 cross(const TVec3 &value) const {
            TVec3 result;
            result.cross(*this, value);
            return result;
        }

        void negate() {
            x = -x;
            y = -y;
            z = -z;
        }

        void negate(const TVec3 &value) {
            const f32 newX = -value.x;
            const f32 newY = -value.y;
            const f32 newZ = -value.z;
            set(newX, newY, newZ);
        }

        void killElement(const TVec3 &value, const TVec3 &killDirection) {
            scaleAdd(-killDirection.dot(value), killDirection, value);
        }

        [[nodiscard]] TVec3 killElement(const TVec3 &killDirection) const {
            TVec3 result;
            result.killElement(*this, killDirection);
            return result;
        }

        void zero() {
            set(0.0F, 0.0F, 0.0F);
        }

        [[nodiscard]] constexpr TVec3 operator+(const TVec3 &value) const {
            return TVec3{x + value.x, y + value.y, z + value.z};
        }

        [[nodiscard]] constexpr TVec3 operator-(const TVec3 &value) const {
            return TVec3{x - value.x, y - value.y, z - value.z};
        }

        [[nodiscard]] constexpr TVec3 operator-() const {
            return TVec3{-x, -y, -z};
        }

        [[nodiscard]] constexpr TVec3 operator*(f32 scaleValue) const {
            return TVec3{x * scaleValue, y * scaleValue, z * scaleValue};
        }

        [[nodiscard]] constexpr TVec3 operator/(f32 divisor) const {
            return *this * (1.0F / divisor);
        }

        void operator+=(const TVec3 &value) {
            add(value);
        }

        void operator-=(const TVec3 &value) {
            sub(value);
        }

        void operator*=(f32 scaleValue) {
            scale(scaleValue);
        }

        void operator/=(f32 divisor) {
            scale(1.0F / divisor);
        }
    };

    static_assert(sizeof(TVec3<f32>) == sizeof(Vec));
    static_assert(alignof(TVec3<f32>) == alignof(Vec));
    static_assert(std::is_standard_layout_v<TVec3<f32>>);
    static_assert(std::is_trivially_copyable_v<TVec3<f32>>);
}  // namespace JGeometry

using TVec3f = JGeometry::TVec3<f32>;

[[nodiscard]] constexpr TVec3f operator*(f32 scaleValue, const TVec3f &value) {
    return value * scaleValue;
}
