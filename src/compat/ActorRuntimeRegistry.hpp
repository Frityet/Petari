#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/JGeometry/TVec.hpp>
#include <revolution/mtx.h>

#include "camera/CameraPose.hpp"
#include "render/J3dMatrix.hpp"
#include <memory>

class ActorLightCtrl;
class HitSensor;
class HitSensorKeeper;
class JMapInfoIter;
class LiveActor;
class ModelManager;
class ClippingActorHolder;
class ClippingGroupHolder;
class LodCtrl;
class NameObj;
class Nerve;
class RailRider;
class Spine;
class ShadowController;
class StageSwitchCtrl;
class TalkMessageCtrl;

namespace smgpc::compat {
    struct NameObjRuntimeRegistrationMarker final {
        std::uint64_t next_registration_order = 0U;
    };

    // Original names are borrowed, including member buffers filled after the
    // NameObj base constructor. Registration must not inspect their bytes.
    void register_name_obj_runtime_state(NameObj* object);
    void release_name_obj_runtime_state(const NameObj* object);
    [[nodiscard]] bool has_name_obj_runtime_state(const NameObj* object);
    [[nodiscard]] std::uint64_t name_obj_runtime_generation(
        const NameObj* object) noexcept;
    [[nodiscard]] std::size_t name_obj_runtime_state_count();
    // Host-side owners that retain a registered NameObj independently of the
    // scene heap must claim it at adoption time. Construction captures treat
    // claimed identities as non-owning observations and never adopt/delete
    // them a second time.
    void claim_name_obj_runtime_ownership(NameObj* object,
                                          const void* owner);
    [[nodiscard]] bool name_obj_runtime_ownership_is_claimed(
        const NameObj* object) noexcept;
    // Returns the currently live host-tracked NameObj identities in their
    // construction order. A marker provides the matching ordered suffix so a
    // compatibility owner can adopt raw-new retail construction children
    // without changing the original Game class layout or source.
    [[nodiscard]] std::vector<NameObj*> snapshot_name_obj_runtime_objects();
    [[nodiscard]] NameObjRuntimeRegistrationMarker mark_name_obj_runtime_registrations();
    [[nodiscard]] std::vector<NameObj*> snapshot_name_obj_runtime_objects_since(
        NameObjRuntimeRegistrationMarker marker);
    using NameObjRuntimeRegistrationFilter =
        bool (*)(const NameObj* object, const void* context) noexcept;
    // Finds the newest still-live identity in a captured suffix without
    // allocating. Compatibility owners use this to unwind a partially built
    // graph in exact reverse construction order while excluding identities
    // that another scene service already owns.
    [[nodiscard]] NameObj* newest_name_obj_runtime_object_since_if(
        NameObjRuntimeRegistrationMarker marker,
        NameObjRuntimeRegistrationFilter filter,
        const void* context) noexcept;
    [[nodiscard]] bool name_obj_runtime_object_was_registered_since(
        const NameObj* object,
        NameObjRuntimeRegistrationMarker marker) noexcept;
    [[nodiscard]] bool name_obj_is_suspended(const NameObj* object);

    // One generalized record owns every native-only LiveActor resource. The
    // exact Game fields remain the retail pointer/flag surface and never own
    // STL, renderer, or native collision state.
    void register_actor_runtime_state(LiveActor* actor);
    [[nodiscard]] bool has_actor_runtime_state(const LiveActor* actor);
    [[nodiscard]] std::size_t actor_runtime_state_count();
    void release_actor_runtime_state(const LiveActor* actor);

    void replace_actor_spine(LiveActor* actor, const Nerve* nerve);
    void update_actor_nerve(LiveActor* actor);
    void replace_actor_rail_rider(LiveActor* actor, const JMapInfoIter& iter);
    void adopt_actor_stage_switch(LiveActor* actor, StageSwitchCtrl* controller);
    void replace_actor_light_ctrl(LiveActor* actor);
    void adopt_actor_lod_ctrl(LiveActor* actor, LodCtrl* lod_ctrl);

    class JkrAllocationDomain;
    [[nodiscard]] std::shared_ptr<JkrAllocationDomain> actor_scene_allocation_domain(const LiveActor*);
    void adopt_actor_sound_object(LiveActor*, std::shared_ptr<JkrAllocationDomain>);
    void initialize_actor_model(LiveActor* actor, const char* model_archive,
                                const char* animation_archive, bool create_display_list);
    void adopt_actor_animation_helpers(LiveActor* actor);
    [[nodiscard]] std::shared_ptr<ModelManager> retain_actor_model(const LiveActor* actor);

    // Borrow retirement only: Game's actual keeper owns all sensor storage.
    void retire_hit_sensor_borrows(const HitSensorKeeper* keeper) noexcept;

    void configure_actor_binder(LiveActor* actor, float radius, float offset, std::uint32_t plane_capacity);

    void retire_clipping_actor_holder(ClippingActorHolder& holder) noexcept;
    void retire_clipping_group_holder(ClippingGroupHolder& holder) noexcept;

}  // namespace smgpc::compat
