#include "compat/StarPointerDepthOwnership.hpp"
#include <aurora/exception.hpp>
#include "compat/StageSessionState.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/AlreadyDoneFlagInGalaxy.hpp"
#include "Game/System/GameDataTemporaryInGalaxy.hpp"

#include <exception>
#include <stdexcept>
#include <utility>

namespace {
    thread_local smgpc::compat::StageSessionBinding *s_active_binding = nullptr;
    thread_local smgpc::compat::StageSessionState *s_active_session = nullptr;

    std::string host_string(std::string_view text) {
        smgpc::compat::JkrHostAllocationScope host;
        return std::string(text);
    }
}  // namespace

namespace smgpc::compat {
    struct StageSessionState::TemporaryData {
        // The host wrapper retains any original domain through typed teardown.
        std::shared_ptr<JkrAllocationDomain> domain = current_jkr_allocation_domain();
        std::unique_ptr<GameDataTemporaryInGalaxy> value;

        TemporaryData() {
            if (domain) {
                JkrAllocationScope game(domain);
                value = std::make_unique<GameDataTemporaryInGalaxy>();
            } else {
                value = std::make_unique<GameDataTemporaryInGalaxy>();
            }
        }

        ~TemporaryData() {
            delete value->mAlreadyDoneFlag;
            delete value->mPlayerRestartIdInfo;
        }
    };

    StageSessionState::StageSessionState(std::string_view scene_name, std::string_view stage_name, s32 scenario_no,
                                         const JMapIdInfo &initial_start_id, StageScenarioMetadata metadata)
        : _scene_name(host_string(scene_name)), _stage_name(host_string(stage_name)), _scenario_no(scenario_no), _initial_start_id(initial_start_id),
          _metadata(std::move(metadata)) {
        if (_scene_name.empty()) {
            aurora::throw_host_exception<std::invalid_argument>("A stage session requires a scene name.");
        }
        if (_stage_name.empty()) {
            aurora::throw_host_exception<std::invalid_argument>("A stage session requires a stage name.");
        }
        if (_scenario_no <= 0) {
            aurora::throw_host_exception<std::invalid_argument>("A stage session requires a positive scenario number.");
        }
        // Original temporary data starts with SceneUtil's constant (0, 0)
        // restart key; the selected scene-entry start ID is separate state.
        JkrHostAllocationScope host;
        _temporary = std::make_unique<TemporaryData>();
    }

    StageSessionState::~StageSessionState() = default;

    const std::string &StageSessionState::scene_name() const {
        return _scene_name;
    }

    const std::string &StageSessionState::stage_name() const {
        return _stage_name;
    }

    s32 StageSessionState::scenario_no() const {
        return _scenario_no;
    }

    const JMapIdInfo &StageSessionState::initial_start_id() const {
        return _initial_start_id;
    }

    JMapIdInfo &StageSessionState::restart_id() {
        return *temporary_data().mPlayerRestartIdInfo;
    }

    const JMapIdInfo &StageSessionState::restart_id() const {
        return *temporary_data().mPlayerRestartIdInfo;
    }

    void StageSessionState::set_restart_id(const JMapIdInfo &restart_id) {
        temporary_data().setPlayerRestartIdInfo(restart_id);
    }

    GameDataTemporaryInGalaxy &StageSessionState::temporary_data() {
        return *_temporary->value;
    }

    const GameDataTemporaryInGalaxy &StageSessionState::temporary_data() const {
        return *_temporary->value;
    }

    const StageScenarioMetadata &StageSessionState::metadata() const {
        return _metadata;
    }

    void StageSessionState::set_metadata(StageScenarioMetadata metadata) {
        _metadata = std::move(metadata);
    }

    StageSessionState::ExecutionPhase StageSessionState::execution_phase() const {
        return _execution_phase;
    }

    void StageSessionState::set_execution_phase(ExecutionPhase phase) {
        _execution_phase = phase;
    }

    StageSessionBinding::StageSessionBinding(StageSessionState &session)
        : _previous(s_active_binding), _session(&session) {
        if (try_star_pointer_depth()) _pointer_scene = std::make_unique<StarPointerSceneBinding>();
        s_active_binding = this;
        s_active_session = _session;
    }

    StageSessionBinding::~StageSessionBinding() {
        if (s_active_binding != this) {
            std::terminate();
        }
        _pointer_scene.reset();
        s_active_binding = _previous;
        s_active_session = _previous != nullptr ? _previous->_session : nullptr;
    }

    StageSessionState *try_active_stage_session() {
        return s_active_session;
    }

    StageSessionState &require_active_stage_session() {
        auto *session = try_active_stage_session();
        if (session == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Stage-session state is unavailable outside an active stage lifetime.");
        }
        return *session;
    }

}  // namespace smgpc::compat
