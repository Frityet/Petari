#pragma once

#include "scene/SceneTransitionRequestService.hpp"
#include "scene/StageHostService.hpp"

#include <string>

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    class SequenceBootService final {
    public:
        SequenceBootService(smgpc::runtime::RuntimeContext &runtime, SceneTransitionRequestService &scene_transitions,
                            StageHostService &stage_host);
        ~SequenceBootService();

        SequenceBootService(const SequenceBootService &) = delete;
        SequenceBootService &operator=(const SequenceBootService &) = delete;

        void request_boot_to_initial_stage();
        void update_after_runtime_frame();

        [[nodiscard]] bool is_boot_requested() const;
        [[nodiscard]] bool is_initial_stage_host_active() const;

    private:
        void update_stage_transition_requests();

        smgpc::runtime::RuntimeContext &_runtime;
        SceneTransitionRequestService &_scene_transitions;
        StageHostService &_stage_host;
        std::string _boot_stage_name;
        bool _boot_requested = false;
    };

}  // namespace smgpc::scene
