#include <aurora/exception.hpp>

struct JMapData;

#include "Game/Scene/GameSceneFunction.hpp"
#include "Game/Screen/MoviePlayingSequence.hpp"
#include "Game/Screen/StaffRoll.hpp"
#include "Game/System/GameDataConst.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameEventFlag.hpp"
#include "Game/System/GameEventFlagTable.hpp"
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
#include "compat/StageSessionState.hpp"
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

    [[nodiscard]] const GameEventFlag &require_retail_flag(std::string_view name) {
        if (name.empty()) {
            aurora::throw_host_exception<std::invalid_argument>("Game event flag name must not be empty");
        }

        for (auto index = s32{}; index < GameEventFlagTable::getTableSize(); ++index) {
            const auto *flag = GameEventFlagTable::getFlag(index);
            if (flag != nullptr && name == flag->mName) {
                return *flag;
            }
        }

        aurora::throw_host_exception<std::invalid_argument>("Game event flag is absent from the retail table: " + std::string(name));
    }

    [[nodiscard]] bool has_retail_special_star(const GameEventFlag &flag) {
        auto &holder = require_current_game_data();
        if (holder.calcCurrentPowerStarNum() == 0) {
            return false;
        }
        return holder.hasPowerStar(flag.mGalaxyName, flag.mStarID);
    }

    [[nodiscard]] bool is_retail_flag_on(const GameEventFlag &flag, unsigned depth);

    [[nodiscard]] bool can_turn_on_retail_flag(const GameEventFlag &flag, unsigned depth) {
        if (depth > 32U) {
            aurora::throw_host_exception<std::logic_error>("Retail game event flag dependency graph exceeded its recursion bound");
        }

        switch (flag.mType) {
        case GameEventFlag::Type_0:
            return true;
        case GameEventFlag::Type_1:
            return require_current_game_data().calcCurrentPowerStarNum() >= flag.mStarNum;
        case GameEventFlag::Type_SpecialStar:
            return has_retail_special_star(flag);
        case GameEventFlag::Type_4:
            return (flag.mRequirement1 == nullptr || is_retail_flag_on(require_retail_flag(flag.mRequirement1), depth + 1U)) &&
                   (flag.mRequirement2 == nullptr || is_retail_flag_on(require_retail_flag(flag.mRequirement2), depth + 1U));
        case GameEventFlag::Type_EventValueIsZero:
            if (flag.mRequirement == nullptr) {
                aurora::throw_host_exception<std::logic_error>("Retail event-value flag has no requirement: " + std::string(flag.mName));
            }
            return is_retail_flag_on(require_retail_flag(flag.mRequirement), depth + 1U) &&
                   require_current_game_data().getGameEventValue(flag.mEventValueName) == 0U;
        case GameEventFlag::Type_10:
            return require_current_game_data().isCompleteMarioAndLuigi();
        case GameEventFlag::Type_GalaxyOpenStar:
        case GameEventFlag::Type_5:
        case GameEventFlag::Type_Galaxy:
        case GameEventFlag::Type_Comet:
        case GameEventFlag::Type_StarPiece:
        case GameEventFlag::Type_11:
        default:
            unavailable("event-flag predicate for " + std::string(flag.mName));
        }
    }

    [[nodiscard]] bool is_retail_flag_on(const GameEventFlag &flag, unsigned depth) {
        if ((flag.mSaveFlag & 0x1U) != 0U) {
            return can_turn_on_retail_flag(flag, depth);
        }
        return require_current_game_data().isOnGameEventFlag(flag.mName);
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
    u32 getHashCode(const char *text) {
        if (text == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Hash input must not be null");
        }

        auto hash = u32{};
        for (; *text != '\0'; ++text) {
            hash = static_cast<u8>(*text) + hash * 31U;
        }
        return hash;
    }

    bool isExecScenarioStarter() {
        return smgpc::compat::require_active_stage_session().execution_phase() ==
               smgpc::compat::StageSessionState::ExecutionPhase::ScenarioStarter;
    }

    bool isStarCompleteAllGalaxy() {
        unavailable("all-galaxy Power Star completion state");
    }



    void requestChangeScene(const char *) {
        unavailable("story-driven scene request");
    }

    void requestChangeSceneTitle() {
        unavailable("title scene request");
    }


    void startMovieEpilogueA() {
        unavailable("epilogue movie playback");
    }

    void startMovieEndingA() {
        unavailable("ending A movie playback");
    }

    void startMovieEndingB() {
        unavailable("ending B movie playback");
    }

    bool isEndMovieEpilogueA() {
        unavailable("epilogue movie completion");
    }

    bool isEndMovieEndingA() {
        unavailable("ending A movie completion");
    }

    bool isEndMovieEndingB() {
        unavailable("ending B movie completion");
    }

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
