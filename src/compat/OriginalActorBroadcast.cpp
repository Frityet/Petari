#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace MR {

    void sendMsgToAllLiveActor(u32 msg, LiveActor* pActor) {
        AllLiveActorGroup* pGroup = getAllLiveActorGroup();

        for (int i = 0; i < pGroup->getObjNum(); i++) {
            LiveActor* pGroupActor = pGroup->getActor(i);

            if (isDead(pGroupActor)) {
                continue;
            }

            if (pGroupActor == pActor) {
                continue;
            }

            pGroupActor->receiveMessage(msg, getMessageSensor(), getMessageSensor());
        }
    }

}  // namespace MR
