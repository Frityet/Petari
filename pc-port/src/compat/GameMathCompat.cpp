#include "Game/Util/MathUtil.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace {
    auto sRandomSeed = std::uint32_t{0U};

    const std::array<f32, 256>& acos_table() {
        static const auto table = [] {
            auto values = std::array<f32, 256>{};
            for (auto index = std::size_t{}; index < values.size(); ++index) {
                const auto ratio = (static_cast<f64>(index) / 255.0) * (1.0 - 0.98) + 0.98;
                values[index] = static_cast<f32>(std::acos(std::min(ratio, 1.0)));
            }
            return values;
        }();
        return table;
    }
}  // namespace

namespace MR {
    bool isNan(const TVec3f& vector) {
        return std::isnan(vector.x) || std::isnan(vector.y) || std::isnan(vector.z);
    }

    void initAcosTable() {
        (void)acos_table();
    }

    f32 acosEx(f32 value) {
        if (std::fabs(value) < 0.98F) {
            return JMAAcosRadian(value);
        }
        // Avoid an undefined floating-to-integer conversion at the host
        // boundary; retail callers supply values in the arccosine domain.
        if (!std::isfinite(value) || value < -1.0F || value > 1.0F) {
            return std::numeric_limits<f32>::quiet_NaN();
        }
        const auto index = static_cast<u32>((std::fabs(value) - 0.98F) * 50.0F * 255.0F);
        const auto angle = acos_table()[index];
        return value < 0.0F ? pi() - angle : angle;
    }

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
        const auto cosine = left.dot(right) / (left_length * right_length);
        if (cosine >= 1.0F) {
            return 0.0F;
        }
        if (cosine <= -1.0F) {
            return PI;
        }
        return acosEx(cosine);
    }

    void PSvecBlend(const register TVec3f* a1, const register TVec3f* a2, register TVec3f* a3, register f32 a4, register f32 a5) {
#ifdef __MWERKS__
        __asm {
            psq_l     f0, 0(a1), 0, 0
            psq_l     f3, 8(a1), 1, 0
            ps_muls0  f4, f0, a4
            psq_l     f0, 0(a2), 0, 0
            ps_muls0  f3, f3, a4
            psq_l     f1, 8(a2), 1, 0
            ps_madds0 f4, f0, f2, f4
            ps_madds0 f3, f1, f2, f3
            psq_st    f4, 0(a3), 0, 0
            psq_st    f3, 8(a3), 1, 0
        }
#else
        const f32 fromX = a1->x;
        const f32 fromY = a1->y;
        const f32 fromZ = a1->z;
        const f32 toX = a2->x;
        const f32 toY = a2->y;
        const f32 toZ = a2->z;
        const f32 scaledX = fromX * a4;
        const f32 scaledY = fromY * a4;
        const f32 scaledZ = fromZ * a4;
        a3->x = std::fma(toX, a5, scaledX);
        a3->y = std::fma(toY, a5, scaledY);
        a3->z = std::fma(toZ, a5, scaledZ);
#endif
    }

    void vecBlend(const TVec3f& rFrom, const TVec3f& rTo, TVec3f* pDst, f32 rate) {
        PSvecBlend(&rFrom, &rTo, pDst, 1.0f - rate, rate);
    }

    void vecBlendNormal(const TVec3f& from, const TVec3f& to, TVec3f* destination, f32 rate) {
        auto left = TVec3f{};
        if (!isNearZero(from)) {
            left.set(from);
            PSVECNormalize(left, left);
            PSVECScale(left, left, 1.0F - rate);
        }

        auto right = TVec3f{};
        if (!isNearZero(to)) {
            right.set(to);
            PSVECNormalize(right, right);
            PSVECScale(right, right, rate);
        }
        PSVECAdd(left, right, destination);
    }

    bool vecBlendSphere(const TVec3f& from, const TVec3f& to, TVec3f* destination, f32 rate) {
        const auto from_length = from.length();
        const auto to_length = to.length();
        const auto angle = from_length == 0.0F || to_length == 0.0F ? 0.0F : diffAngleAbs(from, to);
        const auto length = from_length * (1.0F - rate) + to_length * rate;
        if (angle < 0.1F) {
            vecBlendNormal(from, to, destination, rate);
            destination->setLength(length);
            return true;
        }
        if (angle == 0.0F) {
            destination->set(from);
            destination->setLength(length);
            return false;
        }
        if (angle == pi()) {
            return false;
        }

        destination->set((from * JMASinRadian(angle * (1.0F - rate)) + to * JMASinRadian(angle * rate)) /
                         JMASinRadian(angle));
        destination->setLength(length);
        return true;
    }

    void vecRotAxis(const TVec3f& source, const TVec3f& target, const TVec3f& axis,
                    TVec3f* destination, f32 angle) {
        const auto source_length = source.length();
        const auto target_length = target.length();
        const auto difference = source_length == 0.0F || target_length == 0.0F ? 0.0F : diffAngleAbs(source, target);
        if (difference == 0.0F) {
            destination->set(target);
            return;
        }

        if (difference > angle) {
            if (difference < pi() && source.cross(target).dot(axis) < 0.0F) {
                angle = -angle;
            }
            auto rotation = TPos3f{};
            PSMTXRotAxisRad(rotation.toMtxPtr(), axis, angle);
            PSMTXMultVec(rotation.toMtxPtr(), source, destination);
            return;
        }
        destination->set(target);
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

    void turnRandomVector(TVec3f* destination, const TVec3f& source, f32 range) {
        const auto length = source.length();
        addRandomVector(destination, source, range);
        if (isNearZero(*destination)) {
            destination->set(source);
        } else {
            destination->setLength(length);
        }
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
        blendQuatUpFront(pQuat, *pQuat, rUp, rFront, upRate, frontRate);
    }

    void blendQuatUpFront(TQuat4f* destination, const TQuat4f& source, const TVec3f& target_up,
                         const TVec3f& target_front, f32 up_rate, f32 front_rate) {
        auto quaternion = source;
        auto up = TVec3f{};
        quaternion.getYDir(up);
        if (!(up.dot(target_up) >= 0.0F) && isSameDirection(up, target_up)) {
            turnRandomVector(&up, up, 0.001F);
        }

        auto up_rotation = TQuat4f{};
        up_rotation.setRotate(up, target_up, up_rate);
        quaternion.mult(up_rotation);

        quaternion.getYDir(up);
        auto front = TVec3f{};
        quaternion.getZDir(front);
        auto projected_front = target_front.killElement(up);
        normalizeOrZero(&projected_front);
        if (!(front.dot(projected_front) >= 0.0F) && isSameDirection(front, projected_front)) {
            turnRandomVector(&front, front, 0.001F);
        }

        auto front_rotation = TQuat4f{};
        front_rotation.setRotate(front, projected_front, front_rate);
        quaternion.mult(front_rotation);
        quaternion.normalize();
        destination->set(quaternion);
    }
}  // namespace MR

const Vec gZeroVec = {0.0F, 0.0F, 0.0F};
