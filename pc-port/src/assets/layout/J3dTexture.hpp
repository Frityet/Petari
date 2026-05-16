#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "AssetServices.hpp"
#include "Tpl.hpp"

namespace smgpc::assets::layout {

struct J3dTexture {
    std::string name {};
    tpl::DecodedImage image {};
    std::uint8_t wrap_s {};
    std::uint8_t wrap_t {};
};

[[nodiscard]] AssetResult<std::vector<J3dTexture>> parse_j3d_tex1_textures(std::span<const std::byte> bdl_bytes);

}  // namespace smgpc::assets::layout
