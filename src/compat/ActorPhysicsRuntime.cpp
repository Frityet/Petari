#include "compat/ActorPhysicsRuntime.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ClippingJudge.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

namespace smgpc::compat {
    void update_actor_clipping(LiveActor& actor, const ClippingJudge& judge) {
        if (!actor_is_clipping_target(&actor)) return;
        const auto* state = actor_clipping_runtime_state(&actor);
        if (state == nullptr || !state->sphere_configured || actor.mFlag.mIsInvalidClipping) {
            if (actor.mFlag.mIsInvalidClipping && actor.mFlag.mIsClipped) {
                actor.endClipped();
            }
            return;
        }
        const auto& center = state->sphere_center != nullptr ? *state->sphere_center : actor.mPosition;
        const auto clipped = judge.isJudgedToClipFrustum(center, state->sphere_radius, state->far_level.value_or(0));
        if (clipped && !actor.mFlag.mIsClipped) {
            actor.startClipped();
        } else if (!clipped && actor.mFlag.mIsClipped) {
            actor.endClipped();
        }
    }

}  // namespace smgpc::compat
