#pragma once

#include "Game/Scene/Scene.hpp"
#include "camera/CameraPose.hpp"
#include "scene/StageHostService.hpp"

#include <memory>
#include <string_view>

class NameObj;
namespace smgpc::runtime {
    class RuntimeContext;
}

namespace smgpc::scene {
    class StageInitializationService;

    class StageHostScene final : public Scene {
    public:
        StageHostScene(smgpc::runtime::RuntimeContext &runtime, StageHostRequest request);
        ~StageHostScene() override;
        void bind_initialization(StageInitializationService &initialization);
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
        smgpc::runtime::RuntimeContext &_runtime;
        StageInitializationService *_initialization = nullptr;
    };
}  // namespace smgpc::scene
