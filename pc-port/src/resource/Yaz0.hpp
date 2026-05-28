#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace smgpc::resource {

    [[nodiscard]] bool is_yaz0(std::span<const std::uint8_t> data);
    [[nodiscard]] std::vector<std::uint8_t> decompress_yaz0(std::span<const std::uint8_t> data);

}  // namespace smgpc::resource
