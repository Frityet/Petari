#include "Game/Util/SystemUtil.hpp"
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

namespace MR {

    JMapIdInfo *getPlayerRestartIdInfo() {
        return ::getGameDataTemporaryInGalaxy()->mPlayerRestartIdInfo;
    }

    void setPlayerRestartIdInfo(const JMapIdInfo &restart_id) {
        ::getGameDataTemporaryInGalaxy()->setPlayerRestartIdInfo(restart_id);
    }

}  // namespace MR
