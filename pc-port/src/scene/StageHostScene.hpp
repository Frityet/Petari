#pragma once

#include "Game/Scene/Scene.hpp"
#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "camera/StageStartCamera.hpp"
#include "scene/StageCollisionService.hpp"
#include "scene/StageGravityService.hpp"
#include "scene/StageHostService.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

class NameObj;

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    [[nodiscard]] bool should_apply_host_appear(const StagePlacementObject *placement);

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
        void construct_root_object(std::string_view object_name, std::string_view actor_name, const StagePlacementObject *placement);
        void init_explicit_root();
        void init_placement_roots();
        void trace_placement_object(const StagePlacementObject &placement) const;
        void init_roots_after_placement();
        void init_stage_start_camera();
        void appear_roots();
        void destroy_roots();
        [[nodiscard]] std::string resolve_actor_name(std::string_view object_name) const;

        smgpc::runtime::RuntimeContext &_runtime;
        std::size_t _registration_scope_id = 0U;
        StageHostRequest _request;
        StageCollisionService _collision;
        StageGravityService _gravity;
        std::vector<StagePlacementObject> _placements;
        std::vector<std::unique_ptr<NameObj>> _roots;
        std::vector<const StagePlacementObject *> _root_placements;
        std::optional<smgpc::camera::ResolvedStageStartCamera> _stage_start_camera;
    };

}  // namespace smgpc::scene
