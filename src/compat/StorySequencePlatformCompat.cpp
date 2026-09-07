#include "compat/StorySequencePlatformCompat.hpp"

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
    bool s_comet_scheduler_active = false;
    thread_local smgpc::compat::story_sequence::SceneStateBinding *s_scene_state = nullptr;

    [[noreturn]] void unavailable(std::string_view operation) {
        throw std::runtime_error("StorySequenceExecutor platform operation is unavailable: " + std::string(operation));
    }

    [[nodiscard]] UserFile &require_current_user_file() {
        auto *file = smgpc::game::save_data_handle_sequence().getCurrentUserFile();
        if (file == nullptr || file->mGameDataHolder == nullptr) {
            unavailable("current user file");
        }
        return *file;
    }

    [[nodiscard]] GameDataHolder &require_current_game_data() {
        return *require_current_user_file().mGameDataHolder;
    }

    [[nodiscard]] const GameEventFlag &require_retail_flag(std::string_view name) {
        if (name.empty()) {
            throw std::invalid_argument("Game event flag name must not be empty");
        }

        for (auto index = s32{}; index < GameEventFlagTable::getTableSize(); ++index) {
            const auto *flag = GameEventFlagTable::getFlag(index);
            if (flag != nullptr && name == flag->mName) {
                return *flag;
            }
        }

        throw std::invalid_argument("Game event flag is absent from the retail table: " + std::string(name));
    }

    [[nodiscard]] bool has_retail_special_star(const GameEventFlag &flag) {
        auto &holder = require_current_game_data();
        // Zero aggregate stars proves that no per-galaxy star can be owned.
        // A positive aggregate without the per-star bit is incomplete input,
        // not evidence that this particular star is absent.
        if (holder.calcCurrentPowerStarNum() == 0) {
            return false;
        }

        unavailable("per-galaxy Power Star ownership for " + std::string(flag.mName));
    }

    [[nodiscard]] bool is_retail_flag_on(const GameEventFlag &flag, unsigned depth);

    [[nodiscard]] bool can_turn_on_retail_flag(const GameEventFlag &flag, unsigned depth) {
        if (depth > 32U) {
            throw std::logic_error("Retail game event flag dependency graph exceeded its recursion bound");
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
                throw std::logic_error("Retail event-value flag has no requirement: " + std::string(flag.mName));
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

    [[nodiscard]] const GameEventFlag &require_special_star_flag(const char *galaxy_name, s32 star_id) {
        if (galaxy_name == nullptr) {
            throw std::invalid_argument("Power Star query requires a galaxy name");
        }

        for (auto index = s32{}; index < GameEventFlagTable::getTableSize(); ++index) {
            const auto *flag = GameEventFlagTable::getFlag(index);
            if (flag != nullptr && flag->mType == GameEventFlag::Type_SpecialStar && flag->mGalaxyName != nullptr &&
                std::string_view(flag->mGalaxyName) == galaxy_name && flag->mStarID == star_id) {
                return *flag;
            }
        }

        unavailable("retail Power Star mapping for " + std::string(galaxy_name) + ":" + std::to_string(star_id));
    }
}  // namespace

namespace smgpc::compat::story_sequence {
    SceneStateBinding::SceneStateBinding(std::string_view scene_name, std::string_view stage_name, s32 scenario_no)
        : _previous(s_scene_state), _scene_name(scene_name), _stage_name(stage_name), _scenario_no(scenario_no) {
        if (_scene_name.empty()) {
            throw std::invalid_argument("Story sequence scene state requires a scene name");
        }
        s_scene_state = this;
    }

    SceneStateBinding::~SceneStateBinding() {
        if (s_scene_state != this) {
            std::terminate();
        }
        s_scene_state = _previous;
    }

    const std::string &SceneStateBinding::scene_name() const {
        return _scene_name;
    }

    const std::string &SceneStateBinding::stage_name() const {
        return _stage_name;
    }

    s32 SceneStateBinding::scenario_no() const {
        return _scenario_no;
    }

    const SceneStateBinding &require_scene_state() {
        if (s_scene_state == nullptr) {
            unavailable("bound scene state");
        }
        return *s_scene_state;
    }

    bool is_comet_scheduler_active() {
        return s_comet_scheduler_active;
    }

    void reset_comet_scheduler_state_for_test() {
        s_comet_scheduler_active = false;
    }
}  // namespace smgpc::compat::story_sequence

namespace GameDataFunction {
    bool isDataMario() {
        return require_current_game_data().isDataMario();
    }

    bool hasPowerStar(const char *galaxy_name, s32 star_id) {
        return has_retail_special_star(require_special_star_flag(galaxy_name, star_id));
    }

    bool hasGrandStar(int index) {
        char name[32];
        std::snprintf(name, sizeof(name), "SpecialStarGrand%1d", index);
        const auto &flag = require_retail_flag(name);
        if (flag.mType != GameEventFlag::Type_SpecialStar) {
            throw std::logic_error("Retail Grand Star flag is not a special-star predicate");
        }
        return has_retail_special_star(flag);
    }

    bool canOnGameEventFlag(const char *name) {
        if (name == nullptr) {
            throw std::invalid_argument("Game event flag query requires a name");
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

namespace GameDataConst {
    bool isGrandStar(const char *, s32) {
        unavailable("GalaxyID Grand Star classification");
    }

    u32 getIncludedGrandGalaxyId(const char *) {
        unavailable("GalaxyID included Grand Galaxy lookup");
    }
}  // namespace GameDataConst

namespace GameSequenceFunction {
    void activateGalaxyCometScheduler() {
        s_comet_scheduler_active = true;
    }

    void deactivateGalaxyCometScheduler() {
        s_comet_scheduler_active = false;
    }

    bool hasStageResultSequence() {
        unavailable("stage-result sequence state");
    }

    const char *getClearedStageName() {
        unavailable("cleared stage name");
    }

    s32 getClearedPowerStarId() {
        unavailable("cleared Power Star ID");
    }

    bool hasPowerStarYetAtResultSequence() {
        unavailable("stage-result Power Star state");
    }

    bool isLuigiDisappearFromAstroGalaxy() {
        unavailable("Luigi hide-and-seek sequence state");
    }
}  // namespace GameSequenceFunction

namespace GameSceneFunction {
    void requestStaffRoll() {
        unavailable("staff-roll scene request");
    }
}  // namespace GameSceneFunction

namespace GameSystemFunction {
    bool setPermissionToCheckWiiRemoteConnectAndScreenDimming(bool) {
        unavailable("Wii Remote connection and screen-dimming permission");
    }
}  // namespace GameSystemFunction

StageResultSequenceChecker::StageResultSequenceChecker() {
    unavailable("stage-result sequence checker");
}

void StageResultSequenceChecker::check() {
    unavailable("stage-result sequence checker");
}

bool StageResultSequenceChecker::isJustGetGreenStarFirst() const {
    unavailable("stage-result green-star snapshot");
}

const StorySequenceExecutorType::DemoSequenceInfo *StorySequenceExecutor::addDynamicDemoSequenceInfo(u16, u16, const char *) {
    unavailable("dynamic story demo sequence construction");
}

namespace MR {
    u32 getHashCode(const char *text) {
        if (text == nullptr) {
            throw std::invalid_argument("Hash input must not be null");
        }

        auto hash = u32{};
        for (; *text != '\0'; ++text) {
            hash = static_cast<u8>(*text) + hash * 31U;
        }
        return hash;
    }

    const char *getCurrentStageName() {
        if (const auto *session = smgpc::compat::try_active_stage_session(); session != nullptr) {
            return session->stage_name().c_str();
        }
        return smgpc::compat::story_sequence::require_scene_state().stage_name().c_str();
    }

    s32 getCurrentScenarioNo() {
        if (const auto *session = smgpc::compat::try_active_stage_session(); session != nullptr) {
            return session->scenario_no();
        }
        return smgpc::compat::story_sequence::require_scene_state().scenario_no();
    }

    bool isEqualSceneName(const char *name) {
        if (name == nullptr) {
            return false;
        }
        if (const auto *session = smgpc::compat::try_active_stage_session(); session != nullptr) {
            return MR::isEqualStringCase(session->scene_name().c_str(), name);
        }
        return MR::isEqualStringCase(smgpc::compat::story_sequence::require_scene_state().scene_name().c_str(), name);
    }

    bool isEqualStageName(const char *name) {
        return name != nullptr && MR::isEqualStringCase(getCurrentStageName(), name);
    }

    const JMapIdInfo &getInitializeStartIdInfo() {
        return smgpc::compat::require_active_stage_session().initial_start_id();
    }

    bool isStarCompleteAllGalaxy() {
        unavailable("all-galaxy Power Star completion state");
    }

    void startStarPointerModeEnding(void *) {
        unavailable("ending Star Pointer mode");
    }

    void requestChangeScene(const char *) {
        unavailable("story-driven scene request");
    }

    void requestChangeSceneTitle() {
        unavailable("title scene request");
    }

    bool tryStartDemoWithoutCinemaFrameValidHandPointerFinger(NameObj *, const char *) {
        unavailable("save demo without cinema frame");
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
