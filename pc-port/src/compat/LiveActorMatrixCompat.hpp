#pragma once

#include "render/J3dMaterialRuntime.hpp"

class LiveActor;

namespace MR {
    void setBaseTRMtx(LiveActor* actor, const smgpc::render::J3dMatrix3x4& matrix);
}
