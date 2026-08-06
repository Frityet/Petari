#include "compat/ActorRuntimeRegistry.hpp"

namespace smgpc::compat {
    void release_actor_runtime_state(const LiveActor* actor) {
        release_talk_runtime_state(actor);
        release_demo_runtime_state(actor);
        release_star_piece_runtime_state(actor);
    }
}  // namespace smgpc::compat
