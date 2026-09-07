#pragma once

#include <cstdint>
#include <span>
#include <string_view>

class ActorCameraInfo;

namespace smgpc::runtime {
    class CameraSystemService;
}

namespace smgpc::compat {

    class ScopedCameraSystemServiceOverride final {
    public:
        explicit ScopedCameraSystemServiceOverride(
            smgpc::runtime::CameraSystemService &service);
        ~ScopedCameraSystemServiceOverride();

        ScopedCameraSystemServiceOverride(
            const ScopedCameraSystemServiceOverride &) = delete;
        ScopedCameraSystemServiceOverride &operator=(
            const ScopedCameraSystemServiceOverride &) = delete;

    private:
        smgpc::runtime::CameraSystemService *_previous = nullptr;
    };

    [[nodiscard]] smgpc::runtime::CameraSystemService *
    active_camera_system_for_camera_util();

    void declare_event_camera_animation(
        const ActorCameraInfo &info, std::string_view name,
        std::span<const std::uint8_t> resource);

}  // namespace smgpc::compat
