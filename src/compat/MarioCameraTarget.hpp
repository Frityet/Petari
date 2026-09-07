#pragma once

#include "Game/Camera/CameraTargetObj.hpp"

#include <memory>

namespace smgpc::compat {

    [[nodiscard]] std::unique_ptr<CameraTargetObj> create_mario_camera_target(const MarioActor &actor);
    [[nodiscard]] MtxPtr mario_camera_base_matrix(const MarioActor &actor);

}  // namespace smgpc::compat
