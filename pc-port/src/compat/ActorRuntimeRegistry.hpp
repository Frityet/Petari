#pragma once

#include <cstddef>

class LiveActor;

namespace smgpc::compat {
    void release_talk_runtime_state(const LiveActor* actor);
    void release_demo_runtime_state(const LiveActor* actor);
    void release_actor_runtime_state(const LiveActor* actor);

    [[nodiscard]] bool has_owned_talk_ctrl(const LiveActor* actor);
    [[nodiscard]] bool has_registered_demo_cast(const LiveActor* actor);
    [[nodiscard]] std::size_t registered_demo_action_count(const LiveActor* actor);
}  // namespace smgpc::compat
