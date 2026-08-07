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

        void normalize() {
            const auto length_squared = (x * x) + (y * y) + (z * z) + (w * w);
            if (length_squared <= static_cast<T>(1.0e-12)) {
                set(0, 0, 0, 1);
                return;
            }

            const auto inverse_length = static_cast<T>(1) / std::sqrt(length_squared);
            x *= inverse_length;
            y *= inverse_length;
            z *= inverse_length;
            w *= inverse_length;
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

        void setRotate(const TVec3f& from, const TVec3f& to, T rate) {
            const auto axis = from.cross(to);
            const auto cross_length = axis.length();
            if (cross_length <= JGeometry::TUtil<T>::epsilon()) {
                set(0, 0, 0, 1);
                return;
            }

            const auto half_angle = rate * (std::atan2(cross_length, from.dot(to)) * static_cast<T>(0.5));
            const auto axis_scale = std::sin(half_angle) / cross_length;
            set(axis.x * axis_scale, axis.y * axis_scale, axis.z * axis_scale, std::cos(half_angle));
        }

        void setRotate(const TVec3f& axis, T angle) {
            const auto half_angle = angle * static_cast<T>(0.5);
            const auto axis_scale = std::sin(half_angle);
            set(axis.x * axis_scale, axis.y * axis_scale, axis.z * axis_scale, std::cos(half_angle));
        }

        void rotate(TVec3f& vector) const {
            const auto intermediate_x = (y * vector.z) - (z * vector.y) + (w * vector.x);
            const auto intermediate_y = (-x * vector.z) + (z * vector.x) + (w * vector.y);
            const auto intermediate_z = (x * vector.y) - (y * vector.x) + (w * vector.z);
            const auto intermediate_w = (-x * vector.x) - (y * vector.y) - (z * vector.z);

            vector.set((intermediate_x * w) - (intermediate_y * z) + (intermediate_z * y) - (intermediate_w * x),
                       (intermediate_x * z) + (intermediate_y * w) - (intermediate_z * x) - (intermediate_w * y),
                       -(intermediate_x * y) + (intermediate_y * x) + (intermediate_z * w) - (intermediate_w * z));
        }

        void slerp(const TQuat4& from, const TQuat4& to, T rate) {
            auto target = to;
            auto dot = (from.x * target.x) + (from.y * target.y) + (from.z * target.z) + (from.w * target.w);
            if (dot < 0) {
                dot = -dot;
                target.x = -target.x;
                target.y = -target.y;
                target.z = -target.z;
                target.w = -target.w;
            }

            rate = std::clamp(rate, static_cast<T>(0), static_cast<T>(1));
            if (dot > static_cast<T>(0.9995)) {
                set(from.x + ((target.x - from.x) * rate), from.y + ((target.y - from.y) * rate),
                    from.z + ((target.z - from.z) * rate), from.w + ((target.w - from.w) * rate));
                normalize();
                return;
            }

            const auto angle = std::acos(std::clamp(dot, static_cast<T>(-1), static_cast<T>(1)));
            const auto denominator = std::sin(angle);
            const auto from_weight = std::sin((static_cast<T>(1) - rate) * angle) / denominator;
            const auto to_weight = std::sin(rate * angle) / denominator;
            set((from.x * from_weight) + (target.x * to_weight), (from.y * from_weight) + (target.y * to_weight),
                (from.z * from_weight) + (target.z * to_weight), (from.w * from_weight) + (target.w * to_weight));
        }
    };
}  // namespace JGeometry

using TQuat4f = JGeometry::TQuat4<f32>;

static_assert(sizeof(TQuat4f) == sizeof(f32) * 4U);
