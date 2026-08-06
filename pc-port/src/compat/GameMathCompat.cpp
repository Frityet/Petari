#include "Game/Util/MathUtil.hpp"

#include <cmath>

namespace {
    TQuat4f quaternion_from_axes(const TVec3f& side, const TVec3f& up, const TVec3f& front) {
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
    f32 vecKillElement(const TVec3f& rVector, const TVec3f& rDirection, TVec3f* pOut) {
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

    void blendQuatUpFront(TQuat4f* pQuat, const TVec3f& rUp, const TVec3f& rFront, f32 upRate, f32 frontRate) {
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
