#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "AssetServices.hpp"

namespace smgpc::assets::layout {

[[nodiscard]] bool is_yaz0(std::span<const std::byte> bytes);
[[nodiscard]] AssetResult<std::vector<std::byte>> decode_yaz0(std::span<const std::byte> bytes);

}  // namespace smgpc::assets::layout
