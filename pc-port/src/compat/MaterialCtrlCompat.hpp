#pragma once

class LiveActor;
class ProjmapEffectMtxSetter;

namespace smgpc::compat {
    ProjmapEffectMtxSetter* create_projmap_effect_mtx_setter(LiveActor* actor);
    void release_actor_material_ctrl_state(const LiveActor* actor);
}
