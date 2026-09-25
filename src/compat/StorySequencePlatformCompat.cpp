#include <aurora/exception.hpp>

struct JMapData;

#include "Game/Scene/GameSceneFunction.hpp"
#include "Game/Screen/StaffRoll.hpp"
#include "Game/System/GameDataConst.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/System/GameSystemFunction.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/StageResultSequenceChecker.hpp"
#include "Game/System/StorySequenceExecutor.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    [[noreturn]] void unavailable(std::string_view operation) {
        aurora::throw_host_exception<std::runtime_error>("StorySequenceExecutor platform operation is unavailable: " + std::string(operation));
    }

    [[nodiscard]] GameDataHolder &require_current_game_data() {
        return *GameDataFunction::getCurrentGameDataHolder();
    }

}  // namespace

namespace GameDataFunction {
    bool isDataMario() {
        return require_current_game_data().isDataMario();
    }

    bool canOnGameEventFlag(const char *name) {
        if (name == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Game event flag query requires a name");
        }
        return require_current_game_data().canOnGameEventFlag(name);
    }

    bool canOnAndIsOffGameEventFlag(const char *name) {
        auto &holder = require_current_game_data();
        return holder.canOnGameEventFlag(name) && !holder.isOnGameEventFlag(name);
    }

    bool isOnJustGameEventFlag(const char *name) {
        return getCurrentGameDataHolder()->isOnGameEventFlag(name) &&
               !getSceneStartGameDataHolder()->isOnGameEventFlag(name);
    }

    bool canOnJustGameEventFlag(const char *name) {
        return getCurrentGameDataHolder()->canOnGameEventFlag(name) &&
               !getSceneStartGameDataHolder()->canOnGameEventFlag(name);
    }
}  // namespace GameDataFunction




const StorySequenceExecutorType::DemoSequenceInfo *StorySequenceExecutor::addDynamicDemoSequenceInfo(u16, u16, const char *) {
    unavailable("dynamic story demo sequence construction");
}

namespace MR {
    StaffRoll *getStaffRoll() {
        unavailable("staff-roll object lookup");
    }
}  // namespace MR

void StaffRoll::startInfo() {
    unavailable("staff-roll information sequence");
}

bool StaffRoll::isPauseOrEnd() const {
    unavailable("staff-roll pause/end state");
}
