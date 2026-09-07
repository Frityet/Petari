#pragma once

#include "camera/EventCamera.hpp"

#include <span>

namespace smgpc::runtime {
    class CameraSystemService;
    class DvdFileSystemService;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    class StageEventCameraBinding final {
    public:
        StageEventCameraBinding(
            smgpc::runtime::CameraSystemService &camera_system,
            smgpc::runtime::DvdFileSystemService &dvd,
            std::span<const StagePlacementTable> tables);
        ~StageEventCameraBinding();

        StageEventCameraBinding(const StageEventCameraBinding &) = delete;
        StageEventCameraBinding &operator=(const StageEventCameraBinding &) =
            delete;

        [[nodiscard]] const smgpc::camera::EventCameraCatalog &catalog() const
            noexcept;

    private:
        smgpc::runtime::CameraSystemService *_camera_system = nullptr;
        smgpc::camera::EventCameraCatalog _catalog{};
    };

}  // namespace smgpc::scene
