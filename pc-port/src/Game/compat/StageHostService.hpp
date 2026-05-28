#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <revolution.h>

#include "Game/compat/NameObjFactoryCompat.hpp"

namespace smgpc::game {

    class RuntimeContext;
    class SceneLifecycleService;

    struct StageHostRequest {
        std::string scene_name;
        std::string stage_name;
        std::string object_name;
        std::string actor_name;
        s32 scenario_no = 1;
        bool appear_after_init = false;
    };

    class StageHostService final {
    public:
        explicit StageHostService(RuntimeContext &runtime);
        ~StageHostService();

        StageHostService(const StageHostService &) = delete;
        StageHostService &operator=(const StageHostService &) = delete;

        void request_stage(const StageHostRequest &request);

        [[nodiscard]] bool has_active_stage(std::string_view stage_name) const;
        [[nodiscard]] std::string_view active_scene_name() const;
        [[nodiscard]] std::string_view active_stage_name() const;
        [[nodiscard]] s32 active_scenario_no() const;

    private:
        void create_stage_from_factory(const StageHostRequest &request);

        SceneLifecycleService &_scene_lifecycle;
    };

}  // namespace smgpc::game
