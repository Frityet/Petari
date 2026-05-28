#pragma once

#include "camera/CameraParam.hpp"

namespace smgpc::camera {

    struct CameraPose {
        CameraParamVec3 eye = {};
        CameraParamVec3 watch = {};
        CameraParamVec3 up = {0.0F, 1.0F, 0.0F};
        float fovy_degrees = 45.0F;
        float aspect_ratio = 608.0F / 456.0F;
        float near_clip = 100.0F;
        float far_clip = 800000.0F;
    };

    struct CameraViewPoint {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
    };

    [[nodiscard]] CameraViewPoint transform_world_to_camera(const CameraPose &pose, const CameraParamVec3 &world);

}  // namespace smgpc::camera
