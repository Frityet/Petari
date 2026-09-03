#pragma once

#include "camera/CameraPose.hpp"
#include "compat/CameraViewRuntime.hpp"

#include <cstdint>
#include <memory>

class CameraPoseParam;
class CameraMan;
class CameraTargetObj;
class CameraViewInterpolator;
class OnlyCamera;

namespace smgpc::camera {

    struct OriginalCameraViewFlags {
        bool interpolation_off = false;
        bool collision_off = false;
        bool correcting_position_off = false;
        bool zero_frame_move_off = false;
    };

    // Own the original view interpolator and its stage-arena allocations.
    // Camera controllers retain their raw manager poses; rendering uses this
    // separate, persistent CameraDirector view phase.
    class OriginalCameraView final {
    public:
        OriginalCameraView();
        ~OriginalCameraView();
        OriginalCameraView(const OriginalCameraView &) = delete;
        OriginalCameraView &operator=(const OriginalCameraView &) = delete;

        [[nodiscard]] CameraPose update(const CameraPoseParam &pose, const CameraTargetObj *target,
                                        const CameraPose &projection, OriginalCameraViewFlags flags = {});
        [[nodiscard]] CameraPose update(CameraMan &manager, const CameraTargetObj *target,
                                        const CameraPose &projection, OriginalCameraViewFlags flags = {});
        void request_pose_reset();
        [[nodiscard]] OnlyCamera &pose_processor();
        [[nodiscard]] const CameraPoseParam &processed_pose() const;
        void set_interpolation(std::uint32_t frames, bool start_camera_active = false);
        [[nodiscard]] CameraViewInterpolator &original();
        [[nodiscard]] const CameraViewInterpolator &original() const;
        [[nodiscard]] const smgpc::compat::CameraViewOutput &output() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}  // namespace smgpc::camera
