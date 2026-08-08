#include "Game/Util/MathUtil.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace {
    auto sRandomSeed = std::uint32_t{0U};

    TQuat4f quaternion_from_axes(const TVec3f &side, const TVec3f &up, const TVec3f &front) {
        const auto trace = side.x + up.y + front.z;
        auto result = TQuat4f{};
        if (trace > 0.0F) {
            const auto scale = std::sqrt(trace + 1.0F) * 2.0F;
            result.set((up.z - front.y) / scale, (front.x - side.z) / scale, (side.y - up.x) / scale, 0.25F * scale);
        } else if (side.x > up.y && side.x > front.z) {
            const auto scale = std::sqrt(1.0F + side.x - up.y - front.z) * 2.0F;
            result.set(0.25F * scale, (up.x + side.y) / scale, (front.x + side.z) / scale, (up.z - front.y) / scale);
        } else if (up.y > front.z) {
            const auto scale = std::sqrt(1.0F + up.y - side.x - front.z) * 2.0F;
            result.set((up.x + side.y) / scale, 0.25F * scale, (front.y + up.z) / scale, (front.x - side.z) / scale);
        } else {
            const auto scale = std::sqrt(1.0F + front.z - side.x - up.y) * 2.0F;
            result.set((front.x + side.z) / scale, (front.y + up.z) / scale, 0.25F * scale, (side.y - up.x) / scale);
        }
        result.normalize();
        return result;
    }
}  // namespace

namespace MR {
    f32 getRandom() {
        sRandomSeed = (sRandomSeed * 0x0019660DU) + 0x3C6EF35FU;
        const auto value = (sRandomSeed >> 9U) | 0x3F800000U;
        return std::bit_cast<f32>(value) - 1.0F;
    }

    f32 getRandom(f32 min, f32 max) {
        return min + ((max - min) * getRandom());
    }

    s32 getRandom(s32 min, s32 max) {
        return static_cast<s32>(getRandom(static_cast<f32>(min), static_cast<f32>(max)));
    }

    f32 getRandomDegree() {
        return getRandom(0.0F, 360.0F);
    }

    void getRandomVector(TVec3f *pDst, f32 range) {
        if (pDst == nullptr) {
            return;
        }

        const auto x = getRandom(-range, range);
        const auto y = getRandom(-range, range);
        const auto z = getRandom(-range, range);
        pDst->set(x, y, z);
    }

    void addRandomVector(TVec3f *pDst, const TVec3f &rSrc, f32 range) {
        if (pDst == nullptr) {
            return;
        }

        const auto x = getRandom(-range, range);
        const auto y = getRandom(-range, range);
        const auto z = getRandom(-range, range);
        pDst->set(rSrc + TVec3f{x, y, z});
    }

    f32 mod(f32 x, f32 y) {
        return std::fmod(x, y);
    }

    bool isInRange(f32 value, f32 bound1, f32 bound2) {
        if (bound1 > bound2) {
            if (value < bound2) {
                return false;
            }
            return !(value > bound1);
        }

        if (value < bound1) {
            return false;
        }
        return !(value > bound2);
    }

    bool isNearZero(f32 x, f32 tolerance) {
        return std::fabs(x) < tolerance;
    }

    bool isNearZero(const TVec3f &rVec, f32 tolerance) {
        return std::fabs(rVec.x) <= tolerance && std::fabs(rVec.y) <= tolerance && std::fabs(rVec.z) <= tolerance;
    }

    void makeAxisVerticalZX(TVec3f *pDst, const TVec3f &rAxis) {
        if (pDst == nullptr) {
            return;
        }

        pDst->killElement(TVec3f{0.0F, 0.0F, 1.0F}, rAxis);
        if (isNearZero(*pDst)) {
            pDst->killElement(TVec3f{1.0F, 0.0F, 0.0F}, rAxis);
        }
        normalize(pDst);
    }

    f32 calcPerpendicFootToLineInside(TVec3f *pOut, const TVec3f &rPoint, const TVec3f &rTip,
                                      const TVec3f &rTail) {
        auto line = rTail - rTip;
        auto parameter = (rPoint.dot(line) - rTip.dot(line)) / line.squared();
        parameter = JGeometry::TUtil<f32>::clamp(parameter, 0.0F, 1.0F);
        line.scale(parameter);
        pOut->set(rTip);
        pOut->add(line);
        return parameter;
    }

