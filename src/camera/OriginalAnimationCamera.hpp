#pragma once

#include "camera/CameraAnimation.hpp"
#include "camera/StageStartCamera.hpp"
#include "camera/OriginalCameraView.hpp"

#include <memory>

class CameraPoseParam;
class CameraMan;
class CameraTargetObj;

namespace smgpc::camera {

    // Own the original CANM/CKAN controller and its decoded resource. Requests
    // construct/reset it; only calc advances the game's animation cursor.
    class OriginalAnimationCamera final {
    public:
        OriginalAnimationCamera(const CameraAnimation &animation,
                                const StageCameraTargetState &target, float speed,
                                const CameraPoseParam *manager_seed = nullptr,
                                const TPos3f *manager_matrix_seed = nullptr);
        OriginalAnimationCamera(const CameraAnimation &animation, CameraTargetObj &target,
                                float speed, const CameraPoseParam *manager_seed = nullptr,
                                const TPos3f *manager_matrix_seed = nullptr);
        ~OriginalAnimationCamera();

        OriginalAnimationCamera(const OriginalAnimationCamera &) = delete;
        OriginalAnimationCamera &operator=(const OriginalAnimationCamera &) = delete;

        [[nodiscard]] CameraPose calc(const StageCameraTargetState &target);
        [[nodiscard]] CameraPose calc(CameraTargetObj &target);
        [[nodiscard]] CameraPose pose() const;
        [[nodiscard]] const CameraPoseParam &pose_param() const;
        [[nodiscard]] CameraMan &manager();
        [[nodiscard]] const CameraTargetObj *target_object() const;
        [[nodiscard]] OriginalCameraViewFlags view_flags() const;
        [[nodiscard]] float current_frame() const;
        [[nodiscard]] bool is_end() const;
        void set_speed(float speed);
        void set_paused(bool paused);

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

}  // namespace smgpc::camera
