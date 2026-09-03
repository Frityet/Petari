#pragma once

#include "camera/StageStartCamera.hpp"

#include <memory>

class CameraPoseParam;

namespace smgpc::camera {

    // Own the original Game camera and its arena-style allocations. This
    // boundary adapts resources and target data; CameraParallel owns the
    // camera calculation and CameraHeightArrange owns its temporal state.
    class OriginalGameCamera final {
    public:
        OriginalGameCamera(const smgpc::scene::StageZoneTransform &zone_transform,
                           const CameraParamChunk &camera_param,
                           const StageCameraTargetState &initial_target,
                           float default_fovy_degrees = 45.0F,
                           const StageCameraCalculationState &initial_state = {});
        ~OriginalGameCamera();

        OriginalGameCamera(const OriginalGameCamera &) = delete;
        OriginalGameCamera &operator=(const OriginalGameCamera &) = delete;

        void reset(const StageCameraTargetState &target);
        void reset_manager(const StageCameraTargetState &target);
        [[nodiscard]] StageCameraPoseCalculation calc(const StageCameraTargetState &target);
        [[nodiscard]] StageCameraPoseCalculation calculation() const;
        [[nodiscard]] const CameraPoseParam &pose_param() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

}  // namespace smgpc::camera
