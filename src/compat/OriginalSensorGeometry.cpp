#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Util/ActorMovementUtil.hpp"

namespace MR {
    // Original ActorMovementUtil body; sensor positions are world coordinates.
    f32 calcDistance(const HitSensor* pSensor1, const HitSensor* pSensor2, TVec3f* a3) {
        TVec3f sensor2_pos = pSensor2->mPosition - pSensor1->mPosition;

        f32 mag = sensor2_pos.length();
        if (a3 != nullptr) {
            if (mag > 0.0f) {
                a3->scale(1.0f / mag, sensor2_pos);
            } else {
                a3->zero();
            }
        }

        return mag;
    }
}
