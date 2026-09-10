#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/PlayerEvent.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/GameSceneLayoutHolder.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"

namespace GameDataFunction {

    void addMissPoint(int num) {
        getCurrentGameDataHolder()->addMissPoint(num);
    }

    void incPlayerMissNum() {
        getCurrentGameDataHolder()->incPlayerMissNum();
    }

}  // namespace GameDataFunction

namespace MR {

    void decPlayerLeft() {
        GameDataFunction::addPlayerLeft(-1);
        GameDataFunction::addMissPoint(1);
        GameDataFunction::incPlayerMissNum();
    }

    void startMissLayout() {
        getSceneObj< GameSceneLayoutHolder >(SceneObj_GameSceneLayoutHolder)->startMiss();
    }

    bool isMissLayoutAnimEnd() {
        return getSceneObj< GameSceneLayoutHolder >(SceneObj_GameSceneLayoutHolder)->isMissAnimEnd();
    }

}  // namespace MR
