#pragma once

#include "camera/CameraPose.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <JSystem/JGeometry/TVec.hpp>

#include <memory>
#include <optional>

class LiveActor;

namespace smgpc::runtime {
    class RuntimeContext;
}

namespace smgpc::compat {

    struct StagePlayerBasis {
        TVec3f side{1.0F, 0.0F, 0.0F};
        TVec3f up{0.0F, 1.0F, 0.0F};
        TVec3f front{0.0F, 0.0F, 1.0F};
    };

    struct StagePlayerMotionConfig {
        float maximum_speed = 14.0F;
        float ground_acceleration = 0.22F;
        float air_acceleration = 0.08F;
        float gravity_acceleration = 1.2F;
        float terminal_speed = 40.0F;
        float jump_speed = 20.0F;
        float ground_snap_speed = 1.5F;
    };

    struct StagePlayerFollowCamera {
        TVec3f reference_front{0.0F, 0.0F, 1.0F};
        TVec3f eye_local{};
        TVec3f watch_local{};
        float fovy_degrees = 45.0F;
        float aspect_ratio = 608.0F / 456.0F;
        float near_clip = 100.0F;
        float far_clip = 800000.0F;
    };

    [[nodiscard]] StagePlayerBasis calculate_stage_player_basis(const TVec3f &gravity,
                                                                 const TVec3f &preferred_front);
    [[nodiscard]] TVec3f calculate_camera_relative_stage_player_input(float stick_x, float stick_y,
                                                                       const TVec3f &gravity,
                                                                       const smgpc::camera::CameraPose &camera_pose,
                                                                       const TVec3f &fallback_front,
                                                                       float dead_zone = 0.15F);
    [[nodiscard]] TVec3f calculate_stage_player_velocity(const TVec3f &velocity, const TVec3f &tangent_input,
                                                          const TVec3f &gravity, bool on_ground, bool jump_triggered,
                                                          const StagePlayerMotionConfig &config = {});
    [[nodiscard]] smgpc::camera::CameraPose make_stage_player_fallback_camera(const TVec3f &position,
                                                                              const TVec3f &gravity,
                                                                              const TVec3f &preferred_front);
    [[nodiscard]] StagePlayerFollowCamera make_stage_player_follow_camera(const smgpc::camera::CameraPose &pose,
                                                                          const TVec3f &position,
                                                                          const TVec3f &gravity,
                                                                          const TVec3f &fallback_front);
    [[nodiscard]] smgpc::camera::CameraPose calculate_stage_player_follow_camera_pose(
        const StagePlayerFollowCamera &follow, const TVec3f &position, const TVec3f &gravity);

    class StagePlayerActor;

    // Compatibility-owned stand-in for MarioActor until the original player
    // module can be compiled. It consumes only generic StartInfo, input,
    // gravity, collision, scheduler, and camera services.
    class StagePlayerRuntime final {
    public:
        StagePlayerRuntime(smgpc::runtime::RuntimeContext &runtime,
                           const smgpc::scene::StageStartInfo &start_info);
        ~StagePlayerRuntime();

        StagePlayerRuntime(const StagePlayerRuntime &) = delete;
        StagePlayerRuntime &operator=(const StagePlayerRuntime &) = delete;

        void reset_to_start();
        void use_follow_camera_pose(const smgpc::camera::CameraPose &pose);
        void use_fallback_follow_camera();
        void synchronize_after_movement();

        [[nodiscard]] LiveActor *actor() const;
        [[nodiscard]] const smgpc::scene::StageStartInfo &start_info() const;

    private:
        smgpc::runtime::RuntimeContext &_runtime;
        smgpc::scene::StageStartInfo _start_info;
        std::unique_ptr<StagePlayerActor> _actor;
        std::optional<StagePlayerFollowCamera> _follow_camera;
    };

}  // namespace smgpc::compat
