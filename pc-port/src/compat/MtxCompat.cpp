#include "Game/Util/MtxUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

#include <cmath>
#include <stdexcept>

namespace {
    constexpr f32 cDegreesToRadians = 3.14159265358979323846F / 180.0F;
    Mtx sTemporaryRotationX = {
        {1.0F, 0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F, 0.0F},
        {0.0F, 0.0F, 1.0F, 0.0F},
    };
    Mtx sTemporaryRotationY = {
        {1.0F, 0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F, 0.0F},
        {0.0F, 0.0F, 1.0F, 0.0F},
    };
    Mtx sTemporaryRotationZ = {
        {1.0F, 0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F, 0.0F},
        {0.0F, 0.0F, 1.0F, 0.0F},
    };

    void set_axes(TPos3f *pMatrix, const TVec3f &rAxisX, const TVec3f &rAxisY, const TVec3f &rAxisZ) {
        pMatrix->mMtx[0][0] = rAxisX.x;
        pMatrix->mMtx[1][0] = rAxisX.y;
        pMatrix->mMtx[2][0] = rAxisX.z;
        pMatrix->mMtx[0][1] = rAxisY.x;
        pMatrix->mMtx[1][1] = rAxisY.y;
        pMatrix->mMtx[2][1] = rAxisY.z;
        pMatrix->mMtx[0][2] = rAxisZ.x;
        pMatrix->mMtx[1][2] = rAxisZ.y;
        pMatrix->mMtx[2][2] = rAxisZ.z;
    }
}  // namespace

namespace MR {
    void makeMtxUpFront(TPos3f *pMatrix, const TVec3f &rUp,
                        const TVec3f &rFront) {
        if (pMatrix == nullptr) {
            throw std::invalid_argument("An up/front matrix requires a real destination.");
        }

        auto axisY = rUp;
        if (axisY.normalize() <= JGeometry::TUtil<f32>::epsilon()) {
            throw std::invalid_argument("An up/front matrix requires a non-degenerate up axis.");
        }
        auto axisX = axisY.cross(rFront);
        if (axisX.normalize() <= JGeometry::TUtil<f32>::epsilon()) {
            throw std::invalid_argument(
                "An up/front matrix requires independent up and front axes.");
        }
        const auto axisZ = axisX.cross(axisY);
        set_axes(pMatrix, axisX, axisY, axisZ);
    }

    void makeMtxUpFrontPos(TPos3f *pMatrix, const TVec3f &rUp,
                           const TVec3f &rFront, const TVec3f &rPosition) {
        makeMtxUpFront(pMatrix, rUp, rFront);
        pMatrix->setTrans(rPosition);
    }

    void makeMtxRotate(MtxPtr pMatrix, s16 rotationX, s16 rotationY, s16 rotationZ) {
        const auto sinY = JMASSin(rotationY);
        const auto cosZ = JMASCos(rotationZ);
        const auto sinZ = JMASSin(rotationZ);
        const auto cosX = JMASCos(rotationX);
        const auto sinX = JMASSin(rotationX);
        const auto cosY = JMASCos(rotationY);

        const auto sinZSinY = sinZ * sinY;
        const auto cosZSinY = cosZ * sinY;
        const auto sinXSinZSinY = sinX * sinZSinY;
        const auto cosXCosZ = cosX * cosZ;

        pMatrix[2][0] = -sinY;
        pMatrix[0][0] = cosZ * cosY;
        pMatrix[1][0] = sinZ * cosY;

        const auto cosXSinZ = cosX * sinZ;
        const auto sinXSinZ = sinX * sinZ;
        const auto sinXCosZ = sinX * cosZ;
        const auto sinXCosZSinY = sinX * cosZSinY;
        const auto cosXCosZSinY = cosX * cosZSinY;
        const auto cosXSinZSinY = cosX * sinZSinY;

        pMatrix[0][3] = 0.0F;
        pMatrix[0][1] = sinXCosZSinY - cosXSinZ;
        pMatrix[0][2] = cosXCosZSinY + sinXSinZ;
        pMatrix[2][1] = sinX * cosY;
        pMatrix[1][1] = sinXSinZSinY + cosXCosZ;
        pMatrix[1][2] = cosXSinZSinY - sinXCosZ;
        pMatrix[2][2] = cosX * cosY;
        pMatrix[1][3] = 0.0F;
        pMatrix[2][3] = 0.0F;
    }

