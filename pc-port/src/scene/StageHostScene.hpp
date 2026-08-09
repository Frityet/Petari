#pragma once

#include "Game/Scene/Scene.hpp"
#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "camera/StageStartCamera.hpp"
#include "scene/AuthoredPlacementInstantiator.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "scene/StageCollisionService.hpp"
#include "scene/StageHostService.hpp"
#include "scene/StageAuthoredData.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

class NameObj;

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::compat {
    class DemoSceneRuntime;
    class StageSessionBinding;
    class StageSessionState;
}  // namespace smgpc::compat

namespace smgpc::scene {

    class SceneObjHolderBinding;
    class StageEventCameraBinding;
    class StageLightSceneBinding;
    struct NameObjPlacementContext;

    namespace nameobj {
        class ObjectNameTable;
        class PlanetMapCatalog;
    }  // namespace nameobj

    [[nodiscard]] bool should_apply_host_appear(const StagePlacementObject *placement, bool explicit_root = false);
    void preflight_stage_placements_or_throw(
        std::string_view stage_name, s32 scenario_no,
        std::span<const StagePlacementObject> placements,
        const StagePlacementObject *explicit_placement = nullptr);

    class StageHostScene final : public Scene {
    public:
        StageHostScene(smgpc::runtime::RuntimeContext &runtime, StageHostRequest request);
        ~StageHostScene() override;

        void init() override;
        void start() override;
        void update() override;
        void calcAnim() override;
        void draw3DNormal(const smgpc::camera::CameraPose &camera_pose);
        void draw2DNormal();

        [[nodiscard]] NameObj *root() const;
        [[nodiscard]] std::string_view scene_name() const;
        [[nodiscard]] std::string_view stage_name() const;
        [[nodiscard]] s32 scenario_no() const;

    private:
        void construct_root_object(std::string_view object_name,
                                   const char *actor_name,
                                   const NameObjPlacementContext *placement,
                                   bool apply_host_appear);
        void init_explicit_root();
        void init_placement_roots();
        void preflight_stage_start_or_throw() const;
        void construct_stage_start_root();
        void prepare_authored_placements(
            const StagePlacementObject *explicit_placement = nullptr);
        void preload_authored_placements();
        void construct_authored_placements();
        void init_stage_environment();
        void init_stage_audio();
        void trace_placement_object(const StagePlacementObject &placement) const;
        void init_roots_after_placement();
        void init_stage_start_camera();
        void appear_roots();
        void destroy_roots();
        [[nodiscard]] const char *resolve_actor_name(
            std::string_view object_name,
            const StagePlacementObject *placement) const;

        smgpc::runtime::RuntimeContext &_runtime;
        bool _initialized = false;
        std::size_t _registration_scope_id = 0U;
        StageHostRequest _request;
        // State precedes its binding so normal reverse member destruction also
        // closes the active-session scope before releasing the referenced state.
        std::unique_ptr<smgpc::compat::StageSessionState> _stage_session;
        std::unique_ptr<smgpc::compat::StageSessionBinding> _stage_session_binding;
        bool _stage_audio_started = false;
        StageCollisionService _collision;
        std::unique_ptr<SceneObjHolderBinding> _scene_obj_holder_binding;
        std::unique_ptr<StageLightSceneBinding> _stage_light_binding;
        std::unique_ptr<smgpc::scene::nameobj::PlanetMapCatalog> _planet_map_catalog;
        std::unique_ptr<smgpc::scene::nameobj::ObjectNameTable> _object_name_table;
        std::unique_ptr<smgpc::compat::DemoSceneRuntime> _demo_scene_runtime;
        std::unique_ptr<StageAuthoredData> _authored_data;
        std::unique_ptr<StageEventCameraBinding> _event_camera_binding;
        std::unique_ptr<AuthoredPlacementInstantiator> _authored_placements;
        std::vector<std::unique_ptr<NameObj>> _roots;
        std::vector<std::unique_ptr<NameObjChildOwner>>
            _root_registration_graphs;
        std::vector<bool> _root_host_appear;
        const StagePlacementObject *_explicit_placement_source = nullptr;
        NameObj *_explicit_placement_root = nullptr;
        std::optional<smgpc::camera::ResolvedStageStartCamera> _stage_start_camera;
    };

}  // namespace smgpc::scene
