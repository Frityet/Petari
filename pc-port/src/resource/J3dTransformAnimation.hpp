#pragma once

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>

#include <cstdint>
#include <memory>
#include <span>

namespace smgpc::resource {

    // Decode BCK/ANK1 or BCA/ANF1 into an original J3D transform animation.
    // The returned object owns its native-endian tables and values independently
    // of data. Full animation interpolation corresponds to the original loader
    // flag; it has no effect on keyed animations. Invalid files throw.
    [[nodiscard]] std::unique_ptr<J3DAnmTransform>
    load_j3d_transform_animation(std::span<const std::uint8_t> data,
                                 bool interpolate_full = false);

}  // namespace smgpc::resource