    void makeMtxRotate(MtxPtr pMatrix, f32 rotationX, f32 rotationY, f32 rotationZ) {
        makeMtxRotate(pMatrix, static_cast<s16>(rotationX * DEGREE_TO_S16),
                      static_cast<s16>(rotationY * DEGREE_TO_S16),
                      static_cast<s16>(rotationZ * DEGREE_TO_S16));
    }

    void makeMtxRotate(MtxPtr pMatrix, const TVec3f &rRotation) {
        makeMtxRotate(pMatrix, rRotation.x, rRotation.y, rRotation.z);
    }

    void makeMtxRotateY(MtxPtr pMatrix, f32 rotationY) {
        const auto radians = rotationY * cDegreesToRadians;
        const auto sinY = std::sin(radians);
        const auto cosY = std::cos(radians);

        pMatrix[0][0] = cosY;
        pMatrix[1][0] = 0.0F;
        pMatrix[2][0] = -sinY;
        pMatrix[0][1] = 0.0F;
        pMatrix[1][1] = 1.0F;
        pMatrix[2][1] = 0.0F;
        pMatrix[0][2] = sinY;
        pMatrix[1][2] = 0.0F;
        pMatrix[2][2] = cosY;
        pMatrix[0][3] = 0.0F;
        pMatrix[1][3] = 0.0F;
        pMatrix[2][3] = 0.0F;
    }

    void makeMtxTransRotateY(MtxPtr pMatrix, f32 tx, f32 ty, f32 tz, f32 rotationY) {
        makeMtxRotateY(pMatrix, rotationY);
        pMatrix[0][3] = tx;
        pMatrix[1][3] = ty;
        pMatrix[2][3] = tz;
    }

    void makeMtxTransRotateY(MtxPtr pMatrix, const LiveActor *pActor) {
        makeMtxTransRotateY(pMatrix, pActor->mPosition.x, pActor->mPosition.y,
                            pActor->mPosition.z, pActor->mRotation.y);
    }

    void makeMtxTR(MtxPtr pMatrix, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz) {
        const auto sinX = std::sin(rx * cDegreesToRadians);
        const auto sinY = std::sin(ry * cDegreesToRadians);
        const auto sinZ = std::sin(rz * cDegreesToRadians);
        const auto cosX = std::cos(rx * cDegreesToRadians);
        const auto cosY = std::cos(ry * cDegreesToRadians);
        const auto cosZ = std::cos(rz * cDegreesToRadians);

        pMatrix[0][0] = cosZ * cosY;
        pMatrix[1][0] = sinZ * cosY;
        pMatrix[2][0] = -sinY;
        pMatrix[0][1] = cosZ * sinY * sinX - sinZ * cosX;
        pMatrix[1][1] = sinZ * sinY * sinX + cosZ * cosX;
        pMatrix[2][1] = cosY * sinX;
        pMatrix[0][2] = cosZ * sinY * cosX + sinZ * sinX;
        pMatrix[1][2] = sinZ * sinY * cosX - cosZ * sinX;
        pMatrix[2][2] = cosY * cosX;
        pMatrix[0][3] = tx;
        pMatrix[1][3] = ty;
        pMatrix[2][3] = tz;
    }

    void makeMtxTR(MtxPtr pMatrix, const TVec3f &rTranslation, const TVec3f &rRotation) {
        makeMtxTR(pMatrix, rTranslation.x, rTranslation.y, rTranslation.z, rRotation.x, rRotation.y, rRotation.z);
    }

    void makeMtxTR(MtxPtr pMatrix, const LiveActor *pActor) {
        if (pActor != nullptr) {
            makeMtxTR(pMatrix, pActor->mPosition, pActor->mRotation);
        }
    }

