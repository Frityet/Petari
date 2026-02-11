#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "AssetServices.hpp"

namespace smgpc::assets {

[[nodiscard]] AssetResult<std::vector<std::byte>> unpack_packed_asset(std::span<const std::byte> packed_bytes);

}  // namespace smgpc::assets
