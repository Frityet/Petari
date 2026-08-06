#include "Game/Util/ActorSensorUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"

namespace MR {
    HitSensor* addHitSensorNpc(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& rOffset) {
        return pActor != nullptr ? pActor->addHitSensor(pName, ATYPE_NPC, groupSize, radius, rOffset) : nullptr;
    }
}  // namespace MR
