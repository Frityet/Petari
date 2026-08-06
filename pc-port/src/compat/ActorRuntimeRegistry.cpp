#include "compat/ActorRuntimeRegistry.hpp"

#include <unordered_set>

namespace {
    auto& actor_binders() {
        static auto actors = std::unordered_set<const LiveActor*>{};
        return actors;
    }
}  // namespace

namespace smgpc::compat {
    void register_actor_binder(const LiveActor* actor) {
        if (actor != nullptr) {
            actor_binders().insert(actor);
        }
    }

    bool has_actor_binder(const LiveActor* actor) {
        return actor != nullptr && actor_binders().contains(actor);
    }

    void release_actor_binder_state(const LiveActor* actor) {
        actor_binders().erase(actor);
    }

    void release_actor_runtime_state(const LiveActor* actor) {
        release_actor_binder_state(actor);
        release_talk_runtime_state(actor);
        release_demo_runtime_state(actor);
        release_star_piece_runtime_state(actor);
    }
}  // namespace smgpc::compat