    bool calcReboundVelocity(TVec3f *pVelocity, const TVec3f &rNormal, f32 restitution) {
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

    bool calcReboundVelocity(TVec3f *pVelocity, const TVec3f &rNormal, f32 restitution, f32 tangentScale) {
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

    void rotateVecDegree(TVec3f *pDst, const TVec3f &rAxis, f32 degree) {
        if (pDst == nullptr) {
            return;
        }

        const auto source = *pDst;
        rotateVecDegree(pDst, source, rAxis, degree);
    }

    void rotateVecDegree(TVec3f *pDst, const TVec3f &rSrc, const TVec3f &rAxis, f32 degree) {
        if (pDst == nullptr) {
            return;
        }

        auto axis = rAxis;
        if (axis.normalize() <= JGeometry::TUtil<f32>::epsilon()) {
            pDst->set(rSrc);
            return;
        }

        const auto radians = degree * (std::numbers::pi_v<f32> / 180.0F);
        const auto sine = std::sin(radians);
        const auto cosine = std::cos(radians);
        const auto axisProjection = axis * (axis.dot(rSrc) * (1.0F - cosine));
        pDst->set((rSrc * cosine) + (axis.cross(rSrc) * sine) + axisProjection);
    }

    void normalize(TVec3f *pVec) {
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

    void normalize(const TVec3f &rSrc, TVec3f *pDst) {
        if (pDst == nullptr) {
            return;
        }

        pDst->set(rSrc);
        normalize(pDst);
    }

    bool normalizeOrZero(TVec3f *pVec) {
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

    bool normalizeOrZero(const TVec3f &rSrc, TVec3f *pDst) {
        if (pDst == nullptr) {
            return true;
        }

        pDst->set(rSrc);
        return normalizeOrZero(pDst);
    }

    void separateScalarAndDirection(f32 *scalar, TVec3f *direction, const TVec3f &vector) {
        *scalar = vector.length();
        if (isNearZero(vector)) {
            direction->zero();
        } else {
            normalize(vector, direction);
        }
    }

    f32 vecKillElement(const TVec3f &rVector, const TVec3f &rDirection, TVec3f *pOut) {
        if (pOut == nullptr) {
            return 0.0F;
        }
        auto direction = rDirection;
        if (normalizeOrZero(&direction)) {
            pOut->set(rVector);
            return 0.0F;
        }
        const auto scalar = rVector.dot(direction);
        pOut->set(rVector - (direction * scalar));
        return scalar;
    }

    f32 getMaxElement(const TVec3f &rVec) {
        if (rVec.x > rVec.y && rVec.x > rVec.z) {
            return rVec.x;
        }
        return rVec.y > rVec.z ? rVec.y : rVec.z;
    }

    f32 frsqrte(f32 x) {
        return x > 0.0F ? std::sqrt(x) : x;
    }

    f32 fastSqrtf(f32 x) {
        return x > 0.0F ? std::sqrt(x) : x;
    }

    void blendQuatUpFront(TQuat4f *pQuat, const TVec3f &rUp, const TVec3f &rFront, f32 upRate, f32 frontRate) {
        if (pQuat == nullptr) {
            return;
        }
        auto current_up = TVec3f{};
        auto current_front = TVec3f{};
        pQuat->normalize();
        pQuat->getYDir(current_up);
        pQuat->getZDir(current_front);

        auto up = current_up + ((rUp - current_up) * clamp(upRate, 0.0F, 1.0F));
        if (normalizeOrZero(&up)) {
            up.set(0.0F, 1.0F, 0.0F);
        }
        auto front = current_front + ((rFront - current_front) * clamp(frontRate, 0.0F, 1.0F));
        vecKillElement(front, up, &front);
        if (normalizeOrZero(&front)) {
            makeAxisVerticalZX(&front, up);
        }
        auto side = up.cross(front);
        if (normalizeOrZero(&side)) {
            side.set(1.0F, 0.0F, 0.0F);
        }
        front = side.cross(up);
        normalize(&front);
        *pQuat = quaternion_from_axes(side, up, front);
    }
}  // namespace MR

const Vec gZeroVec = {0.0F, 0.0F, 0.0F};
