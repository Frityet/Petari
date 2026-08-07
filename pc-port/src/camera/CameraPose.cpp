#include "CameraPose.hpp"

#include <cmath>
#include <stdexcept>

namespace smgpc::camera {
    namespace {

        [[nodiscard]] CameraParamVec3 subtract(const CameraParamVec3 &a, const CameraParamVec3 &b) {
            return CameraParamVec3 {
                .x = a.x - b.x,
                .y = a.y - b.y,
                .z = a.z - b.z,
            };
        }

        [[nodiscard]] CameraParamVec3 cross(const CameraParamVec3 &a, const CameraParamVec3 &b) {
            return CameraParamVec3 {
                .x = a.y * b.z - a.z * b.y,
                .y = a.z * b.x - a.x * b.z,
                .z = a.x * b.y - a.y * b.x,
            };
        }

        [[nodiscard]] float dot(const CameraParamVec3 &a, const CameraParamVec3 &b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        [[nodiscard]] CameraParamVec3 normalized(const CameraParamVec3 &value) {
            const auto length = std::sqrt(dot(value, value));
            if (length <= 0.000001F) {
                throw std::logic_error("camera view basis is degenerate");
            }

            return CameraParamVec3 {
                .x = value.x / length,
                .y = value.y / length,
                .z = value.z / length,
            };
        }

    }  // namespace

    CameraViewPoint transform_world_to_camera(const CameraPose &pose, const CameraParamVec3 &world) {
        const auto forward = normalized(subtract(pose.watch, pose.eye));
        const auto right = normalized(cross(forward, pose.up));
        const auto corrected_up = normalized(cross(right, forward));
        const auto delta = subtract(world, pose.eye);

        return CameraViewPoint {
            .x = dot(delta, right),
            .y = dot(delta, corrected_up),
            .z = dot(delta, forward),
        };
    }

}  // namespace smgpc::camera
