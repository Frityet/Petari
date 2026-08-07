#pragma once

#include <cstddef>

class LiveActor;
class NameObj;
class TalkMessageCtrl;

namespace smgpc::render::live_actor {
    class LiveActorModel;
}

namespace smgpc::compat {
    void register_actor_binder(const LiveActor *actor);
    [[nodiscard]] bool has_actor_binder(const LiveActor *actor);
    void release_actor_binder_state(const LiveActor *actor);
    void register_actor_model(const LiveActor *actor, smgpc::render::live_actor::LiveActorModel *model);
    [[nodiscard]] smgpc::render::live_actor::LiveActorModel *actor_model(const LiveActor *actor);
    void release_actor_model_state(const LiveActor *actor);

    void release_talk_runtime_state(const LiveActor *actor);
    [[nodiscard]] TalkMessageCtrl *owned_talk_ctrl(const LiveActor *actor);
    void release_demo_runtime_state(const LiveActor *actor);
    void release_star_piece_runtime_state(const LiveActor *actor);
    void release_actor_runtime_state(const LiveActor *actor);

    [[nodiscard]] bool has_owned_talk_ctrl(const LiveActor *actor);
    [[nodiscard]] bool has_registered_demo_cast(const LiveActor *actor);
    [[nodiscard]] std::size_t registered_demo_membership_count(const LiveActor *actor);
    [[nodiscard]] std::size_t registered_demo_action_count(const LiveActor *actor);
    [[nodiscard]] std::size_t declared_star_piece_count(const NameObj *owner);
}  // namespace smgpc::compat
