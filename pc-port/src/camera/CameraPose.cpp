#include "CameraPose.hpp"

#include <cmath>

namespace smgpc::camera {
    namespace {

        [[nodiscard]] CameraParamVec3 subtract(const CameraParamVec3 &a, const CameraParamVec3 &b) {
            return CameraParamVec3{
                .x = a.x - b.x,
                .y = a.y - b.y,
                .z = a.z - b.z,
            };
        }

        [[nodiscard]] CameraParamVec3 cross(const CameraParamVec3 &a, const CameraParamVec3 &b) {
            return CameraParamVec3{
                .x = a.y * b.z - a.z * b.y,
                .y = a.z * b.x - a.x * b.z,
                .z = a.x * b.y - a.y * b.x,
            };
        }

        [[nodiscard]] float dot(const CameraParamVec3 &a, const CameraParamVec3 &b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        [[nodiscard]] CameraParamVec3 normalized_or(const CameraParamVec3 &value, const CameraParamVec3 &fallback) {
            const auto length = std::sqrt(dot(value, value));
            if (length <= 0.000001F) {
                return fallback;
            }

            return CameraParamVec3{
                .x = value.x / length,
                .y = value.y / length,
                .z = value.z / length,
            };
        }

    }  // namespace

    CameraViewPoint transform_world_to_camera(const CameraPose &pose, const CameraParamVec3 &world) {
        const auto forward = normalized_or(subtract(pose.watch, pose.eye), {0.0F, 0.0F, -1.0F});
        const auto right = normalized_or(cross(forward, pose.up), {1.0F, 0.0F, 0.0F});
        const auto corrected_up = normalized_or(cross(right, forward), {0.0F, 1.0F, 0.0F});
        const auto delta = subtract(world, pose.eye);

        return CameraViewPoint{
            .x = dot(delta, right),
            .y = dot(delta, corrected_up),
            .z = dot(delta, forward),
        };
    }

}  // namespace smgpc::camera
