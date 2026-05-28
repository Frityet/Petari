#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "resource/TplTexture.hpp"

namespace smgpc::compat {

    struct J3dTexture {
        std::string name;
        std::uint8_t transparency = 0U;
        std::uint8_t wrap_s = 0U;
        std::uint8_t wrap_t = 0U;
        std::uint8_t palette_format = 0U;
        std::uint16_t palette_entry_count = 0U;
        std::uint32_t palette_data_offset = 0U;
        bool mipmap = false;
        bool do_edge_lod = false;
        bool bias_clamp = false;
        std::uint8_t max_anisotropy = 0U;
        std::uint8_t min_filter = 0U;
        std::uint8_t mag_filter = 0U;
        std::uint8_t min_lod = 0U;
        std::uint8_t max_lod = 0U;
        std::uint8_t image_count = 1U;
        std::int16_t lod_bias = 0;
        std::uint32_t image_data_offset = 0U;
        DecodedTexture image;
    };

    [[nodiscard]] std::vector<J3dTexture> extract_j3d_textures(std::span<const std::uint8_t> model_data);

}  // namespace smgpc::compat
