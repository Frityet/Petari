#include "compat/ActorMotionCompat.hpp"

#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
namespace smgpc::compat {
    void integrate_live_actor_velocity(LiveActor &actor) {
        if (actor.mFlag.mIsDead) {
            return;
        }

        const auto has_binder = actor.mBinder != nullptr;
        if (!has_binder || actor.mFlag.mIsNoBind) {
            actor.mPosition.add(actor.mVelocity);
            if (has_binder) {
                actor.mBinder->clear();
            }
            return;
        }

        actor.mPosition.add(actor.mBinder->bind(actor.mVelocity));
    }
}  // namespace smgpc::compat
