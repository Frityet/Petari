#include "Game/compat/StageHostScene.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/compat/NameObjFactoryCompat.hpp"
#include "Game/compat/RuntimeContext.hpp"
#include "Game/compat/StagePlacementResolver.hpp"

#include <stdexcept>

namespace smgpc::game {

    StageHostScene::StageHostScene(RuntimeContext &runtime, StageHostRequest request)
        : Scene(!request.stage_name.empty() ? request.stage_name.c_str() : "StageHostScene"), _runtime(runtime), _request(std::move(request)) {
    }

    StageHostScene::~StageHostScene() {
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
    }

    void StageHostScene::init_root_object(std::string_view object_name, std::string_view actor_name, const StagePlacementObject* placement) {
        if (!can_create_name_obj(object_name)) {
            throw std::runtime_error("Unsupported stage host request: " + _request.stage_name);
        }

        auto root = create_name_obj(object_name, actor_name);
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "stage_host_constructed",
                                           "host=" + std::string(object_name) + ";stage=" + _request.stage_name);
#endif
        if (placement != nullptr) {
            const auto placement_iter = JMapInfoIter(&placement->jmap_info, placement->jmap_entry_index);
            root->init(placement_iter);
        } else {
            root->initWithoutIter();
        }
        if (_request.appear_after_init) {
            if (auto *layout_actor = dynamic_cast<LayoutActor *>(root.get())) {
                layout_actor->appear();
            } else if (auto *live_actor = dynamic_cast<LiveActor *>(root.get())) {
                live_actor->appear();
            }
        }
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "stage_host_initialized",
                                           "host=" + std::string(object_name) + ";stage=" + _request.stage_name);
#endif
        _roots.push_back(std::move(root));
    }

    void StageHostScene::init_explicit_root() {
        init_root_object(_request.object_name, resolve_actor_name(_request.object_name), nullptr);
    }

    void StageHostScene::init_placement_roots() {
        const auto placements = resolve_stage_root_placements(_runtime.dvd(), _request.stage_name, _request.scenario_no);
        for (const auto &placement : placements) {
            init_root_object(placement.object_name, resolve_actor_name(placement.object_name), &placement);
        }

        if (_roots.empty()) {
            init_root_object(_request.stage_name, resolve_actor_name(_request.stage_name), nullptr);
        }
    }

    void StageHostScene::start() {
    }

    void StageHostScene::update() {
        _runtime.scheduler().execute_movement();
    }

    void StageHostScene::calcAnim() {
        _runtime.scheduler().execute_calc_anim();
        _runtime.scheduler().execute_calc_view_and_entry();
    }

    void StageHostScene::draw3DNormal(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose) {
        _runtime.scheduler().execute_draw_buffer_list_normal(renderer, camera_pose);
        _runtime.scheduler().execute_draw_type(renderer, MR::DrawType_EffectDraw3D);
        _runtime.scheduler().execute_draw_type(renderer, MR::DrawType_EffectDrawForBloomEffect);
        _runtime.scheduler().execute_draw_type(renderer, MR::DrawType_CaptureScreenIndirect);
    }

    void StageHostScene::draw2DNormal(render::IRendererEngine &renderer) {
        _runtime.scheduler().execute_draw_list_2d_normal(renderer);
    }

    NameObj* StageHostScene::root() const {
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

}  // namespace smgpc::game
