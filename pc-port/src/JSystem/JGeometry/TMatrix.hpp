#pragma once

#include "JSystem/JGeometry/TUtil.hpp"
#include "JSystem/JGeometry/TQuat.hpp"
#include "JSystem/JGeometry/TVec.hpp"

#include <cmath>

#include <revolution/mtx.h>
#include <revolution/types.h>

namespace JGeometry {
    template <typename T>
    struct SMatrix34C {
        using ArrType = T[4];

        void set(const ArrType *source) {
            for (int row = 0; row < 3; ++row) {
                for (int column = 0; column < 4; ++column) {
                    mMtx[row][column] = source[row][column];
                }
            }
        }

        void set(const SMatrix34C &source) {
            set(source.mMtx);
        }

        void setInline(const SMatrix34C &source) {
            set(source);
        }

        void setInline(const SMatrix34C *source) {
            if (source != nullptr) {
                set(*source);
            }
        }

        void setInline(MtxPtr source) {
            if (source != nullptr) {
                set(source);
            }
        }

        [[nodiscard]] MtxPtr toMtxPtr() {
            return mMtx;
        }

        [[nodiscard]] const T (*toMtxPtr() const)[4] {
            return mMtx;
        }

        operator ArrType *() {
            return mMtx;
        }

        operator const ArrType *() const {
            return mMtx;
        }

        [[nodiscard]] T get(int row, int column) const {
            return mMtx[row][column];
        }

        [[nodiscard]] T operator()(int row, int column) const {
            return get(row, column);
        }

        T mMtx[3][4];
    };

    template <typename T>
    struct TMatrix34 : public T {
        void identity() {
            this->mMtx[0][0] = 1.0F;
            this->mMtx[0][1] = 0.0F;
            this->mMtx[0][2] = 0.0F;
            this->mMtx[0][3] = 0.0F;
            this->mMtx[1][0] = 0.0F;
            this->mMtx[1][1] = 1.0F;
            this->mMtx[1][2] = 0.0F;
            this->mMtx[1][3] = 0.0F;
            this->mMtx[2][0] = 0.0F;
            this->mMtx[2][1] = 0.0F;
            this->mMtx[2][2] = 1.0F;
            this->mMtx[2][3] = 0.0F;
        }

        void concat(const T &lhs, const T &rhs) {
            T result;

            for (int row = 0; row < 3; ++row) {
                for (int column = 0; column < 3; ++column) {
                    result.mMtx[row][column] = lhs.mMtx[row][0] * rhs.mMtx[0][column] +
                                               lhs.mMtx[row][1] * rhs.mMtx[1][column] +
                                               lhs.mMtx[row][2] * rhs.mMtx[2][column];
                }

                result.mMtx[row][3] = lhs.mMtx[row][0] * rhs.mMtx[0][3] + lhs.mMtx[row][1] * rhs.mMtx[1][3] +
                                      lhs.mMtx[row][2] * rhs.mMtx[2][3] + lhs.mMtx[row][3];
            }

            this->set(result);
        }

        void concat(const T &rhs) {
            concat(static_cast<const T &>(*this), rhs);
        }

        void invert(const TMatrix34<T> &source) {
            MTXInverse(source.mMtx, this->mMtx);
        }

        void mult(const TVec3f &source, TVec3f &destination) const {
            const f32 sourceX = source.x;
            const f32 sourceY = source.y;
            const f32 sourceZ = source.z;
            const f32 resultX = sourceX * this->mMtx[0][0] + sourceY * this->mMtx[0][1] + sourceZ * this->mMtx[0][2] + this->mMtx[0][3];
            const f32 resultY = sourceX * this->mMtx[1][0] + sourceY * this->mMtx[1][1] + sourceZ * this->mMtx[1][2] + this->mMtx[1][3];
            const f32 resultZ = sourceX * this->mMtx[2][0] + sourceY * this->mMtx[2][1] + sourceZ * this->mMtx[2][2] + this->mMtx[2][3];
            destination.set(resultX, resultY, resultZ);
        }

        void multTranspose(const TVec3f &source, TVec3f &destination) const {
            const f32 translatedX = source.x - this->mMtx[0][3];
            const f32 translatedY = source.y - this->mMtx[1][3];
            const f32 translatedZ = source.z - this->mMtx[2][3];
            const f32 resultX = translatedX * this->mMtx[0][0] + translatedY * this->mMtx[1][0] + translatedZ * this->mMtx[2][0];
            const f32 resultY = translatedX * this->mMtx[0][1] + translatedY * this->mMtx[1][1] + translatedZ * this->mMtx[2][1];
            const f32 resultZ = translatedX * this->mMtx[0][2] + translatedY * this->mMtx[1][2] + translatedZ * this->mMtx[2][2];
            destination.set(resultX, resultY, resultZ);
        }
    };

