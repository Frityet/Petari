#include "compat/ActorMotionCompat.hpp"

#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "scene/StageCollisionService.hpp"

#include <cmath>
#include <stdexcept>

namespace {
    bool normalize(TVec3f& value) {
        const auto length_squared = value.dot(value);
        if (!(length_squared > 1.0e-12F) || !std::isfinite(length_squared)) {
            value.zero();
            return false;
        }
        value.scale(1.0F / std::sqrt(length_squared));
        return true;
    }

}  // namespace

namespace smgpc::compat {
    void update_live_actor_gravity(LiveActor& actor) {
        if (!actor.mFlag.mIsDead && actor.mFlag.mIsCalcGravity) {
            // LiveActor::movement calls calcGravity, whose no-field behavior
            // keeps the actor's previous gravity. calcGravityOrZero is a
            // separate opt-in helper used by actors such as Coin.
            auto gravity = TVec3f{};
            if (MR::calcGravityVector(&actor, &gravity, nullptr, 0U) && normalize(gravity)) {
                actor.mGravity.set(gravity);
            }
        }
    }

    void integrate_live_actor_velocity(LiveActor &actor) {
        if (actor.mFlag.mIsDead) {
            return;
        }

        const auto has_binder = actor.mBinder != nullptr;
        if (!has_binder || actor.mFlag.mIsNoBind) {
            actor.mPosition.add(actor.mVelocity);
            if (has_binder) {
                actor.mBinder->clear();
                clear_actor_binder_contacts(&actor);
            }
            return;
        }

        clear_actor_binder_contacts(&actor);
        actor.mPosition.add(actor.mBinder->bind(actor.mVelocity));

        auto contact_state = ActorBinderContactState{};
        contact_state.fix_reaction.set(actor.mBinder->mFixReactionVector);
        if (actor.mBinder->isBindedGround()) {
            contact_state.ground = true;
            contact_state.ground_normal.set(*actor.mBinder->mGroundInfo.mParentTriangle.getNormal(0));
            if (auto* collision = smgpc::scene::StageCollisionService::active()) {
                if (const auto surface = collision->surface(
                        actor.mBinder->mGroundInfo.mParentTriangle.mIdx)) {
                    contact_state.ground_attribute = surface->attribute;
                }
            }
        }
        if (actor.mBinder->isBindedWall()) {
            contact_state.wall = true;
            contact_state.wall_normal.set(*actor.mBinder->mWallInfo.mParentTriangle.getNormal(0));
        }
        if (actor.mBinder->isBindedRoof()) {
            contact_state.roof = true;
            contact_state.roof_normal.set(*actor.mBinder->mRoofInfo.mParentTriangle.getNormal(0));
        }
        record_actor_binder_contacts(&actor, contact_state);
    }
}  // namespace smgpc::compat
