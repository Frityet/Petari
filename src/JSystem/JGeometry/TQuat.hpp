#pragma once

#include "JSystem/JGeometry/TVec.hpp"

#include <algorithm>
#include <cmath>

namespace JGeometry {
    template <typename T>
    struct TQuat4 {
        T x{};
        T y{};
        T z{};
        T w{1};

        TQuat4() = default;
        TQuat4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {
        }

        template <typename U>
        void set(U x_, U y_, U z_, U w_) {
            x = static_cast<T>(x_);
            y = static_cast<T>(y_);
            z = static_cast<T>(z_);
            w = static_cast<T>(w_);
        }

        void set(const TQuat4& source) {
            set(source.x, source.y, source.z, source.w);
        }

        void mult(const TQuat4& rotation) {
            // The retail single-argument overload applies a world-space
            // rotation: rotation * this, rather than this * rotation.
            mult(rotation, *this);
        }

        void mult(const TQuat4& lhs, const TQuat4& rhs) {
            const auto result_w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z;
            const auto result_x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y;
            const auto result_y = lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z;
            const auto result_z = lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x;
            set(result_x, result_y, result_z, result_w);
        }

        void normalize() {
            const auto length_squared = squared();
            if (length_squared <= JGeometry::TUtil<T>::epsilon()) {
                set(0, 0, 0, 1);
                return;
            }

            const auto inverse_length = JGeometry::TUtil<T>::inv_sqrt(length_squared);
            x *= inverse_length;
            y *= inverse_length;
            z *= inverse_length;
            w *= inverse_length;
        }

        void normalize(const TQuat4& source) {
            set(source);
            normalize();
        }

        [[nodiscard]] T dot(const TQuat4& other) const {
            return x * other.x + y * other.y + z * other.z + w * other.w;
        }

        [[nodiscard]] T squared() const {
            return dot(*this);
        }

        void getXDir(TVec3f& out) const {
            out.set(1.0F - (2.0F * y * y) - (2.0F * z * z), (2.0F * x * y) + (2.0F * w * z),
                    (2.0F * x * z) - (2.0F * w * y));
        }

        void getYDir(TVec3f& out) const {
            out.set((2.0F * x * y) - (2.0F * w * z), 1.0F - (2.0F * x * x) - (2.0F * z * z),
                    (2.0F * y * z) + (2.0F * w * x));
        }

        void getZDir(TVec3f& out) const {
            out.set((2.0F * x * z) + (2.0F * w * y), (2.0F * y * z) - (2.0F * w * x),
                    1.0F - (2.0F * x * x) - (2.0F * y * y));
        }

        void getEuler(TVec3f& out) const {
            const auto sin_x = 2.0F * ((w * x) + (y * z));
            const auto cos_x = 1.0F - (2.0F * ((x * x) + (y * y)));
            const auto sin_y = std::clamp(2.0F * ((w * y) - (z * x)), -1.0F, 1.0F);
            const auto sin_z = 2.0F * ((w * z) + (x * y));
            const auto cos_z = 1.0F - (2.0F * ((y * y) + (z * z)));
            out.set(std::atan2(sin_x, cos_x), std::asin(sin_y), std::atan2(sin_z, cos_z));
        }

        f32 getRotate(TVec3< T >& rAxis) {
            const TVec3< T > vector(this->x, this->y, this->z);
            f32 length = vector.squared();
            if (length <= JGeometry::TUtil< f32 >::epsilon()) {
                rAxis.zero();
                return 0.0f;
            }

            f32 lengthinv = JGeometry::TUtil< f32 >::inv_sqrt(length);
            rAxis.scale(lengthinv, vector);
            return JGeometry::TUtil< f32 >::acos(this->w) * 2.0f;
        }

        void setRotate(const TVec3f& from, const TVec3f& to, T rate) {
            const auto axis = from.cross(to);
            const auto cross_length = axis.length();
            if (cross_length <= JGeometry::TUtil<T>::epsilon()) {
                set(0, 0, 0, 1);
                return;
            }

            const auto half_angle = rate * (JMAATan2(cross_length, from.dot(to)) * static_cast<T>(0.5));
            const auto axis_scale = static_cast<T>(std::sin(static_cast<f64>(half_angle))) / cross_length;
            set(axis.x * axis_scale, axis.y * axis_scale, axis.z * axis_scale, static_cast<T>(std::cos(static_cast<f64>(half_angle))));
        }

        void setRotate(const TVec3<T>& from, const TVec3<T>& to);

        void setRotate(const TVec3f& axis, T angle) {
            const auto half_angle = angle * static_cast<T>(0.5);
            const auto axis_scale = std::sin(half_angle);
            set(axis.x * axis_scale, axis.y * axis_scale, axis.z * axis_scale, std::cos(half_angle));
        }

        void rotate(TVec3f& vector) const {
            transform(vector, vector);
        }

        void transform(const TVec3<T>& vector, TVec3<T>& destination) const {
            TQuat4<T> intermediate;
            intermediate.x = (y * vector.z) - (z * vector.y) + (w * vector.x);
            intermediate.y = (-x * vector.z) + (z * vector.x) + (w * vector.y);
            intermediate.z = (x * vector.y) - (y * vector.x) + (w * vector.z);
            intermediate.w = (-x * vector.x) - (y * vector.y) - (z * vector.z);

            destination.template set<T>(
                intermediate.x * w + intermediate.y * -z - intermediate.z * -y + intermediate.w * -x,
                -intermediate.x * -z + intermediate.y * w + intermediate.z * -x + intermediate.w * -y,
                intermediate.x * -y - intermediate.y * -x + intermediate.z * w + intermediate.w * -z);
        }

        void transform(TVec3<T>& vector) const {
            transform(vector, vector);
        }

        void slerp(const TQuat4& from, const TQuat4& to, T rate) {
            set(from);
            slerp(to, rate);
        }

        void slerp(const TQuat4& target, T rate);

        void makeMtx(MtxPtr matrix) const {
            const auto yy = 2.0F * y * y;
            const auto zz = 2.0F * z * z;
            const auto xx = 2.0F * x * x;
            const auto xy = 2.0F * x * y;
            const auto xz = 2.0F * x * z;
            const auto yz = 2.0F * y * z;
            const auto wx = 2.0F * w * x;
            const auto wy = 2.0F * w * y;
            const auto wz = 2.0F * w * z;

            matrix[0][0] = 1.0F - yy - zz;
            matrix[0][1] = xy - wz;
            matrix[0][2] = xz + wy;
            matrix[1][0] = xy + wz;
            matrix[1][1] = 1.0F - xx - zz;
            matrix[1][2] = yz - wx;
            matrix[2][0] = xz - wy;
            matrix[2][1] = yz + wx;
            matrix[2][2] = 1.0F - xx - yy;
        }
    };

    template <>
    void TQuat4<f32>::slerp(const TQuat4<f32>& target, f32 rate);
    template <>
    void TQuat4<f32>::setRotate(const TVec3<f32>& from, const TVec3<f32>& to);
}  // namespace JGeometry

using TQuat4f = JGeometry::TQuat4<f32>;

static_assert(sizeof(TQuat4f) == sizeof(f32) * 4U);
