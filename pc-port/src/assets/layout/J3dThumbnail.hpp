#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "AssetServices.hpp"
#include "Tpl.hpp"

namespace smgpc::assets::layout {

struct J3dThumbnailOptions {
    std::uint16_t width {128U};
    std::uint16_t height {128U};
    float yaw_degrees {};
    float pitch_degrees {};
    float margin {0.90F};
    float ambient_light {0.48F};
    float diffuse_light {0.52F};
    float color_scale_r {1.0F};
    float color_scale_g {1.0F};
    float color_scale_b {1.0F};
};

[[nodiscard]] AssetResult<tpl::DecodedImage> render_j3d_thumbnail(std::span<const std::byte> bdl_bytes, const J3dThumbnailOptions &options);

}  // namespace smgpc::assets::layout
