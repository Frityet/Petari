#include "Game/Util/ActorSensorUtil.hpp"

namespace MR {
    bool isMsgAutoRushBegin(u32 msg) {
        return msg == ACTMES_AUTORUSH_BEGIN;
    }

    bool isMsgUpdateBaseMtx(u32 msg) {
        return msg == ACTMES_UPDATE_BASEMTX;
    }
}  // namespace MR
