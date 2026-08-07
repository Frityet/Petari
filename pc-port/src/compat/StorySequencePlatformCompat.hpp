#pragma once

#include <string>
#include <string_view>

#include <revolution/types.h>

namespace smgpc::compat::story_sequence {
    class SceneStateBinding final {
    public:
        SceneStateBinding(std::string_view scene_name, std::string_view stage_name, s32 scenario_no);
        ~SceneStateBinding();

        SceneStateBinding(const SceneStateBinding&) = delete;
        SceneStateBinding& operator=(const SceneStateBinding&) = delete;

        [[nodiscard]] const std::string& scene_name() const;
        [[nodiscard]] const std::string& stage_name() const;
        [[nodiscard]] s32 scenario_no() const;

    private:
        SceneStateBinding* _previous = nullptr;
        std::string _scene_name;
        std::string _stage_name;
        s32 _scenario_no = 0;
    };

    [[nodiscard]] const SceneStateBinding& require_scene_state();
    [[nodiscard]] bool is_comet_scheduler_active();
    void reset_comet_scheduler_state_for_test();
}  // namespace smgpc::compat::story_sequence
