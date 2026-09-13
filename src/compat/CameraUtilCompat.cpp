#include "compat/CameraUtilCompat.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/SceneScheduler.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/CameraUtil.hpp"

#include <aurora/exception.hpp>
#include <stdexcept>
#include <string>

namespace smgpc::compat {

    void declare_event_camera_animation(
        const ActorCameraInfo& info, std::string_view name,
        std::span<const std::uint8_t> resource) {
        auto* owner = smgpc::camera::current_camera_director_runtime();
        if (owner == nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Animation event-camera declaration requires the original scene camera owner.");
        }
        JkrHostAllocationScope host;
        const std::string event_name(name);
        void* animation = owner->retain_animation(resource);
        auto* scheduler = smgpc::runtime::try_active_scene_scheduler();
        if (scheduler == nullptr || !scheduler->allocation_domain()) {
            aurora::throw_host_exception<std::logic_error>(
                "Animation camera parameters require the original scene Game heap.");
        }
        JkrAllocationScope game(scheduler->allocation_domain());
        MR::declareEventCameraAnim(&info, event_name.c_str(), animation);
    }

}  // namespace smgpc::compat
