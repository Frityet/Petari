#pragma once

#include <cstdint>
#include <span>
#include <string_view>

class ActorCameraInfo;

namespace smgpc::compat {

    void declare_event_camera_animation(
        const ActorCameraInfo &info, std::string_view name,
        std::span<const std::uint8_t> resource);

}  // namespace smgpc::compat
