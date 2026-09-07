#include "scene/StageEventCameraBinding.hpp"

#include "runtime/RuntimeServices.hpp"

#include <utility>

namespace smgpc::scene {

    StageEventCameraBinding::StageEventCameraBinding(
        smgpc::runtime::CameraSystemService &camera_system,
        smgpc::runtime::DvdFileSystemService &dvd,
        std::span<const StagePlacementTable> tables)
        : _camera_system(&camera_system),
          _catalog(smgpc::camera::EventCameraCatalog::from_stage_tables(dvd,
                                                                        tables)) {
        _camera_system->attach_event_camera_catalog(_catalog);
    }

    StageEventCameraBinding::~StageEventCameraBinding() {
        if (auto *camera_system = std::exchange(_camera_system, nullptr);
            camera_system != nullptr) {
            camera_system->detach_event_camera_catalog(_catalog);
        }
    }

    const smgpc::camera::EventCameraCatalog &StageEventCameraBinding::catalog()
        const noexcept {
        return _catalog;
    }

}  // namespace smgpc::scene
