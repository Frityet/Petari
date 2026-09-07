#pragma once

#include <JSystem/JGeometry/TMatrix.hpp>

#include <optional>

class Camera;
class CameraPoseParam;
class CameraTargetObj;

namespace smgpc::compat {

    enum class OriginalCameraMode {
        Game,
        Event,
        Subjective,
    };

    // Bind a real target only for the duration of an original camera's
    // reset/calc call. Nested calculations restore their caller's target.
    class ScopedCameraTargetBinding final {
    public:
        ScopedCameraTargetBinding(Camera& camera, CameraTargetObj& target, OriginalCameraMode mode);
        ~ScopedCameraTargetBinding();

        ScopedCameraTargetBinding(const ScopedCameraTargetBinding&) = delete;
        ScopedCameraTargetBinding& operator=(const ScopedCameraTargetBinding&) = delete;
        ScopedCameraTargetBinding(ScopedCameraTargetBinding&&) = delete;
        ScopedCameraTargetBinding& operator=(ScopedCameraTargetBinding&&) = delete;

    private:
        Camera* _previous_camera;
        CameraTargetObj* _previous_target;
        std::optional<OriginalCameraMode> _previous_mode;
    };

    void calcCameraViewMtxFromPoseParam(TPos3f*, const CameraPoseParam*);

}  // namespace smgpc::compat
