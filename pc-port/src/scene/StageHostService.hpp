#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <revolution.h>

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    class GameSystemSceneControllerService;

    struct StageHostRequest {
        std::string scene_name;
        std::string stage_name;
        std::string object_name;
        std::string actor_name;
        s32 scenario_no = 1;
        bool appear_after_init = false;
        bool fail_unsupported_placement = false;
    };

    class StageHostService final {
    public:
        explicit StageHostService(GameSystemSceneControllerService &scene_controller);
        ~StageHostService();

        StageHostService(const StageHostService &) = delete;
        StageHostService &operator=(const StageHostService &) = delete;

        void request_stage(const StageHostRequest &request);
        void update_scene_requests();

        [[nodiscard]] bool has_active_stage(std::string_view stage_name) const;
        [[nodiscard]] std::string_view active_scene_name() const;
        [[nodiscard]] std::string_view active_stage_name() const;
        [[nodiscard]] s32 active_scenario_no() const;

    private:
        void create_stage_from_factory(const StageHostRequest &request);

        GameSystemSceneControllerService &_scene_controller;
    };

}  // namespace smgpc::scene
