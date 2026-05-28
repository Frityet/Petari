#include "Game/compat/StageHostService.hpp"

#include "Game/compat/GameSystemSceneControllerService.hpp"

namespace smgpc::game {

    StageHostService::StageHostService(GameSystemSceneControllerService &scene_controller) : _scene_controller(scene_controller) {
    }

    StageHostService::~StageHostService() = default;

    void StageHostService::request_stage(const StageHostRequest &request) {
        create_stage_from_factory(request);
    }

    void StageHostService::update_scene_requests() {
        _scene_controller.check_request_and_change_scene();
    }

    bool StageHostService::has_active_stage(std::string_view stage_name) const {
        return _scene_controller.has_active_stage(stage_name);
    }

    std::string_view StageHostService::active_scene_name() const {
        return _scene_controller.active_scene_name();
    }

    std::string_view StageHostService::active_stage_name() const {
        return _scene_controller.active_stage_name();
    }

    s32 StageHostService::active_scenario_no() const {
        return _scene_controller.active_scenario_no();
    }

    void StageHostService::create_stage_from_factory(const StageHostRequest &request) {
        _scene_controller.request_change_scene(request);
    }

}  // namespace smgpc::game
