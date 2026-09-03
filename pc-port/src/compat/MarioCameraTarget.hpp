#pragma once

#include "camera/StageStartCamera.hpp"

class MarioActor;

namespace smgpc::compat {

    [[nodiscard]] camera::StageCameraTargetState mario_camera_target(const MarioActor &actor);

}  // namespace smgpc::compat
