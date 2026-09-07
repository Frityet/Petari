#include "Game/Util/SceneUtil.hpp"
#include "scene/SceneInitializationState.hpp"

namespace MR {

    void setInitializeStatePlacementPlayer() {
        smgpc::scene::set_scene_initialization_state(SceneInitializeState_PlacementPlayer);
    }

    void setInitializeStatePlacementHighPriority() {
        smgpc::scene::set_scene_initialization_state(SceneInitializeState_PlacementHighPriority);
    }

    void setInitializeStatePlacement() {
        smgpc::scene::set_scene_initialization_state(SceneInitializeState_Placement);
    }

    void setInitializeStateAfterPlacement() {
        smgpc::scene::set_scene_initialization_state(SceneInitializeState_AfterPlacement);
    }

    bool isInitializeStateEnd() {
        return smgpc::scene::current_scene_initialization_state() == SceneInitializeState_End;
    }

    bool isInitializeStatePlacementSomething() {
        return smgpc::scene::current_scene_initialization_state() == SceneInitializeState_PlacementPlayer ||
               smgpc::scene::current_scene_initialization_state() == SceneInitializeState_PlacementHighPriority ||
               smgpc::scene::current_scene_initialization_state() == SceneInitializeState_Placement;
    }

}  // namespace MR