    template <typename T>
    struct TRotation3 : public T {
        void setXDir(const TVec3f& source) {
            this->mMtx[0][0] = source.x;
            this->mMtx[1][0] = source.y;
            this->mMtx[2][0] = source.z;
        }

        void setYDir(const TVec3f& source) {
            this->mMtx[0][1] = source.x;
            this->mMtx[1][1] = source.y;
            this->mMtx[2][1] = source.z;
        }

        void setZDir(const TVec3f& source) {
            this->mMtx[0][2] = source.x;
            this->mMtx[1][2] = source.y;
            this->mMtx[2][2] = source.z;
        }

        void getXDir(TVec3f &destination) const {
            destination.set(this->mMtx[0][0], this->mMtx[1][0], this->mMtx[2][0]);
        }

        void getYDir(TVec3f &destination) const {
            destination.set(this->mMtx[0][1], this->mMtx[1][1], this->mMtx[2][1]);
        }

        void getZDir(TVec3f &destination) const {
            destination.set(this->mMtx[0][2], this->mMtx[1][2], this->mMtx[2][2]);
        }

        void setXYZDir(const TVec3f &x_direction, const TVec3f &y_direction, const TVec3f &z_direction) {
            this->mMtx[0][0] = x_direction.x;
            this->mMtx[1][0] = x_direction.y;
            this->mMtx[2][0] = x_direction.z;
            this->mMtx[0][1] = y_direction.x;
            this->mMtx[1][1] = y_direction.y;
            this->mMtx[2][1] = y_direction.z;
            this->mMtx[0][2] = z_direction.x;
            this->mMtx[1][2] = z_direction.y;
            this->mMtx[2][2] = z_direction.z;
        }

        void setQuat(const TQuat4f& quaternion) {
            quaternion.makeMtx(this->mMtx);
        }

        void getEulerXYZ(TVec3f &destination) const {
            if (this->mMtx[2][0] - 1.0F >= -TUtil<f32>::epsilon()) {
                destination.set(std::atan2(-this->mMtx[0][1], this->mMtx[1][1]),
                                -1.57079632679489661923F, 0.0F);
                return;
            }

            if (this->mMtx[2][0] + 1.0F <= TUtil<f32>::epsilon()) {
                destination.set(std::atan2(this->mMtx[0][1], this->mMtx[1][1]),
                                1.57079632679489661923F, 0.0F);
                return;
            }

            destination.set(std::atan2(this->mMtx[2][1], this->mMtx[2][2]),
                            std::asin(TUtil<f32>::clamp(-this->mMtx[2][0], -1.0F, 1.0F)),
                            std::atan2(this->mMtx[1][0], this->mMtx[0][0]));
        }

        void getEuler(TVec3f &destination) const {
            getEulerXYZ(destination);
        }

        void getEulerDegree(TVec3f &destination) const {
            getEulerXYZ(destination);
            destination.scale(180.0F / 3.1415927F);
        }

        void mult33(const TVec3f &source, TVec3f &destination) const {
            const f32 sourceX = source.x;
            const f32 sourceY = source.y;
            const f32 sourceZ = source.z;
            const f32 resultX = sourceX * this->mMtx[0][0] + sourceY * this->mMtx[0][1] + sourceZ * this->mMtx[0][2];
            const f32 resultY = sourceX * this->mMtx[1][0] + sourceY * this->mMtx[1][1] + sourceZ * this->mMtx[1][2];
            const f32 resultZ = sourceX * this->mMtx[2][0] + sourceY * this->mMtx[2][1] + sourceZ * this->mMtx[2][2];
            destination.set(resultX, resultY, resultZ);
        }

        void mult33(TVec3f &vector) const {
            mult33(vector, vector);
        }

        void setRotate(const TVec3f& axis, f32 angle) {
            TVec3f normalized_axis;
            normalized_axis.normalize(axis);

            const auto sine = std::sin(angle);
            const auto cosine = std::cos(angle);
            const auto one_minus_cosine = 1.0F - cosine;
            const auto x = normalized_axis.x;
            const auto y = normalized_axis.y;
            const auto z = normalized_axis.z;

            this->mMtx[0][0] = cosine + (one_minus_cosine * x * x);
            this->mMtx[0][1] = (one_minus_cosine * x * y) - (sine * z);
            this->mMtx[0][2] = (one_minus_cosine * x * z) + (sine * y);
            this->mMtx[1][0] = (one_minus_cosine * x * y) + (sine * z);
            this->mMtx[1][1] = cosine + (one_minus_cosine * y * y);
            this->mMtx[1][2] = (one_minus_cosine * y * z) - (sine * x);
            this->mMtx[2][0] = (one_minus_cosine * x * z) - (sine * y);
            this->mMtx[2][1] = (one_minus_cosine * y * z) + (sine * x);
            this->mMtx[2][2] = cosine + (one_minus_cosine * z * z);
        }
    };

