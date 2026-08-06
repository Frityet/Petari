#pragma once

#include <array>
#include <cstdint>

#include "camera/CameraPose.hpp"
#include "render/core/RenderTypes.hpp"

namespace smgpc::render::effects {

    enum class JpcParticlePacketPath {
        ScreenSpace,
        WorldBillboard,
        WorldBillboardFallback,
    };

    struct JpcBillboardGeometry {
        camera::CameraParamVec3 center = {};
        float half_size_x = 0.0F;
        float half_size_y = 0.0F;
        float rotation_radians = 0.0F;
        std::array<std::uint8_t, 4U> color = {255U, 255U, 255U, 255U};
    };

    [[nodiscard]] bool jpc_shape_is_billboard(std::uint8_t shape_type);
    [[nodiscard]] JpcParticlePacketPath jpc_particle_packet_path(bool world_draw, std::uint8_t shape_type);
    [[nodiscard]] camera::CameraParamVec3
    jpc_transform_particle_center(const std::array<float, 12U> &host_matrix,
                                  const camera::CameraParamVec3 &local_center);
    [[nodiscard]] std::array<core::TexturedVertex2D, 4U>
    jpc_billboard_world_vertices(const camera::CameraPose &camera_pose, const JpcBillboardGeometry &geometry);

}  // namespace smgpc::render::effects
