#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/JGeometry/TVec.hpp>
#include <revolution/mtx.h>

#include "camera/CameraPose.hpp"
#include "render/J3dMatrix.hpp"
#include "render/live_actor/LiveActorModel.hpp"

class ActorLightCtrl;
class HitSensor;
class JMapInfoIter;
class LiveActor;
class LodCtrl;
class NameObj;
class Nerve;
class RailRider;
class Spine;
class StageSwitchCtrl;
class TalkMessageCtrl;

namespace smgpc::compat {
    struct NameObjRuntimeRegistrationMarker final {
        std::uint64_t next_registration_order = 0U;
    };

    // Construction capture shares the process-global NameObj registry and is
    // therefore restricted to the scene-construction thread. Overlapping or
    // nested capture scopes are rejected instead of exposing the same raw
    // child identity to two owners.
    class NameObjRuntimeRegistrationCapture final {
    public:
        NameObjRuntimeRegistrationCapture();
        ~NameObjRuntimeRegistrationCapture();

        NameObjRuntimeRegistrationCapture(
            const NameObjRuntimeRegistrationCapture &) = delete;
        NameObjRuntimeRegistrationCapture &operator=(
            const NameObjRuntimeRegistrationCapture &) = delete;
        NameObjRuntimeRegistrationCapture(
            NameObjRuntimeRegistrationCapture &&) = delete;
        NameObjRuntimeRegistrationCapture &operator=(
            NameObjRuntimeRegistrationCapture &&) = delete;

        [[nodiscard]] NameObjRuntimeRegistrationMarker marker() const noexcept;

    private:
        NameObjRuntimeRegistrationMarker _marker{};
    };

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

    struct ActorBinderRuntimeConfig {
        float radius = 0.0F;
        float offset = 0.0F;
        std::uint32_t plane_capacity = 0U;
    };

    struct ActorClippingRuntimeState {
        bool sphere_configured = false;
        float sphere_radius = 0.0F;
        // Null has the original meaning: center the sphere on actor position.
        const TVec3f* sphere_center = nullptr;
        std::optional<int> far_level{};
    };

    enum class ActorShadowControllerKind {
        SurfaceCircle,
        SurfaceOval,
        SurfaceBox,
        VolumeSphere,
        VolumeOval,
        VolumeOvalPole,
        VolumeCylinder,
        VolumeBox,
        VolumeFlatModel,
        VolumeLine,
    };

    enum class ActorShadowPositionBinding {
        ActorTranslation,
        BaseMatrix,
        FixedPosition,
        OtherTranslation,
        OtherMatrix,
        JointMatrix,
    };

    enum class ActorShadowCalculationMode {
        Disabled,
        Continuous,
        OneTime,
    };

    enum class ActorShadowGravityMode {
        HostDirection,
        HostContinuous,
        HostOneTime,
        PrivateDisabled,
        PrivateContinuous,
        PrivateOneTime,
    };

    struct ActorShadowControllerRuntimeState {
        std::string name{};
        std::string name_raw{};
        std::string group_name{};
        ActorShadowControllerKind kind = ActorShadowControllerKind::VolumeSphere;
        ActorShadowPositionBinding position_binding = ActorShadowPositionBinding::ActorTranslation;
        std::string joint_name{};
        std::string joint_name_raw{};
        std::optional<std::string> model_name{};
        std::optional<std::string> line_start_name{};
        std::optional<std::string> line_end_name{};
        std::optional<std::string> line_start_name_raw{};
        std::optional<std::string> line_end_name_raw{};
        float radius = 0.0F;
        TVec3f size{};
        TVec3f drop_offset{};
        TVec3f fixed_drop_position{};
        TVec3f fixed_drop_direction{};
        const TVec3f* drop_position = nullptr;
        MtxPtr drop_position_matrix = nullptr;
        const TVec3f* drop_direction = nullptr;
        float drop_length = 1000.0F;
        float drop_start_offset = 0.0F;
        float volume_start_offset = 100.0F;
        float volume_end_offset = 100.0F;
        float line_start_radius = 100.0F;
        float line_end_radius = 100.0F;
        std::optional<std::size_t> line_start_controller_index{};
        std::optional<std::size_t> line_end_controller_index{};
        bool volume_cut_drop_length = false;
        bool follow_host_scale = false;
        bool valid = true;
        bool visible_sync_host = true;
        ActorShadowCalculationMode calculation_mode = ActorShadowCalculationMode::Disabled;
        ActorShadowGravityMode gravity_mode = ActorShadowGravityMode::HostDirection;
    };

