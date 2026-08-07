#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <JSystem/JGeometry/TVec.hpp>

class LiveActor;
class TalkMessageCtrl;

namespace smgpc::render::live_actor {
    class LiveActorModel;
}

namespace smgpc::compat {
    struct ActorBinderContactState {
        bool ground = false;
        bool wall = false;
        bool roof = false;
        TVec3f ground_normal{};
        TVec3f wall_normal{};
        TVec3f roof_normal{};
        TVec3f fix_reaction{};
        // This is the KCL prism attribute index. It is deliberately not
        // interpreted as a Floor_code value without the source
        // CollisionParts' attribute table.
        std::optional<std::uint16_t> ground_attribute{};
    };

    struct ActorClippingRuntimeState {
        bool sphere_configured = false;
        float sphere_radius = 0.0F;
        // Null has the original meaning: center the sphere on actor position.
        const TVec3f* sphere_center = nullptr;
        std::optional<int> far_level{};
    };

    void register_actor_binder(const LiveActor *actor);
    [[nodiscard]] bool has_actor_binder(const LiveActor *actor);
    void clear_actor_binder_contacts(LiveActor *actor);
    void record_actor_binder_contacts(LiveActor *actor, const ActorBinderContactState &contacts);
    [[nodiscard]] const ActorBinderContactState *actor_binder_contacts(const LiveActor *actor);
    void release_actor_binder_state(const LiveActor *actor);
    void configure_actor_clipping_sphere(LiveActor *actor, float radius, const TVec3f *center);
    void configure_actor_clipping_far_level(LiveActor *actor, int level);
    [[nodiscard]] const ActorClippingRuntimeState *actor_clipping_runtime_state(const LiveActor *actor);
    void release_actor_clipping_state(const LiveActor *actor);
    void register_actor_model(const LiveActor *actor, smgpc::render::live_actor::LiveActorModel *model);
    [[nodiscard]] smgpc::render::live_actor::LiveActorModel *actor_model(const LiveActor *actor);
    void release_actor_model_state(const LiveActor *actor);

    void release_talk_runtime_state(const LiveActor *actor);
    [[nodiscard]] TalkMessageCtrl *owned_talk_ctrl(const LiveActor *actor);
    void release_demo_runtime_state(const LiveActor *actor);
    void release_actor_runtime_state(const LiveActor *actor);

    [[nodiscard]] bool has_owned_talk_ctrl(const LiveActor *actor);
    [[nodiscard]] bool has_registered_demo_cast(const LiveActor *actor);
    [[nodiscard]] std::size_t registered_demo_membership_count(const LiveActor *actor);
    [[nodiscard]] std::size_t registered_demo_action_count(const LiveActor *actor);
}  // namespace smgpc::compat
