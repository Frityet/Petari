#include "Game/NPC/TrickRabbit.hpp"
#include "Game/Util/FootPrint.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace TrickRabbitUtil {
    FootPrint* createRabbitFootPrint(LiveActor* pActor) {
        FootPrint* pFootPrint = new FootPrint("ウサギ足跡", 0x40);
        pFootPrint->_38 = 100.0f;
        pFootPrint->setTexture(MR::getTexFromArc("RabbitFootprint.bti", pActor));
        pFootPrint->_2C = 0.0f;
        pFootPrint->_30 = 30.0f;
        pFootPrint->_34 = 30.0f;
        return pFootPrint;
    }
}  // namespace TrickRabbitUtil
