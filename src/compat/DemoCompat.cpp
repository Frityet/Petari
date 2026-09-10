#include "Game/Util/ScreenUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoSceneRuntime.hpp"

namespace smgpc::compat {

    void release_demo_runtime_state(const LiveActor *actor) {
        release_actor_from_all_demo_scenes(actor);
    }

    bool has_registered_demo_cast(const LiveActor *actor) {
        return has_any_demo_scene_cast(actor);
    }

    std::size_t registered_demo_membership_count(const LiveActor *actor) {
        return demo_scene_membership_count(actor);
    }

    std::size_t registered_demo_action_count(const LiveActor *actor) {
        return demo_scene_action_count(actor);
    }

}  // namespace smgpc::compat

namespace MR {
    void timeKeepDemoFadeOut() {
        MR::closeWipeFade(60);
    }

    void timeKeepDemoFadeIn() {
        MR::openWipeFade(60);
    }
}  // namespace MR
