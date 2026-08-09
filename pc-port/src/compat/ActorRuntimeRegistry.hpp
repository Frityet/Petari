#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/JGeometry/TVec.hpp>

#include "camera/CameraPose.hpp"
#include "render/J3dMatrix.hpp"
#include "render/live_actor/LiveActorModel.hpp"

class ActorLightCtrl;
class HitSensor;
class JMapInfoIter;
class LiveActor;
class NameObj;
class Nerve;
class RailRider;
class Spine;
class StageSwitchCtrl;
class TalkMessageCtrl;

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

    struct ActorShadowRuntimeState {
        bool valid = false;
        bool calculation_enabled = false;
        bool private_gravity = false;
    };

    // NameObj keeps the retail const-char pointer. The host-owned copy lives
    // here so names remain stable without expanding the Game object.
    [[nodiscard]] const char* register_name_obj_runtime_state(NameObj* object, const char* name);
    [[nodiscard]] const char* update_name_obj_runtime_name(NameObj* object, const char* name);
    void release_name_obj_runtime_state(const NameObj* object);
    [[nodiscard]] bool has_name_obj_runtime_state(const NameObj* object);
    [[nodiscard]] std::size_t name_obj_runtime_state_count();
    // Returns the currently live host-tracked NameObj identities. This lets
    // compatibility owners adopt retail child objects allocated with raw new
    // without changing the original Game class layout or source.
    [[nodiscard]] std::vector<NameObj*> snapshot_name_obj_runtime_objects();
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

    void initialize_actor_model(LiveActor* actor, const char* model_archive, const char* animation_archive);
    [[nodiscard]] smgpc::render::live_actor::LiveActorModel* actor_model(const LiveActor* actor);
    void require_actor_model(LiveActor* actor);
    [[nodiscard]] std::size_t actor_model_joint_count(const LiveActor* actor);
    void release_actor_model_state(const LiveActor* actor);
    [[nodiscard]] const smgpc::render::J3dMatrix3x4& actor_base_matrix(const LiveActor* actor);
    void set_actor_base_matrix(LiveActor* actor, const smgpc::render::J3dMatrix3x4& matrix);
    void set_actor_projmap_effect_matrix(LiveActor* actor, const smgpc::render::J3dMatrix3x4& matrix);
    void draw_actor_model(LiveActor* actor, const smgpc::camera::CameraPose& camera_pose,
                          std::uint64_t frame, smgpc::render::live_actor::LiveActorModel::DrawPass pass);

    void start_actor_bck(LiveActor* actor, const char* name, const char* file_name);
    [[nodiscard]] std::int16_t require_actor_bck(LiveActor* actor, const char* name, const char* file_name);
    void start_actor_brk(LiveActor* actor, const char* name);
    void start_actor_btk(LiveActor* actor, const char* name);
    void set_actor_brk_frame(LiveActor* actor, float frame);
    void set_actor_brk_frame_and_stop(LiveActor* actor, float frame);
    void set_actor_brk_frame_end_and_stop(LiveActor* actor);
    [[nodiscard]] J3DFrameCtrl* actor_brk_ctrl(const LiveActor* actor);
    [[nodiscard]] bool is_actor_brk_one_time_and_stopped(const LiveActor* actor);
    [[nodiscard]] std::string_view actor_current_bck_name(const LiveActor* actor);
    [[nodiscard]] std::string_view actor_current_brk_name(const LiveActor* actor);
    [[nodiscard]] std::string_view actor_current_btk_name(const LiveActor* actor);
    void advance_actor_animation(LiveActor* actor);

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

    [[nodiscard]] ActorShadowRuntimeState* actor_shadow_runtime_state(LiveActor* actor);
    [[nodiscard]] const ActorShadowRuntimeState* actor_shadow_runtime_state(const LiveActor* actor);

    void release_talk_runtime_state(const LiveActor* actor);
    [[nodiscard]] TalkMessageCtrl* owned_talk_ctrl(const LiveActor* actor);
    void release_demo_runtime_state(const LiveActor* actor);

    [[nodiscard]] bool has_owned_talk_ctrl(const LiveActor* actor);
    [[nodiscard]] bool has_registered_demo_cast(const LiveActor* actor);
    [[nodiscard]] std::size_t registered_demo_membership_count(const LiveActor* actor);
    [[nodiscard]] std::size_t registered_demo_action_count(const LiveActor* actor);
}  // namespace smgpc::compat
