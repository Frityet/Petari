#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace MR {
    HitSensor* addHitSensorEye(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& rOffset) {
        return pActor != nullptr ? pActor->addHitSensor(pName, ATYPE_EYE, groupSize, radius, rOffset) : nullptr;
    }

    bool sendArbitraryMsg(u32 msg, HitSensor* pReceiver, HitSensor* pSender) {
        return pReceiver != nullptr && pReceiver->receiveMessage(msg, pSender);
    }

    void setClippingFar50m(LiveActor*) {
        // The host scheduler currently keeps all registered actors active. This
        // remains a general clipping-policy boundary until host culling lands.
    }
}  // namespace MR
