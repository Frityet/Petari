#include "FileSelectSkyRuntime.hpp"

#include "Game/compat/JMathTrig.hpp"

#include <cmath>

namespace smgpc::game {

    float file_select_sky_yaw(std::uint64_t frame) {
        return static_cast< float >(frame) * 0.001F;
    }

    float file_select_sky_pitch(std::uint64_t frame) {
        constexpr auto pi = 3.1415927F;
        constexpr auto step_period = 3000.0F;
        constexpr auto short_angle_scale = 2607.5945F;

        auto steps = (pi * static_cast< float >(frame)) / step_period;
        if (steps < 0.0F) {
            steps = -steps;
        }

        const auto short_angle = jmath_fctiwz_to_u16(steps * short_angle_scale);
        const auto temp = 1.0F - jmath_cos_short(short_angle);
        return (3.0F * ((temp * 0.5F) * pi)) * 0.25F;
    }

    J3dMatrix3x4 file_select_sky_actor_matrix(std::uint64_t frame) {
        constexpr auto sky_scale = 0.8F;
        const auto yaw = file_select_sky_yaw(frame);
        const auto pitch = frame == 0U ? 0.0F : file_select_sky_pitch(frame - 1U);
        const auto cos_yaw = std::cos(yaw);
        const auto sin_yaw = std::sin(yaw);
        const auto cos_pitch = std::cos(pitch);
        const auto sin_pitch = std::sin(pitch);

        return J3dMatrix3x4{
            .m =
                {
                    sky_scale * cos_yaw,
                    0.0F,
                    -sky_scale * sin_yaw,
                    0.0F,
                    sky_scale * sin_pitch * sin_yaw,
                    sky_scale * cos_pitch,
                    sky_scale * sin_pitch * cos_yaw,
                    0.0F,
                    sky_scale * cos_pitch * sin_yaw,
                    -sky_scale * sin_pitch,
                    sky_scale * cos_pitch * cos_yaw,
                    0.0F,
                },
        };
    }

}  // namespace smgpc::game
