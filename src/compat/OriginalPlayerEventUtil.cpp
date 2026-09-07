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


    void startPlayerEvent(const char* pName) {
        EventSequencer* eventSequencer;

        eventSequencer = MR::getSceneObj< EventSequencer >(SceneObj_EventSequencer);
        eventSequencer->startEvent(pName);

        requestMovementOn(eventSequencer);
        requestMovementOnPlayer();
    }

    void requestMovementOnPlayer() {
        requestMovementOn(MarioAccess::getPlayerActor());
    }

    void startBckPlayerJ(const char* pName) {
        MarioAccess::changeAnimationJ(pName);
    }

    void startSoundPlayerJ(const char* pName) {
        MarioAccess::getPlayerActor()->playSound(pName, -1);
    }

    void setPlayerSpot(f32 param1, u32 param2) {
        MarioAccess::setSpot(param1, param2);
    }

    void startPlayerDownWipe() {
        MarioAccess::startDownWipe();
    }

    void startMissLayout() {
        getSceneObj< GameSceneLayoutHolder >(SceneObj_GameSceneLayoutHolder)->startMiss();
    }

    bool isMissLayoutAnimEnd() {
        return getSceneObj< GameSceneLayoutHolder >(SceneObj_GameSceneLayoutHolder)->isMissAnimEnd();
    }

}  // namespace MR
