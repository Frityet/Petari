#pragma once

#include "Game/Scene/Scene.hpp"
#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "scene/StageHostService.hpp"

#include <memory>
#include <string_view>
#include <vector>

class NameObj;

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    struct StagePlacementObject;

    class StageHostScene final : public Scene {
    public:
        StageHostScene(smgpc::runtime::RuntimeContext &runtime, StageHostRequest request);
        ~StageHostScene() override;

        void init() override;
        void start() override;
        void update() override;
        void calcAnim() override;
        void draw3DNormal(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose);
        void draw2DNormal(render::AuroraRenderer &renderer);

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
        void appear_roots();
        void destroy_roots();
        [[nodiscard]] std::string resolve_actor_name(std::string_view object_name) const;

        smgpc::runtime::RuntimeContext &_runtime;
        StageHostRequest _request;
        std::vector<std::unique_ptr<NameObj>> _roots;
    };

}  // namespace smgpc::scene
