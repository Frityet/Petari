#include "compat/ActorMotionCompat.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "scene/StageCollisionService.hpp"

#include <cmath>
#include <stdexcept>

namespace {
    constexpr auto cWallDot = 0.34202015F;
    constexpr auto cDegreesToRadians = 3.14159265358979323846F / 180.0F;

    bool normalize(TVec3f& value) {
        const auto length_squared = value.dot(value);
        if (!(length_squared > 1.0e-12F) || !std::isfinite(length_squared)) {
            value.zero();
            return false;
        }
        value.scale(1.0F / std::sqrt(length_squared));
        return true;
    }

    TVec3f cross(const TVec3f& lhs, const TVec3f& rhs) {
        return TVec3f{lhs.y * rhs.z - lhs.z * rhs.y,
                      lhs.z * rhs.x - lhs.x * rhs.z,
                      lhs.x * rhs.y - lhs.y * rhs.x};
    }

    TVec3f rotation_up(const TVec3f& rotation) {
        const auto rx = rotation.x * cDegreesToRadians;
        const auto ry = rotation.y * cDegreesToRadians;
        const auto rz = rotation.z * cDegreesToRadians;
        const auto sx = std::sin(rx);
        const auto cx = std::cos(rx);
        const auto sy = std::sin(ry);
        const auto cy = std::cos(ry);
        const auto sz = std::sin(rz);
        const auto cz = std::cos(rz);
        return TVec3f{cz * sy * sx - sz * cx,
                      sz * sy * sx + cz * cx,
                      cy * sx};
    }

}  // namespace

namespace smgpc::compat {
    void update_live_actor_gravity(LiveActor& actor) {
        if (!actor.isDead() && actor.mFlag.mIsCalcGravity) {
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
        if (actor.isDead()) {
            return;
        }

        const auto has_binder = has_actor_binder(&actor);
        if (!has_binder || actor.mFlag.mIsNoBind) {
            actor.mPosition.add(actor.mVelocity);
            if (has_binder) {
                clear_actor_binder_contacts(&actor);
            }
            return;
        }

        clear_actor_binder_contacts(&actor);
        auto* collision = smgpc::scene::StageCollisionService::active();
        if (collision == nullptr || collision->empty()) {
            actor.mPosition.add(actor.mVelocity);
            return;
        }

        auto gravity = actor.mGravity;
        if (!normalize(gravity)) {
            throw std::logic_error("Binder contact classification requires a non-degenerate actor gravity vector.");
        }
        const auto& base_matrix = actor.getBaseMatrix().m;
        auto binder_up = TVec3f{base_matrix[1], base_matrix[5], base_matrix[9]};
        // The original model base matrix supplied to Binder contains the
        // actor basis but not model scale. The host render matrix is TRS, so
        // remove that host-only scale before applying Binder's offset.
        if (std::abs(actor.mScale.y) > 1.0e-8F) {
            binder_up.scale(1.0F / actor.mScale.y);
        } else {
            auto binder_side = TVec3f{base_matrix[0], base_matrix[4], base_matrix[8]};
            auto binder_front = TVec3f{base_matrix[2], base_matrix[6], base_matrix[10]};
            if (std::abs(actor.mScale.x) > 1.0e-8F) {
                binder_side.scale(1.0F / actor.mScale.x);
            }
            if (std::abs(actor.mScale.z) > 1.0e-8F) {
                binder_front.scale(1.0F / actor.mScale.z);
            }
            binder_up = cross(binder_front, binder_side);
        }
        if (!normalize(binder_up)) {
            binder_up = rotation_up(actor.mRotation);
            if (!normalize(binder_up)) {
                throw std::logic_error("Binder movement requires a finite, non-degenerate actor basis.");
            }
        }
        const auto binder_center = actor.mPosition + binder_up * actor.mBinderOffset;
        // LiveActor's third initBinder argument is Binder's stored-plane
        // capacity (_24), with zero selecting its temporary 32-plane array.
        const auto maximum_contacts = actor.mBinderType == 0U
                                          ? std::size_t{32U}
                                          : static_cast<std::size_t>(actor.mBinderType);
        const auto resolved = collision->move_sphere(binder_center, actor.mVelocity,
                                                     actor.mBinderRadius, maximum_contacts);
        actor.mPosition.add(resolved.displacement);

        auto contact_state = ActorBinderContactState{};
        contact_state.fix_reaction.set(resolved.fix_reaction);
        auto strongest_ground = -1.0F;
        auto strongest_wall = -1.0F;
        auto strongest_roof = -1.0F;
        for (const auto& contact : resolved.contacts) {
            const auto gravity_dot = contact.normal.dot(gravity);
            if (std::abs(gravity_dot) < cWallDot) {
                if (contact.penetration <= strongest_wall) {
                    continue;
                }
                contact_state.wall = true;
                contact_state.wall_normal.set(contact.normal);
                strongest_wall = contact.penetration;
            } else if (gravity_dot < 0.0F) {
                if (contact.penetration <= strongest_ground) {
                    continue;
                }
                contact_state.ground = true;
                contact_state.ground_normal.set(contact.normal);
                contact_state.ground_attribute = contact.attribute;
                strongest_ground = contact.penetration;
            } else if (contact.penetration > strongest_roof) {
                contact_state.roof = true;
                contact_state.roof_normal.set(contact.normal);
                strongest_roof = contact.penetration;
            }
        }
        record_actor_binder_contacts(&actor, contact_state);
    }
}  // namespace smgpc::compat