    void makeMtxTRS(MtxPtr pMatrix, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz) {
        makeMtxTR(pMatrix, tx, ty, tz, rx, ry, rz);
        pMatrix[0][0] *= sx;
        pMatrix[1][0] *= sx;
        pMatrix[2][0] *= sx;
        pMatrix[0][1] *= sy;
        pMatrix[1][1] *= sy;
        pMatrix[2][1] *= sy;
        pMatrix[0][2] *= sz;
        pMatrix[1][2] *= sz;
        pMatrix[2][2] *= sz;
    }

    void makeMtxTRS(MtxPtr pMatrix, const TVec3f &rTranslation, const TVec3f &rRotation, const TVec3f &rScale) {
        makeMtxTRS(pMatrix, rTranslation.x, rTranslation.y, rTranslation.z, rRotation.x, rRotation.y, rRotation.z, rScale.x,
                   rScale.y, rScale.z);
    }

    void makeMtxTRS(MtxPtr pMatrix, const LiveActor *pActor) {
        if (pActor != nullptr) {
            makeMtxTRS(pMatrix, pActor->mPosition, pActor->mRotation, pActor->mScale);
        }
    }

    void preScaleMtx(MtxPtr pMatrix, f32 scale) {
        preScaleMtx(pMatrix, scale, scale, scale);
    }

    void preScaleMtx(MtxPtr pMatrix, const TVec3f &rScale) {
        preScaleMtx(pMatrix, rScale.x, rScale.y, rScale.z);
    }

    void preScaleMtx(MtxPtr pMatrix, f32 scaleX, f32 scaleY, f32 scaleZ) {
        if (pMatrix == nullptr) {
            throw std::invalid_argument("Matrix scaling requires a real matrix.");
        }

        for (auto row = 0; row < 3; ++row) {
            pMatrix[row][0] *= scaleX;
            pMatrix[row][1] *= scaleY;
            pMatrix[row][2] *= scaleZ;
        }
    }

    MtxPtr tmpMtxRotXDeg(f32 degrees) {
        const auto cosine = JMACosDegree(degrees);
        const auto sine = JMASinDegree(degrees);
        sTemporaryRotationX[1][1] = cosine;
        sTemporaryRotationX[1][2] = sine;
        sTemporaryRotationX[2][1] = -sine;
        sTemporaryRotationX[2][2] = cosine;
        return sTemporaryRotationX;
    }

    MtxPtr tmpMtxRotYDeg(f32 degrees) {
        const auto cosine = JMACosDegree(degrees);
        const auto sine = JMASinDegree(degrees);
        sTemporaryRotationY[0][0] = cosine;
        sTemporaryRotationY[0][2] = -sine;
        sTemporaryRotationY[2][0] = sine;
        sTemporaryRotationY[2][2] = cosine;
        return sTemporaryRotationY;
    }

    MtxPtr tmpMtxRotZDeg(f32 degrees) {
        const auto cosine = JMACosDegree(degrees);
        const auto sine = JMASinDegree(degrees);
        sTemporaryRotationZ[0][0] = cosine;
        sTemporaryRotationZ[0][1] = sine;
        sTemporaryRotationZ[1][0] = -sine;
        sTemporaryRotationZ[1][1] = cosine;
        return sTemporaryRotationZ;
    }

    void makeMtxUpNoSupportPos(TPos3f *pMatrix, const TVec3f &rUp, const TVec3f &rPosition) {
        if (pMatrix == nullptr) {
            return;
        }

        auto axisY = rUp;
        if (axisY.normalize() == 0.0F) {
            axisY.set(0.0F, 1.0F, 0.0F);
        }

        const auto absX = std::fabs(axisY.x);
        const auto absY = std::fabs(axisY.y);
        const auto absZ = std::fabs(axisY.z);
        const auto maxElementIsZ = absZ >= absX && absZ >= absY;
        const auto support = maxElementIsZ ? TVec3f{0.0F, 1.0F, 0.0F} : TVec3f{0.0F, 0.0F, 1.0F};

        auto axisX = axisY.cross(support);
        if (axisX.normalize() == 0.0F) {
            axisX.set(1.0F, 0.0F, 0.0F);
        }
        auto axisZ = axisX.cross(axisY);
        axisZ.normalize();

        set_axes(pMatrix, axisX, axisY, axisZ);
        pMatrix->setTrans(rPosition);
    }
}  // namespace MR
