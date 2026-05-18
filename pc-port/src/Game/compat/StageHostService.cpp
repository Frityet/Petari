#include "Game/compat/StageHostService.hpp"

#include "Game/compat/RuntimeContext.hpp"
#include "Game/compat/SceneLifecycleService.hpp"

namespace smgpc::game {

    StageHostService::StageHostService(RuntimeContext &runtime) : _scene_lifecycle(runtime.scene_lifecycle()) {
    }

    StageHostService::~StageHostService() = default;

    void StageHostService::request_stage(const StageHostRequest &request) {
        create_stage_from_factory(request);
    }

    bool StageHostService::has_active_stage(std::string_view stage_name) const {
        return _scene_lifecycle.has_active_stage(stage_name);
    }

    std::string_view StageHostService::active_scene_name() const {
        return _scene_lifecycle.active_scene_name();
    }

    std::string_view StageHostService::active_stage_name() const {
        return _scene_lifecycle.active_stage_name();
    }

    s32 StageHostService::active_scenario_no() const {
        return _scene_lifecycle.active_scenario_no();
    }

#ifndef NDEBUG
    std::optional<FileSelectStageState> StageHostService::file_select_state() const {
        return _scene_lifecycle.file_select_state();
    }
#endif

    void StageHostService::create_stage_from_factory(const StageHostRequest &request) {
        _scene_lifecycle.request_stage(request);
    }

}  // namespace smgpc::game
