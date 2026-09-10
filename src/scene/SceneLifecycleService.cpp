#include "scene/SceneLifecycleService.hpp"

#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/Scene.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/RuntimeContext.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "scene/GameSceneBinding.hpp"
#include "compat/SceneJ3dScope.hpp"
#include "scene/StageInitializationService.hpp"

#include <string>

namespace smgpc::scene {

    SceneLifecycleService::SceneLifecycleService(smgpc::runtime::RuntimeContext &runtime) : _runtime(runtime) {
    }

    SceneLifecycleService::~SceneLifecycleService() {
        destroy_scene();
    }

    void SceneLifecycleService::request_stage(const StageHostRequest &request) {
        create_stage_scene(request);
    }

    void SceneLifecycleService::destroy_scene() {
#ifndef NDEBUG
        const auto scene_name = _active_scene_name;
        const auto stage_name = _active_stage_name;
        const auto scenario_no = _active_scenario_no;
        const auto before_entry_count = _runtime.scheduler().snapshot().size();
#endif
        if (_active_game_scene_binding)
            _active_game_scene_binding->prepare_retirement();
        _active_scene.reset();
        _active_game_scene_binding.reset();
        _active_initialization.reset();
        _active_scene_domain.reset();
        _active_scene_name.clear();
        _active_stage_name.clear();
        _active_scenario_no = 0;
#ifndef NDEBUG
        const auto after_entry_count = _runtime.scheduler().snapshot().size();
        const auto removed_entry_count = before_entry_count >= after_entry_count ? before_entry_count - after_entry_count : 0U;
        if (!scene_name.empty() || !stage_name.empty() || before_entry_count != after_entry_count) {
            _runtime.emit_semantic_trace_event("scene_lifecycle", "stage_destroy_scheduler_cleanup",
                                               "scene=" + scene_name + ";stage=" + stage_name +
                                                   ";scenario=" + std::to_string(scenario_no) +
                                                   ";before_entries=" + std::to_string(before_entry_count) +
                                                   ";after_entries=" + std::to_string(after_entry_count) +
                                                   ";removed_entries=" + std::to_string(removed_entry_count));
        }
#endif
    }

    void SceneLifecycleService::start_scene() {
        if (_active_scene != nullptr) {
            const smgpc::compat::JkrAllocationScope game(_active_scene_domain);
            _active_scene->start();
        }
    }

    void SceneLifecycleService::update_scene() {
        if (_active_scene != nullptr) {
            const smgpc::compat::JkrAllocationScope game(_active_scene_domain);
            const smgpc::compat::SceneJ3dScope commands;
            _runtime.scheduler().begin_frame();
            _active_scene->update();
        }
    }

    void SceneLifecycleService::calc_anim_scene() {
        if (_active_scene != nullptr) {
            const smgpc::compat::JkrAllocationScope game(_active_scene_domain);
            const smgpc::compat::SceneJ3dScope commands;
            _active_scene->calcAnim();
        }
    }

    void SceneLifecycleService::draw_scene() {
        if (_active_scene) {
            const smgpc::compat::JkrAllocationScope game(_active_scene_domain);
            const smgpc::compat::SceneJ3dScope commands;
            _active_scene->draw();
        }
    }

    Scene *SceneLifecycleService::active_scene() const {
        return _active_scene.get();
    }

    NameObj *SceneLifecycleService::active_root() const {
        return _active_initialization != nullptr ? _active_initialization->root() : nullptr;
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
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        auto domain = smgpc::compat::JkrAllocationDomain::create(_runtime.host_heaps(), 8U * 1024U * 1024U);
        std::unique_ptr<StageInitializationService> initialization;
        std::unique_ptr<GameScene> scene;
        {
            const smgpc::compat::JkrAllocationScope game(domain);
            scene = std::make_unique<GameScene>();
        }
        initialization = std::make_unique<StageInitializationService>(_runtime, *scene, request, domain);
        _active_scene_domain = std::move(domain);
        _active_initialization = std::move(initialization);
        _active_scene = std::move(scene);
        try {
            _active_game_scene_binding = std::make_unique<GameSceneBinding>(*_active_scene);
            _active_scene_name = request.scene_name;
            _active_stage_name = request.stage_name;
            _active_scenario_no = request.scenario_no;
            _active_initialization->pre_scene_init();
            {
                const smgpc::compat::JkrAllocationScope game(_active_scene_domain);
                _active_scene->init();
            }
            SceneFunction::allocateDrawBufferActorList();
            _active_initialization->finalize_scene_initialization();
#ifndef NDEBUG
            _runtime.emit_semantic_trace_event("sequence", "stage_host_started",
                                               "original GameScene created " + object_name + " through scene lifecycle service");
            _runtime.emit_sequence_state_trace_event("stage_host_started", "host=" + object_name + ";stage=" + request.stage_name);
#endif
            start_scene();
        } catch (...) {
            destroy_scene();
            throw;
        }
    }

}  // namespace smgpc::scene