    template <typename T>
    struct TPosition3 : public TRotation3<T> {
        TPosition3() = default;

        TPosition3(MtxPtr source) {
            this->set(source);
        }

        TPosition3 &operator=(MtxPtr source) {
            this->set(source);
            return *this;
        }

        void getTrans(TVec3f &destination) const {
            destination.set(this->mMtx[0][3], this->mMtx[1][3], this->mMtx[2][3]);
        }

        void setTrans(const TVec3f &source) {
            setTrans(source.x, source.y, source.z);
        }

        void setTrans(f32 x, f32 y, f32 z) {
            this->mMtx[0][3] = x;
            this->mMtx[1][3] = y;
            this->mMtx[2][3] = z;
        }

        void zeroTrans() {
            setTrans(0.0F, 0.0F, 0.0F);
        }

        void makeTrans(const TVec3f &translation) {
            this->identity();
            setTrans(translation);
        }

        void makeTrans(f32 x, f32 y, f32 z) {
            this->identity();
            setTrans(x, y, z);
        }

        void makeRotate(const TVec3f &axis, f32 angle) {
            zeroTrans();
            this->setRotate(axis, angle);
        }

        void makeQuat(const TQuat4f& quaternion) {
            zeroTrans();
            this->setQuat(quaternion);
        }

        void setQT(const TQuat4f& quaternion, const TVec3f& translation) {
            this->setQuat(quaternion);
            setTrans(translation);
        }
    };

    template <typename T>
    struct SMatrix44C {
        void set(const T source[4][4]) {
            for (int row = 0; row < 4; ++row) {
                for (int column = 0; column < 4; ++column) {
                    mMtx[row][column] = source[row][column];
                }
            }
        }

        void set(const SMatrix44C& source) {
            set(source.mMtx);
        }

        [[nodiscard]] T get(int row, int column) const {
            return mMtx[row][column];
        }

        T mMtx[4][4]{};
    };

    template <typename T>
    struct TMatrix44 : public T {
        void identity() {
            for (int row = 0; row < 4; ++row) {
                for (int column = 0; column < 4; ++column) {
                    this->mMtx[row][column] = row == column ? 1.0F : 0.0F;
                }
            }
        }
    };

    template <typename T>
    struct TProjection3 : public T {
        void makePerspective(f32 fovy_degrees, f32 aspect, f32 near_clip, f32 far_clip) {
            const auto half_angle = fovy_degrees * 3.14159265358979323846F / 360.0F;
            const auto focal_length = 1.0F / std::tan(half_angle);
            const auto depth_scale = 1.0F / (far_clip - near_clip);

            this->mMtx[0][0] = focal_length / aspect;
            this->mMtx[0][1] = 0.0F;
            this->mMtx[0][2] = 0.0F;
            this->mMtx[0][3] = 0.0F;
            this->mMtx[1][0] = 0.0F;
            this->mMtx[1][1] = focal_length;
            this->mMtx[1][2] = 0.0F;
            this->mMtx[1][3] = 0.0F;
            this->mMtx[2][0] = 0.0F;
            this->mMtx[2][1] = 0.0F;
            this->mMtx[2][2] = -near_clip * depth_scale;
            this->mMtx[2][3] = -(far_clip * near_clip) * depth_scale;
            this->mMtx[3][0] = 0.0F;
            this->mMtx[3][1] = 0.0F;
            this->mMtx[3][2] = -1.0F;
            this->mMtx[3][3] = 0.0F;
        }

        void makeTrans(f32 offset_x, f32 offset_y) {
            this->identity();
            this->mMtx[0][3] = offset_x;
            this->mMtx[1][3] = offset_y;
        }

        void makeTrans(const TVec2f& offset) {
            makeTrans(offset.x, offset.y);
        }
    };
}  // namespace JGeometry

using TSMtxf = JGeometry::SMatrix34C<f32>;
using TMtx34f = JGeometry::TMatrix34<TSMtxf>;
using TRot3f = JGeometry::TRotation3<TMtx34f>;
using TPos3f = JGeometry::TPosition3<TMtx34f>;
using TSMtx44f = JGeometry::SMatrix44C<f32>;
using TMtx44f = JGeometry::TMatrix44<TSMtx44f>;
using TProj3f = JGeometry::TProjection3<TMtx44f>;

static_assert(sizeof(TSMtxf) == sizeof(Mtx));
static_assert(sizeof(TPos3f) == sizeof(Mtx));
static_assert(sizeof(TProj3f) == sizeof(f32) * 16U);
