#pragma once

#include "camera/StageStartCamera.hpp"
#include "camera/OriginalCameraView.hpp"
#include "compat/CameraLocalUtilRuntime.hpp"

#include <memory>

class CameraPoseParam;

namespace smgpc::camera {

    // Own the original Game camera and its arena-style allocations. This
    // boundary adapts resources and target data; the Game controllers own
    // the camera calculation and their temporal state.
    class OriginalGameCamera final {
    public:
        OriginalGameCamera(const smgpc::scene::StageZoneTransform &zone_transform,
                           const CameraParamChunk &camera_param,
                           const StageCameraTargetState &initial_target,
                           float default_fovy_degrees = 45.0F,
                           const StageCameraCalculationState &initial_state = {},
                           smgpc::compat::OriginalCameraMode mode = smgpc::compat::OriginalCameraMode::Game,
                           const CameraPoseParam *manager_seed = nullptr,
                           bool reset_local_offset = false);
        OriginalGameCamera(const smgpc::scene::StageZoneTransform &zone_transform,
                           const CameraParamChunk &camera_param, CameraTargetObj &target,
                           float default_fovy_degrees = 45.0F,
                           const CameraPoseParam *manager_seed = nullptr,
                           bool reset_local_offset = false);
        ~OriginalGameCamera();

        OriginalGameCamera(const OriginalGameCamera &) = delete;
        OriginalGameCamera &operator=(const OriginalGameCamera &) = delete;

        void reset(const StageCameraTargetState &target);
        void reset_manager(const StageCameraTargetState &target);
        [[nodiscard]] StageCameraPoseCalculation calc(const StageCameraTargetState &target);
        [[nodiscard]] StageCameraPoseCalculation calc(CameraTargetObj &target);
        [[nodiscard]] StageCameraPoseCalculation calculation() const;
        [[nodiscard]] const CameraPoseParam &pose_param() const;
        [[nodiscard]] const CameraTargetObj *target_object() const;
        [[nodiscard]] OriginalCameraViewFlags view_flags() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

}  // namespace smgpc::camera
