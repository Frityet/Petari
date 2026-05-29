#include "scene/StageHostScene.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/SceneExecutionService.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <stdexcept>

namespace smgpc::scene {

    StageHostScene::StageHostScene(smgpc::runtime::RuntimeContext &runtime, StageHostRequest request)
        : Scene(!request.stage_name.empty() ? request.stage_name.c_str() : "StageHostScene"), _runtime(runtime), _request(std::move(request)) {
    }

    StageHostScene::~StageHostScene() {
        destroy_roots();
        if (MR::getSceneObjHolder() == mSceneObjHolder) {
            MR::setCurrentSceneObjHolder(nullptr);
        }
    }

    void StageHostScene::init() {
        if (!_roots.empty()) {
            return;
        }

        initSceneObjHolder();
        MR::setCurrentSceneObjHolder(mSceneObjHolder);

        if (!_request.object_name.empty()) {
            init_explicit_root();
        } else {
            init_placement_roots();
        }

        init_roots_after_placement();
        appear_roots();
    }

    void StageHostScene::construct_root_object(std::string_view object_name, std::string_view actor_name, const StagePlacementObject *placement) {
        if (!smgpc::scene::nameobj::can_create_name_obj(object_name)) {
            throw std::runtime_error("Unsupported stage host request: " + _request.stage_name);
        }

        auto &lifecycle = _runtime.name_obj_lifecycle();
        lifecycle.preload_archives(object_name);
        auto root = lifecycle.construct(object_name, actor_name);
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "stage_host_constructed",
                                           "host=" + std::string(object_name) + ";stage=" + _request.stage_name);
#endif
        lifecycle.init(*root, placement);
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "stage_host_initialized",
                                           "host=" + std::string(object_name) + ";stage=" + _request.stage_name);
#endif
        _roots.push_back(std::move(root));
    }

    void StageHostScene::init_explicit_root() {
        construct_root_object(_request.object_name, resolve_actor_name(_request.object_name), nullptr);
    }

    void StageHostScene::init_placement_roots() {
        const auto placements = resolve_stage_placement_objects(_runtime.dvd(), _request.stage_name, _request.scenario_no);
        for (const auto &placement : placements) {
            trace_placement_object(placement);
            if (!placement.factory_supported) {
                continue;
            }
            construct_root_object(placement.object_name, resolve_actor_name(placement.object_name), &placement);
        }

        if (_roots.empty()) {
            construct_root_object(_request.stage_name, resolve_actor_name(_request.stage_name), nullptr);
        }
    }

    void StageHostScene::trace_placement_object(const StagePlacementObject &placement) const {
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("placement", "stage_object",
                                           "stage=" + placement.stage_name + ";zone=" + placement.zone_name +
                                               ";scenario=" + std::to_string(_request.scenario_no) +
                                               ";layer=" + placement.layer_name + ";table=" + placement.table_path +
                                               ";object=" + placement.object_name +
                                               ";factory=" + (placement.factory_supported ? std::string("supported") : std::string("unsupported")) +
                                               ";object_archive=" + placement.object_archive_path);
#else
        (void)placement;
#endif
    }

    void StageHostScene::init_roots_after_placement() {
        auto &lifecycle = _runtime.name_obj_lifecycle();
        for (auto &root : _roots) {
            lifecycle.init_after_placement(*root);
        }
    }

    void StageHostScene::appear_roots() {
        if (!_request.appear_after_init) {
            return;
        }

        auto &lifecycle = _runtime.name_obj_lifecycle();
        for (auto &root : _roots) {
            lifecycle.appear(*root);
        }
    }

    void StageHostScene::destroy_roots() {
        auto &lifecycle = _runtime.name_obj_lifecycle();
        for (auto &root : _roots) {
            lifecycle.destroy(*root);
        }
        _roots.clear();
    }

    void StageHostScene::start() {
    }

    void StageHostScene::update() {
        _runtime.scene_execution().execute_movement();
    }

    void StageHostScene::calcAnim() {
        _runtime.scene_execution().execute_calc_anim_and_view();
    }

    void StageHostScene::draw3DNormal(const smgpc::camera::CameraPose &camera_pose) {
        _runtime.scene_execution().draw_3d_normal(camera_pose);
    }

    void StageHostScene::draw2DNormal() {
        _runtime.scene_execution().draw_2d_normal();
    }

    NameObj *StageHostScene::root() const {
        return !_roots.empty() ? _roots.front().get() : nullptr;
    }

    std::string_view StageHostScene::scene_name() const {
        return _request.scene_name;
    }

    std::string_view StageHostScene::stage_name() const {
        return _request.stage_name;
    }

    s32 StageHostScene::scenario_no() const {
        return _request.scenario_no;
    }

    std::string StageHostScene::resolve_actor_name(std::string_view object_name) const {
        return !_request.actor_name.empty() ? _request.actor_name : std::string(object_name);
    }

}  // namespace smgpc::scene
