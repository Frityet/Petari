#pragma once

#include "Game/compat/J3dMaterialRuntime.hpp"

#include <cstdint>

namespace smgpc::game {

    [[nodiscard]] float file_select_sky_yaw(std::uint64_t frame);
    [[nodiscard]] float file_select_sky_pitch(std::uint64_t frame);
    [[nodiscard]] J3dMatrix3x4 file_select_sky_actor_matrix(std::uint64_t frame);

}  // namespace smgpc::game
