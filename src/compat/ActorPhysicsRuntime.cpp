#include "compat/ActorPhysicsRuntime.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "camera/CameraPose.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <array>
#include <cmath>

namespace {
    bool sphere_outside_frustum(const smgpc::camera::CameraPose& camera, const TVec3f& center, float radius,
                                int far_level) {
        constexpr auto clip_distances = std::array<float, 8U>{
            0.0F, 60000.0F, 50000.0F, 40000.0F, 30000.0F, 20000.0F, 10000.0F, 5000.0F,
        };
        const auto point = smgpc::camera::transform_world_to_camera(
            camera, {.x = center.x, .y = center.y, .z = center.z});
        const auto far_distance = far_level == 0 ? camera.far_clip : clip_distances[static_cast<std::size_t>(far_level)];
        if (point.z + radius < camera.near_clip || point.z - radius > far_distance) {
            return true;
        }

        const auto half_vertical = camera.fovy_degrees * (3.14159265358979323846F / 360.0F);
        const auto vertical_sin = std::sin(half_vertical);
        const auto vertical_cos = std::cos(half_vertical);
        if (point.z * vertical_sin - std::abs(point.y) * vertical_cos < -radius) {
            return true;
        }

        const auto half_horizontal = std::atan(std::tan(half_vertical) * camera.aspect_ratio);
        const auto horizontal_sin = std::sin(half_horizontal);
        const auto horizontal_cos = std::cos(half_horizontal);
        return point.z * horizontal_sin - std::abs(point.x) * horizontal_cos < -radius;
    }
}  // namespace

namespace smgpc::compat {
    void update_actor_clipping(LiveActor& actor, const smgpc::camera::CameraPose& camera) {
        const auto* state = actor_clipping_runtime_state(&actor);
        if (state == nullptr || !state->sphere_configured || actor.mFlag.mIsInvalidClipping) {
            if (actor.mFlag.mIsInvalidClipping && actor.mFlag.mIsClipped) {
                actor.endClipped();
            }
            return;
        }
        const auto& center = state->sphere_center != nullptr ? *state->sphere_center : actor.mPosition;
        const auto clipped = sphere_outside_frustum(camera, center, state->sphere_radius, state->far_level.value_or(0));
        if (clipped && !actor.mFlag.mIsClipped) {
            actor.startClipped();
        } else if (!clipped && actor.mFlag.mIsClipped) {
            actor.endClipped();
        }
    }

}  // namespace smgpc::compat
