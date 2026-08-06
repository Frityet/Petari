#include "Game/Util/MathUtil.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace {
    auto sRandomSeed = std::uint32_t{0U};
}  // namespace

namespace MR {
    f32 getRandom() {
        sRandomSeed = (sRandomSeed * 0x0019660DU) + 0x3C6EF35FU;
        const auto value = (sRandomSeed >> 9U) | 0x3F800000U;
        return std::bit_cast< f32 >(value) - 1.0F;
    }

    f32 getRandom(f32 min, f32 max) {
        return min + ((max - min) * getRandom());
    }

    s32 getRandom(s32 min, s32 max) {
        return static_cast< s32 >(getRandom(static_cast< f32 >(min), static_cast< f32 >(max)));
    }

    f32 getRandomDegree() {
        return getRandom(0.0F, 360.0F);
    }

    void getRandomVector(TVec3f* pDst, f32 range) {
        if (pDst == nullptr) {
            return;
        }

        const auto x = getRandom(-range, range);
        const auto y = getRandom(-range, range);
        const auto z = getRandom(-range, range);
        pDst->set(x, y, z);
    }

    void addRandomVector(TVec3f* pDst, const TVec3f& rSrc, f32 range) {
        if (pDst == nullptr) {
            return;
        }

        const auto x = getRandom(-range, range);
        const auto y = getRandom(-range, range);
        const auto z = getRandom(-range, range);
        pDst->set(rSrc + TVec3f{x, y, z});
    }

    f32 repeat(f32 value, f32 min, f32 max) {
        return min + std::fmod(max + (value - min), max);
    }

    f32 mod(f32 x, f32 y) {
        return std::fmod(x, y);
    }

    bool isNearZero(f32 x, f32 tolerance) {
        return std::fabs(x) < tolerance;
    }

    bool isNearZero(const TVec3f& rVec, f32 tolerance) {
        return std::fabs(rVec.x) <= tolerance && std::fabs(rVec.y) <= tolerance && std::fabs(rVec.z) <= tolerance;
    }

    void makeAxisVerticalZX(TVec3f* pDst, const TVec3f& rAxis) {
        if (pDst == nullptr) {
            return;
        }

        pDst->killElement(TVec3f{0.0F, 0.0F, 1.0F}, rAxis);
        if (isNearZero(*pDst)) {
            pDst->killElement(TVec3f{1.0F, 0.0F, 0.0F}, rAxis);
        }
        normalize(pDst);
    }

    bool calcReboundVelocity(TVec3f* pVelocity, const TVec3f& rNormal, f32 restitution) {
        if (pVelocity == nullptr) {
            return false;
        }

        const auto normalSpeed = pVelocity->dot(rNormal);
        if (normalSpeed >= 0.0F) {
            return false;
        }

        pVelocity->sub(rNormal * ((1.0F + restitution) * normalSpeed));
        return true;
    }

    bool calcReboundVelocity(TVec3f* pVelocity, const TVec3f& rNormal, f32 restitution, f32 tangentScale) {
        if (pVelocity == nullptr) {
            return false;
        }

        const auto normalSpeed = pVelocity->dot(rNormal);
        if (normalSpeed >= 0.0F) {
            return false;
        }

        const auto normalVelocity = rNormal * normalSpeed;
        const auto tangentVelocity = (*pVelocity - normalVelocity) * tangentScale;
        pVelocity->set(tangentVelocity - (normalVelocity * restitution));
        return true;
    }

    void rotateVecDegree(TVec3f* pDst, const TVec3f& rAxis, f32 degree) {
        if (pDst == nullptr) {
            return;
        }

        const auto source = *pDst;
        rotateVecDegree(pDst, source, rAxis, degree);
    }

    void rotateVecDegree(TVec3f* pDst, const TVec3f& rSrc, const TVec3f& rAxis, f32 degree) {
        if (pDst == nullptr) {
            return;
        }

        auto axis = rAxis;
        if (axis.normalize() <= JGeometry::TUtil< f32 >::epsilon()) {
            pDst->set(rSrc);
            return;
        }

        const auto radians = degree * (std::numbers::pi_v< f32 > / 180.0F);
        const auto sine = std::sin(radians);
        const auto cosine = std::cos(radians);
        const auto axisProjection = axis * (axis.dot(rSrc) * (1.0F - cosine));
        pDst->set((rSrc * cosine) + (axis.cross(rSrc) * sine) + axisProjection);
    }

    void normalize(TVec3f* pVec) {
        if (pVec == nullptr) {
            return;
        }

        const auto length = pVec->length();
        if (length <= 0.000001F) {
            pVec->set(0.0F, 0.0F, 0.0F);
            return;
        }

        pVec->scale(1.0F / length);
    }

    void normalize(const TVec3f& rSrc, TVec3f* pDst) {
        if (pDst == nullptr) {
            return;
        }

        pDst->set(rSrc);
        normalize(pDst);
    }

    bool normalizeOrZero(TVec3f* pVec) {
        if (pVec == nullptr) {
            return true;
        }

        if (isNearZero(*pVec)) {
            pVec->zero();
            return true;
        }

        normalize(pVec);
        return false;
    }

    bool normalizeOrZero(const TVec3f& rSrc, TVec3f* pDst) {
        if (pDst == nullptr) {
            return true;
        }

        pDst->set(rSrc);
        return normalizeOrZero(pDst);
    }
}  // namespace MR
