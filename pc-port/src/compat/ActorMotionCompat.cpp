#include "compat/ActorMotionCompat.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "scene/StageCollisionService.hpp"

#include <cmath>

namespace {
    constexpr auto cWallDot = 0.34202015F;

    bool normalize(TVec3f& value) {
        const auto length_squared = value.dot(value);
        if (!(length_squared > 1.0e-12F) || !std::isfinite(length_squared)) {
            value.zero();
            return false;
        }
        value.scale(1.0F / std::sqrt(length_squared));
        return true;
    }

    void clear_bind_state(LiveActor& actor) {
        actor.mBindedGround = false;
        actor.mBindedWall = false;
        actor.mBindedRoof = false;
        actor.mBindedGroundDamageFire = false;
    }
}  // namespace

namespace smgpc::compat {
    void update_live_actor_gravity(LiveActor& actor) {
        if (!actor.isDead() && actor.mFlag.mIsCalcGravity) {
            MR::calcGravityOrZero(&actor);
        }
    }

    void integrate_live_actor_velocity(LiveActor &actor) {
        if (actor.isDead()) {
            return;
        }

        const auto has_binder = actor.mBinderRadius > 0.0F || actor.mBinderOffset > 0.0F;
        if (!has_binder || actor.mFlag.mIsNoBind) {
            actor.mPosition.add(actor.mVelocity);
            if (has_binder) {
                clear_bind_state(actor);
            }
            return;
        }

        clear_bind_state(actor);
        auto* collision = smgpc::scene::StageCollisionService::active();
        if (collision == nullptr || collision->empty()) {
            actor.mPosition.add(actor.mVelocity);
            return;
        }

        auto gravity = actor.mGravity;
        if (!normalize(gravity)) {
            gravity.set(0.0F, -1.0F, 0.0F);
        }
        const auto binder_center = actor.mPosition - gravity * actor.mBinderOffset;
        const auto resolved = collision->move_sphere(binder_center, actor.mVelocity, actor.mBinderRadius);
        actor.mPosition.add(resolved.displacement);

        auto strongest_ground = -1.0F;
        auto strongest_wall = -1.0F;
        auto strongest_roof = -1.0F;
        for (const auto& contact : resolved.contacts) {
            const auto gravity_dot = contact.normal.dot(gravity);
            if (gravity_dot < -cWallDot && contact.penetration > strongest_ground) {
                actor.mBindedGround = true;
                actor.mGroundNormal.set(contact.normal);
                strongest_ground = contact.penetration;
            } else if (gravity_dot > cWallDot && contact.penetration > strongest_roof) {
                actor.mBindedRoof = true;
                actor.mRoofNormal.set(contact.normal);
                strongest_roof = contact.penetration;
            } else if (contact.penetration > strongest_wall) {
                actor.mBindedWall = true;
                actor.mWallNormal.set(contact.normal);
                strongest_wall = contact.penetration;
            }
        }

        // This mirrors calcGravityOrZero's original no-field fallback: a
        // grounded actor can derive a stable local gravity from its floor.
        if (actor.mFlag.mIsCalcGravity && actor.mBindedGround) {
            actor.mGravity.set(-actor.mGroundNormal);
            (void)normalize(actor.mGravity);
        }
    }
}  // namespace smgpc::compat
