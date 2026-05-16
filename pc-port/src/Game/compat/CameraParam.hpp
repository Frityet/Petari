#pragma once

#include "Game/compat/BcsvTable.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::game {

    struct CameraParamVec3 {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
    };

    struct CameraGeneralParamCompat {
        float dist = 1200.0F;
        CameraParamVec3 axis = {0.0F, 1.0F, 0.0F};
        CameraParamVec3 w_point = {};
        CameraParamVec3 up = {};
        float angle_a = 0.0F;
        float angle_b = 0.3F;
        std::int32_t num1 = 0;
        std::int32_t num2 = 0;
        std::string string_param = {};
    };

    struct CameraExtraParamCompat {
        CameraParamVec3 w_offset = {0.0F, 100.0F, 0.0F};
        float l_offset = 0.0F;
        float l_offset_v = 0.0F;
        float roll = 0.0F;
        float fovy = 45.0F;
        std::int32_t cam_int = 120;
        std::uint16_t flags = 0U;
        float upper = 0.3F;
        float lower = 0.1F;
        std::int32_t gnd_int = 160;
        float u_play = 300.0F;
        float l_play = 800.0F;
        std::int32_t push_delay = 120;
        std::int32_t push_delay_low = 120;
        std::int32_t u_down = 120;
        std::int32_t v_pan_use = 1;
        CameraParamVec3 v_pan_axis = {0.0F, 1.0F, 0.0F};
    };

    struct CameraParamChunkCompat {
        std::uint32_t version = 0U;
        std::string id = "";
        std::string camera_type = "";
        CameraExtraParamCompat extra = {};
        CameraGeneralParamCompat general = {};
        std::int32_t game_thru = 0;
        std::int32_t game_enable_end_erp_frame = 0;
        std::uint32_t game_cam_end_int = 120U;
        std::int32_t event_enable_erp_frame = 0;
        std::int32_t event_enable_end_erp_frame = 0;
        std::uint32_t event_cam_end_int = 120U;
        std::uint32_t event_frame = 0U;
        std::uint32_t event_priority = 1U;

        [[nodiscard]] bool is_on_no_reset() const;
        [[nodiscard]] bool is_on_use_fovy() const;
        [[nodiscard]] bool is_l_offset_erp_off() const;
        [[nodiscard]] bool is_anti_blur_off() const;
        [[nodiscard]] bool is_collision_off() const;
        [[nodiscard]] bool is_subjective_camera_off() const;
    };

    [[nodiscard]] std::vector< CameraParamChunkCompat > load_camera_param_chunks(const BcsvTable& table);
    [[nodiscard]] std::optional< CameraParamChunkCompat > find_camera_param_chunk(std::span< const CameraParamChunkCompat > chunks,
                                                                                  std::string_view id);

}  // namespace smgpc::game
