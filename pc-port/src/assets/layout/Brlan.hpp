#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "AssetServices.hpp"

namespace smgpc::assets::layout {

enum class BrlanCurveType : std::uint8_t {
    Step = 1,
    Hermite = 2,
};

struct BrlanKey {
    float frame {};
    float value {};
    float slope {};
};

struct BrlanTrack {
    std::string pane_name {};
    std::string kind {};
    std::uint16_t target {};
    std::uint8_t layer {};
    BrlanCurveType curve_type {BrlanCurveType::Hermite};
    std::vector<BrlanKey> keys {};

    [[nodiscard]] float sample(float frame) const;
};

struct BrlanAnimation {
    std::string name {};
    std::uint16_t frame_size {};
    bool loop {};
    std::vector<std::string> texture_names {};
    std::vector<BrlanTrack> tracks {};

    [[nodiscard]] float normalize_frame(float frame) const;
};

[[nodiscard]] AssetResult<BrlanAnimation> parse_brlan(std::span<const std::byte> bytes, std::string default_name = {});

}  // namespace smgpc::assets::layout
