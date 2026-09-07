#include "Game/Util/MathUtil.hpp"

// Original segment/sphere broad-phase used by CollisionCategorizedKeeper.
namespace MR {
    bool checkHitSegmentSphere(const TVec3f& rSpherePos, const TVec3f& rPointA, const TVec3f& rPointB, f32 radius, TVec3f* pDir) {
        TVec3f pos = rSpherePos;
        TVec3f segment = rPointB;
        pos -= rPointA;
        segment -= rPointA;

        f32 dot = pos.dot(segment);
        f32 radSquared = radius * radius;
        if (dot < 0.0f) {
            if (rPointA.squared(rSpherePos) < radSquared) {
                if (pDir != nullptr) {
                    pDir->set(rSpherePos - rPointA);
                    MR::normalizeOrZero(pDir);
                }
                return true;
            }
        } else if (segment.squared() < dot) {
            if (rPointB.squared(rSpherePos) < radSquared) {
                if (pDir != nullptr) {
                    pDir->set(rSpherePos - rPointB);
                    MR::normalizeOrZero(pDir);
                }
                return true;
            }
        } else {
            TVec3f offset = segment;
            offset.scale(dot / segment.squared());
            offset.sub(pos);
            if (offset.squared() <= radSquared) {
                if (pDir != nullptr) {
                    pDir->set(-offset);
                    MR::normalizeOrZero(pDir);
                }
                return true;
            }
        }

        return false;
    }
}
