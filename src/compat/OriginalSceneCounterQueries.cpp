#include "Game/Player/MarioAccess.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/ScenePlayingResult.hpp"
#include "Game/Screen/GameSceneLayoutHolder.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSequenceDirector.hpp"
#include "Game/System/GameDataTemporaryInGalaxy.hpp"
#include "Game/Util/SingletonHolder.hpp"

#include "compat/StageSessionState.hpp"

namespace {
    GameDataTemporaryInGalaxy* getGameDataTemporaryInGalaxy() {
        if (auto* system = SingletonHolder<GameSystem>::get()) {
            return system->mSequenceDirector->mGameDataTemporaryInGalaxy;
        }
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

    s32 setupAlreadyDoneFlag(const char* pName, const JMapInfoIter& rIter, u32* pValue) {
        return ::getGameDataTemporaryInGalaxy()->setupAlreadyDoneFlag(pName, rIter, pValue);
    }

    void updateAlreadyDoneFlag(int index, u32 value) {
        ::getGameDataTemporaryInGalaxy()->updateAlreadyDoneFlag(index, value);
    }
}
