#include "JpcBillboard.hpp"

#include <cmath>

namespace smgpc::render::effects {
    namespace {

        [[nodiscard]] camera::CameraParamVec3 subtract(const camera::CameraParamVec3 &left,
                                                       const camera::CameraParamVec3 &right) {
            return {
                .x = left.x - right.x,
                .y = left.y - right.y,
                .z = left.z - right.z,
            };
        }

        [[nodiscard]] camera::CameraParamVec3 cross(const camera::CameraParamVec3 &left,
                                                    const camera::CameraParamVec3 &right) {
            return {
                .x = left.y * right.z - left.z * right.y,
                .y = left.z * right.x - left.x * right.z,
                .z = left.x * right.y - left.y * right.x,
            };
        }

        [[nodiscard]] float dot(const camera::CameraParamVec3 &left, const camera::CameraParamVec3 &right) {
            return left.x * right.x + left.y * right.y + left.z * right.z;
        }

        [[nodiscard]] camera::CameraParamVec3 normalized_or(const camera::CameraParamVec3 &value,
                                                            const camera::CameraParamVec3 &fallback) {
            const auto length = std::sqrt(dot(value, value));
            if (length <= 0.000001F) {
                return fallback;
            }

            return {
                .x = value.x / length,
                .y = value.y / length,
                .z = value.z / length,
            };
        }

        [[nodiscard]] core::TexturedVertex2D make_vertex(const JpcBillboardGeometry &geometry,
                                                         const camera::CameraParamVec3 &right,
                                                         const camera::CameraParamVec3 &up, float local_x,
                                                         float local_y, float u, float v) {
            const auto sine = std::sin(geometry.rotation_radians);
            const auto cosine = std::cos(geometry.rotation_radians);
            const auto view_x = cosine * geometry.half_size_x * local_x -
                                sine * geometry.half_size_y * local_y;
            const auto view_y = sine * geometry.half_size_x * local_x +
                                cosine * geometry.half_size_y * local_y;
            return {
                .x = geometry.center.x + right.x * view_x + up.x * view_y,
                .y = geometry.center.y + right.y * view_x + up.y * view_y,
                .z = geometry.center.z + right.z * view_x + up.z * view_y,
                .u = u,
                .v = v,
                .color = geometry.color,
            };
        }

    }  // namespace

    bool jpc_shape_is_billboard(std::uint8_t shape_type) {
        return shape_type == 2U;
    }

    std::optional<JpcParticlePacketPath> jpc_particle_packet_path(bool world_draw,
                                                                  std::uint8_t shape_type) {
        if (!jpc_shape_is_billboard(shape_type)) {
            return std::nullopt;
        }
        return world_draw ? JpcParticlePacketPath::WorldBillboard : JpcParticlePacketPath::ScreenSpace;
    }

    camera::CameraParamVec3 jpc_transform_particle_center(const std::array<float, 12U> &host_matrix,
                                                          const camera::CameraParamVec3 &local_center) {
        return {
            .x = host_matrix[0U] * local_center.x + host_matrix[1U] * local_center.y +
                 host_matrix[2U] * local_center.z + host_matrix[3U],
            .y = host_matrix[4U] * local_center.x + host_matrix[5U] * local_center.y +
                 host_matrix[6U] * local_center.z + host_matrix[7U],
            .z = host_matrix[8U] * local_center.x + host_matrix[9U] * local_center.y +
                 host_matrix[10U] * local_center.z + host_matrix[11U],
        };
    }

    std::array<core::TexturedVertex2D, 4U>
    jpc_billboard_world_vertices(const camera::CameraPose &camera_pose, const JpcBillboardGeometry &geometry) {
        const auto forward = normalized_or(subtract(camera_pose.watch, camera_pose.eye), {0.0F, 0.0F, -1.0F});
        const auto right = normalized_or(cross(forward, camera_pose.up), {1.0F, 0.0F, 0.0F});
        const auto corrected_up = normalized_or(cross(right, forward), {0.0F, 1.0F, 0.0F});

        return {
            make_vertex(geometry, right, corrected_up, -1.0F, -1.0F, 0.0F, 1.0F),
            make_vertex(geometry, right, corrected_up, 1.0F, -1.0F, 1.0F, 1.0F),
            make_vertex(geometry, right, corrected_up, 1.0F, 1.0F, 1.0F, 0.0F),
            make_vertex(geometry, right, corrected_up, -1.0F, 1.0F, 0.0F, 0.0F),
        };
    }

}  // namespace smgpc::render::effects
