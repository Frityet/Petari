#include "Game/compat/SceneLifecycleService.hpp"

#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/compat/RuntimeContext.hpp"
#include "Game/compat/StageHostScene.hpp"

namespace smgpc::game {

    SceneLifecycleService::SceneLifecycleService(RuntimeContext &runtime) : _runtime(runtime) {
    }

    SceneLifecycleService::~SceneLifecycleService() = default;

    void SceneLifecycleService::request_stage(const StageHostRequest &request) {
        if (has_active_stage(request.stage_name)) {
            return;
        }

        create_stage_scene(request);
    }

    void SceneLifecycleService::destroy_scene() {
        _active_scene.reset();
        _active_scene_name.clear();
        _active_stage_name.clear();
        _active_scenario_no = 0;
    }

    void SceneLifecycleService::start_scene() {
        if (_active_scene != nullptr) {
            _active_scene->start();
        }
    }

    void SceneLifecycleService::update_scene() {
        if (_active_scene != nullptr) {
            _active_scene->update();
        }
    }

    void SceneLifecycleService::calc_anim_scene() {
        if (_active_scene != nullptr) {
            _active_scene->calcAnim();
        }
    }

    void SceneLifecycleService::draw_3d_normal(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose) {
        if (_active_scene != nullptr) {
            _active_scene->draw3DNormal(renderer, camera_pose);
        }
    }

    void SceneLifecycleService::draw_2d_normal(render::IRendererEngine &renderer) {
        if (_active_scene != nullptr) {
            _active_scene->draw2DNormal(renderer);
        }
    }

    Scene* SceneLifecycleService::active_scene() const {
        return _active_scene.get();
    }

    NameObj* SceneLifecycleService::active_root() const {
        return _active_scene != nullptr ? _active_scene->root() : nullptr;
    }

    bool SceneLifecycleService::has_active_stage(std::string_view stage_name) const {
        return _active_scene != nullptr && _active_stage_name == stage_name;
    }

    std::string_view SceneLifecycleService::active_scene_name() const {
        return _active_scene_name;
    }

    std::string_view SceneLifecycleService::active_stage_name() const {
        return _active_stage_name;
    }

    s32 SceneLifecycleService::active_scenario_no() const {
        return _active_scenario_no;
    }

    void SceneLifecycleService::create_stage_scene(const StageHostRequest &request) {
        const auto object_name = !request.object_name.empty() ? request.object_name : request.stage_name;

        destroy_scene();
        _active_scene_name = request.scene_name;
        _active_stage_name = request.stage_name;
        _active_scenario_no = request.scenario_no;
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "temporary_stage_host_started",
                                           "stage host factory created " + object_name + " until GameScene placement is available");
        _runtime.emit_sequence_state_trace_event("temporary_stage_host_started", "host=" + object_name + ";stage=" + request.stage_name);
#endif
        auto scene = std::make_unique<StageHostScene>(_runtime, request);
        scene->init();
        _active_scene = std::move(scene);
        start_scene();
    }

}  // namespace smgpc::game
