#pragma once

#include <cstdint>

namespace smgpc::game {

    [[nodiscard]] std::uint16_t jmath_sincos_table_index_from_short(std::uint16_t binary_angle);
    [[nodiscard]] std::uint16_t jmath_fctiwz_to_u16(float value);
    [[nodiscard]] float jmath_sin_short(std::uint16_t binary_angle);
    [[nodiscard]] float jmath_cos_short(std::uint16_t binary_angle);

}  // namespace smgpc::game
