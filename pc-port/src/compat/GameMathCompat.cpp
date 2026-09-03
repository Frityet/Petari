#include "Game/Util/MathUtil.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <stdexcept>

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
    f32 getConvergeVibrationValue(f32 x, f32 start, f32 end, f32 dampScale, f32 rate) {
        f32 vibWeight = (x * x) * (x * x);
        f32 t1 = 1.0f - x;
        f32 convergeWeight = (t1 * t1) * (t1 * t1);
        f32 dampRatio = dampScale * t1;

        f32 vibration = JMASinRadian(PI * (x + rate * vibWeight));
        return getInterpolateValue((1.0f - convergeWeight) + vibration * dampRatio, start, end);
    }

    f32 getReduceVibrationValue(f32 x, f32 time, f32 base, f32 amplitude, f32 freq) {
        // FIXME: float swap
        f32 vibMax = base + amplitude;

        f32 vib = JMACosRadian(x * (1.0f / freq * PI));
        f32 vibration = (amplitude * 0.5f) * (vib - 1.0f);
        if (x >= time) {
            return vibration + vibMax;
        } else {
            f32 t = x - time;
            f32 reduce = (1.0f - vibMax) * (1.0f / (time * time)) * t * t;
            return vibration + (vibMax + reduce);
        }
    }

    f32 normalizeAngleAbs(f32 angle) {
        auto normalized = std::fmod(angle, 2.0F * std::numbers::pi_v<f32>);
        if (normalized < 0.0F) {
            normalized += 2.0F * std::numbers::pi_v<f32>;
        }
        return normalized;
    }

    f32 diffAngleAbs(const TVec3f& left, const TVec3f& right) {
        const auto left_length = left.length();
        const auto right_length = right.length();
        if (!(left_length > 1.0e-8F) || !(right_length > 1.0e-8F)) {
            return 0.0F;
        }
        const auto cosine = std::clamp(left.dot(right) / (left_length * right_length), -1.0F, 1.0F);
        return std::acos(cosine);
    }

    bool vecBlendSphere(const TVec3f& left, const TVec3f& right, TVec3f* destination, f32 blend) {
        if (destination == nullptr) {
            return false;
        }

        const auto left_length = left.length();
        const auto right_length = right.length();
        if (!(left_length > 1.0e-8F) || !(right_length > 1.0e-8F)) {
            destination->set(left * (1.0F - blend) + right * blend);
            return false;
        }

        auto left_direction = left * (1.0F / left_length);
        auto right_direction = right * (1.0F / right_length);
        const auto cosine = std::clamp(left_direction.dot(right_direction), -1.0F, 1.0F);
        if (cosine < -0.9999F) {
            return false;
        }

        auto direction = TVec3f{};
        if (cosine > 0.9999F) {
            direction.set(left_direction * (1.0F - blend) + right_direction * blend);
            if (normalizeOrZero(&direction)) {
                return false;
            }
        } else {
            const auto angle = std::acos(cosine);
            const auto divisor = std::sin(angle);
            const auto left_weight = std::sin((1.0F - blend) * angle) / divisor;
            const auto right_weight = std::sin(blend * angle) / divisor;
            direction.set(left_direction * left_weight + right_direction * right_weight);
        }

        direction.scale(left_length * (1.0F - blend) + right_length * blend);
        destination->set(direction);
        return true;
    }

    void vecRotAxis(const TVec3f& source, const TVec3f& target, const TVec3f& axis,
                    TVec3f* destination, f32 maximum_angle) {
        if (destination == nullptr) {
            return;
        }

        auto unit_axis = axis;
        if (normalizeOrZero(&unit_axis)) {
            destination->set(source);
            return;
        }

        auto angle = std::min(diffAngleAbs(source, target), std::fabs(maximum_angle));
        if (source.cross(target).dot(unit_axis) < 0.0F) {
            angle = -angle;
        }
        const auto cosine = std::cos(angle);
        const auto sine = std::sin(angle);
        destination->set(source * cosine + unit_axis.cross(source) * sine +
                         unit_axis * (unit_axis.dot(source) * (1.0F - cosine)));
    }

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

    s32 getRandom(long min, long max) {
        if (min < std::numeric_limits<s32>::min() ||
            min > std::numeric_limits<s32>::max() ||
            max < std::numeric_limits<s32>::min() ||
            max > std::numeric_limits<s32>::max()) {
            throw std::out_of_range("A retail long random range must fit in s32.");
        }

        return getRandom(static_cast<s32>(min), static_cast<s32>(max));
    }

    f32 getInterpolateValue(f32 t, f32 start, f32 end) {
        return start + ((end - start) * t);
    }

    f32 getLinerValue(f32 x, f32 start, f32 end, f32 max) {
        return getInterpolateValue(x / max, start, end);
    }

    f32 getLinerValueFromMinMax(f32 x, f32 min, f32 max, f32 start, f32 end) {
        return getInterpolateValue(
            (JGeometry::TUtil<f32>::clamp(x, min, max) - min) / (max - min),
            start, end);
    }

    f32 getEaseInValue(f32 x, f32 start, f32 end, f32 max) {
        const auto rate = 1.0F - JMACosRadian(((x / max) * PI) / 2.0F);
        return getInterpolateValue(rate, start, end);
    }

    f32 getEaseOutValue(f32 x, f32 start, f32 end, f32 max) {
        const auto rate = JMASinRadian(((x / max) * PI) / 2.0F);
        return getInterpolateValue(rate, start, end);
    }

    f32 getEaseInOutValue(f32 x, f32 start, f32 end, f32 max) {
        const auto rate = (1.0F - JMACosRadian((x / max) * PI)) / 2.0F;
        return getInterpolateValue(rate, start, end);
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

    f32 normalize(f32 x, f32 min, f32 max) {
        const auto range = max - min;
        if (isNearZero(range)) {
            return x < min ? 0.0F : 1.0F;
        }

        return (JGeometry::TUtil<f32>::clamp(x, min, max) - min) / range;
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