    struct ActorShadowRuntimeState {
        // Retain the original aggregate observability while the exact
        // controller list carries the per-shadow state below.
        bool valid = false;
        bool calculation_enabled = false;
        bool private_gravity = false;
        std::uint32_t capacity = 0U;
        std::vector<ActorShadowControllerRuntimeState> controllers{};
    };

    // NameObj keeps the retail const-char pointer. The host-owned copy lives
    // here so names remain stable without expanding the Game object.
    [[nodiscard]] const char* register_name_obj_runtime_state(NameObj* object, const char* name);
    [[nodiscard]] const char* update_name_obj_runtime_name(NameObj* object, const char* name);
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
    [[nodiscard]] const void* name_obj_runtime_owner(
        const NameObj* object) noexcept;
    // A construction boundary can observe an independently-owned NameObj in
    // its ordered registration suffix and take responsibility only for that
    // object's one scene-wide initAfterPlacement callback. The storage owner
    // remains unchanged and can skip its pre-pass using this marker.
    void delegate_name_obj_runtime_postpass(NameObj* object,
                                            const void* delegate);
    void release_name_obj_runtime_postpass_delegation(
        const NameObj* object, const void* delegate) noexcept;
    [[nodiscard]] bool name_obj_runtime_postpass_is_delegated(
        const NameObj* object) noexcept;
    [[nodiscard]] const void* name_obj_runtime_postpass_delegate(
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
    // Failure rollback for compatibility owners. This performs no allocation
    // and retires the still-live suffix in reverse construction order.
    void destroy_name_obj_runtime_objects_since(
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
    [[nodiscard]] std::size_t actor_lod_ctrl_runtime_state_count();

    void initialize_actor_model(LiveActor* actor, const char* model_archive, const char* animation_archive);
    [[nodiscard]] smgpc::render::live_actor::LiveActorModel* actor_model(const LiveActor* actor);
    [[nodiscard]] std::optional<std::span<const std::uint8_t>>
    actor_model_resource_data_if_present(const LiveActor* actor,
                                         std::string_view resource_name);
    void require_actor_model(LiveActor* actor);
    [[nodiscard]] std::size_t actor_model_joint_count(const LiveActor* actor);
    void release_actor_model_state(const LiveActor* actor);
    // Original model base TR; model scale is retained separately.
    [[nodiscard]] const smgpc::render::J3dMatrix3x4& actor_base_matrix(const LiveActor* actor);
    void set_actor_base_matrix(LiveActor* actor, const smgpc::render::J3dMatrix3x4& matrix);
    void set_actor_model_base_scale(LiveActor* actor, const TVec3f& scale);
    void set_actor_projmap_effect_matrix(LiveActor* actor, const smgpc::render::J3dMatrix3x4& matrix);
    void draw_actor_model(LiveActor* actor, const smgpc::camera::CameraPose& camera_pose,
                          std::uint64_t frame, smgpc::render::live_actor::LiveActorModel::DrawPass pass);
    void draw_actor_model_3d_for_2d(
        LiveActor* actor,
        const smgpc::render::Model3DFor2DProjection& projection,
        std::uint64_t frame,
        smgpc::render::live_actor::LiveActorModel::DrawPass pass);

    void start_actor_bck(LiveActor* actor, const char* name, const char* file_name);
    [[nodiscard]] std::int16_t require_actor_bck(LiveActor* actor, const char* name, const char* file_name);
    void start_actor_brk(LiveActor* actor, const char* name);
    void start_actor_btk(LiveActor* actor, const char* name);
    void start_actor_btp(LiveActor* actor, const char* name);
    [[nodiscard]] bool try_start_actor_bck(LiveActor* actor, const char* name, const char* file_name);
    [[nodiscard]] bool try_start_actor_brk(LiveActor* actor, const char* name);
    [[nodiscard]] bool try_start_actor_btk(LiveActor* actor, const char* name);
    [[nodiscard]] bool try_start_actor_btp(LiveActor* actor, const char* name);
    void set_actor_brk_frame(LiveActor* actor, float frame);
    void set_actor_brk_rate(LiveActor* actor, float rate);
    void set_actor_brk_frame_and_stop(LiveActor* actor, float frame);
    void set_actor_brk_frame_end_and_stop(LiveActor* actor);
    void set_actor_bck_frame_and_stop(LiveActor* actor, float frame);
    [[nodiscard]] J3DFrameCtrl* actor_bck_ctrl(const LiveActor* actor);
    [[nodiscard]] J3DFrameCtrl* actor_brk_ctrl(const LiveActor* actor);
    [[nodiscard]] bool is_actor_brk_one_time_and_stopped(const LiveActor* actor);
    [[nodiscard]] std::string_view actor_current_bck_name(const LiveActor* actor);
    [[nodiscard]] std::string_view actor_current_brk_name(const LiveActor* actor);
    [[nodiscard]] std::string_view actor_current_btk_name(const LiveActor* actor);
    [[nodiscard]] std::string_view actor_current_btp_name(const LiveActor* actor);
    void advance_actor_animation(LiveActor* actor);
    void synchronize_actor_model_animation(LiveActor* actor);

    void initialize_actor_hit_sensors(LiveActor* actor, int sensor_count);
    [[nodiscard]] HitSensor* add_actor_hit_sensor(LiveActor* actor, const char* name, std::uint32_t type,
                                                  std::uint16_t group_size, float radius, const TVec3f& offset);
    [[nodiscard]] HitSensor* actor_hit_sensor(const LiveActor* actor, const char* name);
    [[nodiscard]] const char* actor_hit_sensor_name(const LiveActor* actor, const HitSensor* sensor);
    void collect_actor_hit_sensors(const LiveActor* actor, std::vector<HitSensor*>& sensors);
    void validate_actor_hit_sensors(LiveActor* actor);
    void invalidate_actor_hit_sensors(LiveActor* actor);
    void update_actor_hit_sensors(LiveActor* actor);
    [[nodiscard]] std::size_t actor_hit_sensor_count(const LiveActor* actor);

    void configure_actor_binder(LiveActor* actor, float radius, float offset, std::uint32_t plane_capacity);
    void register_actor_binder(const LiveActor* actor);
    [[nodiscard]] bool has_actor_binder(const LiveActor* actor);
    [[nodiscard]] const ActorBinderRuntimeConfig* actor_binder_config(const LiveActor* actor);
    void clear_actor_binder_contacts(LiveActor* actor);
    void record_actor_binder_contacts(LiveActor* actor, const ActorBinderContactState& contacts);
    [[nodiscard]] const ActorBinderContactState* actor_binder_contacts(const LiveActor* actor);
    void release_actor_binder_state(const LiveActor* actor);

    void configure_actor_clipping_sphere(LiveActor* actor, float radius, const TVec3f* center);
    void configure_actor_clipping_far_level(LiveActor* actor, int level);
    [[nodiscard]] const ActorClippingRuntimeState* actor_clipping_runtime_state(const LiveActor* actor);
    void release_actor_clipping_state(const LiveActor* actor);

    void initialize_actor_shadow_controller_list(LiveActor* actor, std::uint32_t capacity);
    [[nodiscard]] ActorShadowControllerRuntimeState make_actor_shadow_controller_runtime_state(
        LiveActor* actor, std::string_view name, ActorShadowControllerKind kind, float radius);
    void replace_actor_shadow_runtime_state(LiveActor* actor, ActorShadowRuntimeState state);
    [[nodiscard]] ActorShadowControllerRuntimeState& add_actor_shadow_controller(
        LiveActor* actor, std::string_view name, ActorShadowControllerKind kind, float radius);
    [[nodiscard]] ActorShadowControllerRuntimeState* actor_shadow_controller_runtime_state(
        LiveActor* actor, const char* name);
    [[nodiscard]] const ActorShadowControllerRuntimeState* actor_shadow_controller_runtime_state(
        const LiveActor* actor, const char* name);
    [[nodiscard]] ActorShadowRuntimeState* actor_shadow_runtime_state(LiveActor* actor);
    [[nodiscard]] const ActorShadowRuntimeState* actor_shadow_runtime_state(const LiveActor* actor);
    [[nodiscard]] std::size_t actor_shadow_runtime_state_count();

    void release_talk_runtime_state(const LiveActor* actor);
    [[nodiscard]] TalkMessageCtrl* owned_talk_ctrl(const LiveActor* actor);
    void release_demo_runtime_state(const LiveActor* actor);

    [[nodiscard]] bool has_owned_talk_ctrl(const LiveActor* actor);
    [[nodiscard]] bool has_registered_demo_cast(const LiveActor* actor);
    [[nodiscard]] std::size_t registered_demo_membership_count(const LiveActor* actor);
    [[nodiscard]] std::size_t registered_demo_action_count(const LiveActor* actor);
}  // namespace smgpc::compat
