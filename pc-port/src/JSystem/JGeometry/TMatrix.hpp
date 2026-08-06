#pragma once

#include "JSystem/JGeometry/TUtil.hpp"
#include "JSystem/JGeometry/TQuat.hpp"
#include "JSystem/JGeometry/TVec.hpp"

#include <cmath>

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
        void getXDir(TVec3f &destination) const {
            destination.set(this->mMtx[0][0], this->mMtx[1][0], this->mMtx[2][0]);
        }

        void getYDir(TVec3f &destination) const {
            destination.set(this->mMtx[0][1], this->mMtx[1][1], this->mMtx[2][1]);
        }

        void getZDir(TVec3f &destination) const {
            destination.set(this->mMtx[0][2], this->mMtx[1][2], this->mMtx[2][2]);
        }

        void getEulerXYZ(TVec3f &destination) const {
            const f32 y = std::asin(TUtil<f32>::clamp(-this->mMtx[2][0], -1.0F, 1.0F));
            const f32 cosY = std::cos(y);

            if (std::fabs(cosY) <= TUtil<f32>::epsilon()) {
                destination.set(std::atan2(-this->mMtx[0][1], this->mMtx[1][1]), y, 0.0F);
                return;
            }

            destination.set(std::atan2(this->mMtx[2][1], this->mMtx[2][2]), y,
                            std::atan2(this->mMtx[1][0], this->mMtx[0][0]));
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
    };

    template <typename T>
    struct TPosition3 : public TRotation3<T> {
        TPosition3() = default;

        explicit TPosition3(MtxPtr source) {
            this->set(source);
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
    };
}  // namespace JGeometry

using TSMtxf = JGeometry::SMatrix34C<f32>;
using TMtx34f = JGeometry::TMatrix34<TSMtxf>;
using TRot3f = JGeometry::TRotation3<TMtx34f>;
using TPos3f = JGeometry::TPosition3<TMtx34f>;

static_assert(sizeof(TSMtxf) == sizeof(Mtx));
static_assert(sizeof(TPos3f) == sizeof(Mtx));
