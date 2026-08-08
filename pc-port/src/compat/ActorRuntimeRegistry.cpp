#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/GameActorSensorCompat.hpp"
#include "compat/MaterialCtrlCompat.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include <cmath>
#include <stdexcept>
#include <unordered_map>

namespace {
    auto& actor_binders() {
        static auto actors = std::unordered_map<const LiveActor*, smgpc::compat::ActorBinderContactState>{};
        return actors;
    }

    auto& actor_models() {
        static auto models = std::unordered_map<const LiveActor*, smgpc::render::live_actor::LiveActorModel*>{};
        return models;
    }

    auto& actor_clipping_states() {
        static auto states = std::unordered_map<const LiveActor*, smgpc::compat::ActorClippingRuntimeState>{};
        return states;
    }
}  // namespace

namespace smgpc::compat {
    void register_actor_binder(const LiveActor* actor) {
        if (actor != nullptr) {
            actor_binders().try_emplace(actor);
        }
    }

    bool has_actor_binder(const LiveActor* actor) {
        return actor != nullptr && actor_binders().contains(actor);
    }

    void clear_actor_binder_contacts(LiveActor* actor) {
        if (actor == nullptr) {
            return;
        }
        if (const auto found = actor_binders().find(actor); found != actor_binders().end()) {
            found->second = {};
        }
        actor->mBindedGround = false;
        actor->mBindedWall = false;
        actor->mBindedRoof = false;
        actor->mBindedGroundDamageFire = false;
    }

    void record_actor_binder_contacts(LiveActor* actor, const ActorBinderContactState& contacts) {
        if (actor == nullptr) {
            return;
        }
        const auto found = actor_binders().find(actor);
        if (found == actor_binders().end()) {
            return;
        }
        found->second = contacts;
        actor->mBindedGround = contacts.ground;
        actor->mBindedWall = contacts.wall;
        actor->mBindedRoof = contacts.roof;
        // A KCL prism attribute is an index into CollisionParts' attribute
        // table, not a ground code. Do not invent a DamageFire result from it.
        actor->mBindedGroundDamageFire = false;
        if (contacts.ground) {
            actor->mGroundNormal.set(contacts.ground_normal);
        }
        if (contacts.wall) {
            actor->mWallNormal.set(contacts.wall_normal);
        }
        if (contacts.roof) {
            actor->mRoofNormal.set(contacts.roof_normal);
        }
    }

    const ActorBinderContactState* actor_binder_contacts(const LiveActor* actor) {
        const auto found = actor_binders().find(actor);
        return found != actor_binders().end() ? &found->second : nullptr;
    }

    void release_actor_binder_state(const LiveActor* actor) {
        actor_binders().erase(actor);
    }

    void configure_actor_clipping_sphere(LiveActor* actor, float radius, const TVec3f* center) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor clipping operation requires a LiveActor.");
        }
        if (!std::isfinite(radius) || radius < 0.0F) {
            throw std::invalid_argument("Actor clipping radius must be finite and non-negative.");
        }
        auto& state = actor_clipping_states()[actor];
        state.sphere_configured = true;
        state.sphere_radius = radius;
        state.sphere_center = center;
    }

    void configure_actor_clipping_far_level(LiveActor* actor, int level) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor clipping operation requires a LiveActor.");
        }
        if (level < 0 || level > 7) {
            throw std::invalid_argument("Actor clipping far level must be in the original 0..7 range.");
        }
        actor_clipping_states()[actor].far_level = level;
        actor->mClippingFarLevel = level;
    }

    const ActorClippingRuntimeState* actor_clipping_runtime_state(const LiveActor* actor) {
        const auto found = actor_clipping_states().find(actor);
        return found != actor_clipping_states().end() ? &found->second : nullptr;
    }

    void release_actor_clipping_state(const LiveActor* actor) {
        actor_clipping_states().erase(actor);
    }

    void register_actor_model(const LiveActor* actor, smgpc::render::live_actor::LiveActorModel* model) {
        if (actor != nullptr) {
            actor_models().insert_or_assign(actor, model);
        }
    }

    smgpc::render::live_actor::LiveActorModel* actor_model(const LiveActor* actor) {
        const auto found = actor_models().find(actor);
        return found != actor_models().end() ? found->second : nullptr;
    }

    void release_actor_model_state(const LiveActor* actor) {
        actor_models().erase(actor);
    }

    void release_actor_runtime_state(const LiveActor* actor) {
        release_actor_collision_parts(actor);
        release_actor_sensor_bindings(actor);
        release_actor_binder_state(actor);
        release_actor_model_state(actor);
        release_actor_clipping_state(actor);
        release_actor_material_ctrl_state(actor);
        release_talk_runtime_state(actor);
        release_demo_runtime_state(actor);
    }
}  // namespace smgpc::compat
