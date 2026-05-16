#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "TplTexture.hpp"

namespace smgpc::game {

struct J3dTexture {
    std::string name;
    std::uint8_t wrap_s = 0U;
    std::uint8_t wrap_t = 0U;
    DecodedTexture image;
};

[[nodiscard]] std::vector<J3dTexture> extract_j3d_textures(std::span<const std::uint8_t> model_data);

}  // namespace smgpc::game
