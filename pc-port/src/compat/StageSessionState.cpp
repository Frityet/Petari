#include "compat/StageSessionState.hpp"

#include <exception>
#include <stdexcept>
#include <utility>

namespace {
    thread_local smgpc::compat::StageSessionBinding *s_active_binding = nullptr;
    thread_local smgpc::compat::StageSessionState *s_active_session = nullptr;
}  // namespace

namespace smgpc::compat {

    StageSessionState::StageSessionState(std::string_view scene_name, std::string_view stage_name, s32 scenario_no,
                                         const JMapIdInfo &initial_start_id, StageScenarioMetadata metadata)
        : _scene_name(scene_name), _stage_name(stage_name), _scenario_no(scenario_no), _initial_start_id(initial_start_id),
          _restart_id(initial_start_id), _metadata(std::move(metadata)) {
        if (_scene_name.empty()) {
            throw std::invalid_argument("A stage session requires a scene name.");
        }
        if (_stage_name.empty()) {
            throw std::invalid_argument("A stage session requires a stage name.");
        }
        if (_scenario_no <= 0) {
            throw std::invalid_argument("A stage session requires a positive scenario number.");
        }
    }

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
        return _restart_id;
    }

    const JMapIdInfo &StageSessionState::restart_id() const {
        return _restart_id;
    }

    void StageSessionState::set_restart_id(const JMapIdInfo &restart_id) {
        _restart_id = restart_id;
    }

    const StageScenarioMetadata &StageSessionState::metadata() const {
        return _metadata;
    }

    void StageSessionState::set_metadata(StageScenarioMetadata metadata) {
        _metadata = std::move(metadata);
    }

    bool StageSessionState::is_power_star_get_demo_active() const {
        return _power_star_get_demo_active;
    }

    void StageSessionState::set_power_star_get_demo_active(bool active) {
        _power_star_get_demo_active = active;
    }

    StageSessionBinding::StageSessionBinding(StageSessionState &session)
        : _previous(s_active_binding), _session(&session) {
        s_active_binding = this;
        s_active_session = _session;
    }

    StageSessionBinding::~StageSessionBinding() {
        if (s_active_binding != this) {
            std::terminate();
        }
        s_active_binding = _previous;
        s_active_session = _previous != nullptr ? _previous->_session : nullptr;
    }

    StageSessionState *try_active_stage_session() {
        return s_active_session;
    }

    StageSessionState &require_active_stage_session() {
        auto *session = try_active_stage_session();
        if (session == nullptr) {
            throw std::logic_error("Stage-session state is unavailable outside an active stage lifetime.");
        }
        return *session;
    }

}  // namespace smgpc::compat
