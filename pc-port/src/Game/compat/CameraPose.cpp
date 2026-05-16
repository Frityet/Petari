#include "CameraPose.hpp"

#include <cmath>

namespace smgpc::game {
    namespace {

        [[nodiscard]] CameraParamVec3 subtract(const CameraParamVec3& a, const CameraParamVec3& b) {
            return CameraParamVec3{
                .x = a.x - b.x,
                .y = a.y - b.y,
                .z = a.z - b.z,
            };
        }

        [[nodiscard]] CameraParamVec3 cross(const CameraParamVec3& a, const CameraParamVec3& b) {
            return CameraParamVec3{
                .x = a.y * b.z - a.z * b.y,
                .y = a.z * b.x - a.x * b.z,
                .z = a.x * b.y - a.y * b.x,
            };
        }

        [[nodiscard]] float dot(const CameraParamVec3& a, const CameraParamVec3& b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        [[nodiscard]] CameraParamVec3 normalized_or(const CameraParamVec3& value, const CameraParamVec3& fallback) {
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

    CameraPoseCompat file_select_title_camera_pose() {
        constexpr auto c_far_target_y = 800.0F;
        constexpr auto c_far_point_z = 15000.0F;
        constexpr auto c_title_height_offset = 15000.0F;

        return CameraPoseCompat{
            .eye = {0.0F, c_far_target_y + c_title_height_offset, c_far_point_z},
            .watch = {0.0F, c_far_target_y + c_title_height_offset, 0.0F},
            .up = {0.0F, 1.0F, 0.0F},
            .fovy_degrees = 60.0F,
            .aspect_ratio = 608.0F / 456.0F,
            .near_clip = 100.0F,
            .far_clip = 800000.0F,
        };
    }

    CameraViewPointCompat transform_world_to_camera(const CameraPoseCompat& pose, const CameraParamVec3& world) {
        const auto forward = normalized_or(subtract(pose.watch, pose.eye), {0.0F, 0.0F, -1.0F});
        const auto right = normalized_or(cross(forward, pose.up), {1.0F, 0.0F, 0.0F});
        const auto corrected_up = normalized_or(cross(right, forward), {0.0F, 1.0F, 0.0F});
        const auto delta = subtract(world, pose.eye);

        return CameraViewPointCompat{
            .x = dot(delta, right),
            .y = dot(delta, corrected_up),
            .z = dot(delta, forward),
        };
    }

}  // namespace smgpc::game
