#include "Game/Player/MarioAccess.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/ScenePlayingResult.hpp"
#include "Game/Screen/GameSceneLayoutHolder.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameDataTemporaryInGalaxy.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

#include <aurora/wpad.hpp>
#include "compat/StageSessionState.hpp"

namespace {
    ScenePlayingResult* getScenePlayingResult() {
        return MR::getSceneObj<ScenePlayingResult>(SceneObj_ScenePlayingResult);
    }

    GameDataTemporaryInGalaxy* getGameDataTemporaryInGalaxy() {
        return &smgpc::compat::require_active_stage_session().temporary_data();
    }
}

namespace GameDataFunction {
    s32 getPlayerLeft() {
        return getCurrentGameDataHolder()->getPlayerLeft();
    }

    void addPlayerLeft(int num) {
        getCurrentGameDataHolder()->addPlayerLeft(num);
    }

    s32 getStockedStarPieceNum() {
        return getCurrentGameDataHolder()->getStockedStarPieceNum();
    }

    s32 getStarPieceNum() {
        return ::getGameDataTemporaryInGalaxy()->getStarPieceNum();
    }

    void addStarPiece(int num) {
        ::getGameDataTemporaryInGalaxy()->addStarPiece(num);
    }

    void setLast1upStarPieceNum(int num) {
        ::getGameDataTemporaryInGalaxy()->setLast1upStarPieceNum(num);
    }

    s32 getLast1upStarPieceNum() {
        return ::getGameDataTemporaryInGalaxy()->mLast1upStarPieceNum;
    }
}

namespace MR {
    void requestOneUp() {
        startSystemSE("SE_SY_1UP");
        getSceneObj<GameSceneLayoutHolder>(SceneObj_GameSceneLayoutHolder)->requestOneUp(1);
    }

    void requestPowerUpHPMeter() {
        getSceneObj<GameSceneLayoutHolder>(SceneObj_GameSceneLayoutHolder)->requestPowerUpHPMeter();
    }

    s32 getPlayerLeft() {
        return GameDataFunction::getPlayerLeft();
    }

    void incPlayerLeft() {
        GameDataFunction::addPlayerLeft(1);
    }

    s32 getCoinNum() {
        return ::getScenePlayingResult()->getCoinNum();
    }

    s32 getPurpleCoinNum() {
        return ::getScenePlayingResult()->mPurpleCoinNum;
    }

    s32 getStarPieceNum() {
        if (MR::isStageAstroLocation()) {
            return GameDataFunction::getStockedStarPieceNum();
        }
        return ::getScenePlayingResult()->getStarPieceNum();
    }

    s32 getPowerStarNum() {
        return GameDataFunction::calcCurrentPowerStarNum();
    }

    bool isStageAstroLocation() {
        return isEqualStageName("AstroGalaxy") || isEqualStageName("AstroDome") || isEqualStageName("LibraryRoom");
    }

    bool isPlayerSwimming() {
        return MarioAccess::isSwimming();
    }

    bool isSubPadSwing(s32 channel) {
        return aurora::wpad_service().is_sub_swing(channel);
    }
}
