#include "Game/Util/EventUtil.hpp"

#include "Game/System/GameDataFunction.hpp"
#include "Game/Util/PlayerUtil.hpp"

namespace MR {
    bool isOnGameEventFlagEndTicoGuideDemo() {
        return GameDataFunction::isPassedStoryEvent("チコガイドデモ終了");
    }

    void onGameEventFlagEndTicoGuideDemo() {
        GameDataFunction::followStoryEventByName("チコガイドデモ終了");
    }

    void onGameEventFlagEnableToSpinAndStarPointer() {
        GameDataFunction::followStoryEventByName("スピン権利");
        MR::setPlayerSwingPermission(true);
    }
}  // namespace MR
