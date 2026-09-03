#pragma once

#include "JSystem/JGeometry/TUtil.hpp"
#include "JSystem/JMath/JMath.hpp"

#include <cmath>
#include <type_traits>

#include <revolution/types.h>

namespace JGeometry {
    template <typename T>
    struct TVec2 {
        constexpr TVec2() = default;

        constexpr TVec2(T newX, T newY) : x(newX), y(newY) {
        }

        template <typename U>
        constexpr TVec2(U newX, U newY)
            : x(static_cast<T>(newX)), y(static_cast<T>(newY)) {
        }

        template <typename U>
        void set(U newX, U newY) {
            x = static_cast<T>(newX);
            y = static_cast<T>(newY);
        }

        template <typename U>
        void set(const TVec2<U> &value) {
            x = static_cast<T>(value.x);
            y = static_cast<T>(value.y);
        }

        void set(T value) {
            x = value;
            y = value;
        }

        void setMin(const TVec2< f32 >& min) {
            if (x >= min.x)
                x = min.x;
            if (y >= min.y)
                y = min.y;
        }

        void setMax(const TVec2< f32 >& max) {
            if (x <= max.x)
                x = max.x;
            if (y <= max.y)
                y = max.y;
        }

        inline bool isAbove(const TVec2< T >& other) const {
            return (x >= other.x) && (y >= other.y) ? true : false;
        }

        void add(const TVec2& value) {
            x += value.x;
            y += value.y;
        }

        void sub(const TVec2& value) {
            x -= value.x;
            y -= value.y;
        }

        void sub(const TVec2& lhs, const TVec2& rhs) {
            x = lhs.x - rhs.x;
            y = lhs.y - rhs.y;
        }

        void scale(f32 factor) {
            x = static_cast<T>(x * factor);
            y = static_cast<T>(y * factor);
        }

        void zero() {
            set(static_cast<T>(0));
        }

        [[nodiscard]] T squared() const {
            return x * x + y * y;
        }

        [[nodiscard]] T length() const {
            return TUtil<T>::sqrt(squared());
        }

        [[nodiscard]] T squareDist(const TVec2& value) const {
            const auto dx = x - value.x;
            const auto dy = y - value.y;
            return dx * dx + dy * dy;
        }

        [[nodiscard]] T dot(const TVec2& value) const {
            return x * value.x + y * value.y;
        }

        [[nodiscard]] T distance(const TVec2& value) const {
            return TUtil<T>::sqrt(squareDist(value));
        }

        void operator+=(const TVec2& value) {
            x += value.x;
            y += value.y;
        }

        [[nodiscard]] TVec2 operator+(const TVec2& value) const {
            return TVec2{x + value.x, y + value.y};
        }

        [[nodiscard]] TVec2 operator-(const TVec2& value) const {
            return TVec2{x - value.x, y - value.y};
        }

        [[nodiscard]] TVec2 operator*(f32 factor) const {
            return TVec2{static_cast<T>(x * factor), static_cast<T>(y * factor)};
        }

        T x{};
        T y{};
    };

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

        operator const TVec2<f32> &() const {
            return *reinterpret_cast<const TVec2<f32> *>(this);
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

        void setTrans(MtxPtr matrix) {
            set(matrix[0][3], matrix[1][3], matrix[2][3]);
        }

        void set(f32 value) {
            x = value;
            y = value;
            z = value;
        }

        void set2(f32 value) {
            z = value;
            y = value;
            x = value;
        }

        template <typename T>
        void setAll(f32 value) {
            set(static_cast<T>(value));
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

        [[nodiscard]] TVec3 scaleInline(f32 value) const {
            auto result = *this;
            result.scale(value);
            return result;
        }

        [[nodiscard]] TVec3 multiplyOperatorInline(f32 value) const {
            return scaleInline(value);
        }

        [[nodiscard]] TVec3 multiplyOperatorInline2(f32 value) const {
            return scaleInline(value);
        }

        void mul(const TVec3& value) {
            mul(*this, value);
        }

        void mul(const TVec3& lhs, const TVec3& rhs) {
            set(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
        }

        void mult(f32 value) {
            scale(value);
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

        void lerp(const TVec3 &start, const TVec3 &end, f32 rate) {
            x = start.x + ((end.x - start.x) * rate);
            y = start.y + ((end.y - start.y) * rate);
            z = start.z + ((end.z - start.z) * rate);
        }

        [[nodiscard]] f32 dot(const TVec3 &value) const {
            return PSVECDotProduct(this, &value);
        }

        [[nodiscard]] f32 squared() const {
            return JMathInlineVEC::PSVECSquareMag(this);
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
            return PSVECMag(this);
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

        f32 setLength(const TVec3& rVec, f32 newlength) {
            f32 oldlength = rVec.squared();
            if (oldlength <= JGeometry::TUtil< f32 >::epsilon()) {
                zero();
                return 0.0f;
            }
            f32 lengthinv = JGeometry::TUtil< f32 >::inv_sqrt(oldlength);
            scale(lengthinv * newlength, rVec);
            return lengthinv * oldlength;
        };

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
            PSVECCrossProduct(&lhs, &rhs, this);
        }

        [[nodiscard]] TVec3 cross(const TVec3 &value) const {
            TVec3 result;
            PSVECCrossProduct(this, &value, &result);
            return result;
        }

        [[nodiscard]] f32 angle(const TVec3 &value) const {
            const f32 crossPart = cross(value).length();
            const f32 dotPart = dot(value);
            return std::fabs(JMAATan2(crossPart, dotPart));
        }

        [[nodiscard]] bool orientation(const TVec3 &a, const TVec3 &b) const {
            return a.cross(b).dot(*this) < 0.0F;
        }

        void orthogonalize(const TVec3 &killDirection) {
            const TVec3 &kill = killDirection;
            JMAVECScaleAdd(&kill, this, this, -kill.dot(*this));
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

        void killElement2(const TVec3 &value, const TVec3 &killDirection) {
            scaleAdd(-killDirection.dot(value), killDirection, value);
        }

        [[nodiscard]] TVec3 killElement(const TVec3 &killDirection) const {
            TVec3 result;
            result.killElement(*this, killDirection);
            return result;
        }

        [[nodiscard]] TVec3 getOrthogonal(const TVec3 &value) const {
            TVec3 result;
            result.killElement(value, *this);
            return result;
        }

        void zero() {
            set(0.0F, 0.0F, 0.0F);
        }

        void zeroInline() {
            zero();
        }

        [[nodiscard]] bool isZero() const {
            return squared() <= TUtil<f32>::epsilon();
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

using TVec2s = JGeometry::TVec2<s16>;
using TVec2f = JGeometry::TVec2<f32>;
using TVec3s = JGeometry::TVec3<s16>;
using TVec3f = JGeometry::TVec3<f32>;

[[nodiscard]] constexpr TVec3f operator*(f32 scaleValue, const TVec3f &value) {
    return value * scaleValue;
}
