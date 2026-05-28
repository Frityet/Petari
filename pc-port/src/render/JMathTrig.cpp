#include "JMathTrig.hpp"

#include <cmath>

namespace smgpc::render {
    namespace {

        constexpr auto JMA_SINCOS_TABLE_BITS = 14U;
        constexpr auto JMA_SINCOS_TABLE_LENGTH = 1U << JMA_SINCOS_TABLE_BITS;
        constexpr auto JMA_SINCOS_TABLE_STEP = 6.283185482025146 / static_cast<double>(JMA_SINCOS_TABLE_LENGTH);
        constexpr auto JMA_SINCOS_RAD_TO_INDEX = static_cast<double>(JMA_SINCOS_TABLE_LENGTH) / 6.283185482025146;

        [[nodiscard]] float table_sin(std::uint16_t index) {
            return static_cast<float>(std::sin(static_cast<double>(index) * JMA_SINCOS_TABLE_STEP));
        }

        [[nodiscard]] float table_cos(std::uint16_t index) {
            return static_cast<float>(std::cos(static_cast<double>(index) * JMA_SINCOS_TABLE_STEP));
        }

    }  // namespace

    std::uint16_t jmath_sincos_table_index_from_short(std::uint16_t binary_angle) {
        return static_cast<std::uint16_t>(binary_angle >> (16U - JMA_SINCOS_TABLE_BITS));
    }

    std::uint16_t jmath_fctiwz_to_u16(float value) {
        return static_cast<std::uint16_t>(static_cast<std::int32_t>(value));
    }

    float jmath_sin_short(std::uint16_t binary_angle) {
        return table_sin(jmath_sincos_table_index_from_short(binary_angle));
    }

    float jmath_cos_short(std::uint16_t binary_angle) {
        return table_cos(jmath_sincos_table_index_from_short(binary_angle));
    }

    float jmath_cos_lap_rad(float radians) {
        const auto index = jmath_fctiwz_to_u16(static_cast<float>(static_cast<double>(radians) * JMA_SINCOS_RAD_TO_INDEX));
        return table_cos(index);
    }

}  // namespace smgpc::render
