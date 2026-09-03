#pragma once

#include "render/J3dMatrix.hpp"

class LiveActor;
class ProjmapEffectMtxSetter;

namespace smgpc::compat {
    ProjmapEffectMtxSetter* create_projmap_effect_mtx_setter(LiveActor* actor);
    void release_actor_material_ctrl_state(const LiveActor* actor);
    const smgpc::render::J3dMatrix3x4& projmap_effect_matrix(const ProjmapEffectMtxSetter* controller);
}
