#include "compat/CameraUtilCompat.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/RuntimeContext.hpp"
#include "Game/LiveActor/MirrorCamera.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/CameraUtil.hpp"

#include <aurora/exception.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace smgpc::compat {

    namespace {
        thread_local smgpc::runtime::CameraSystemService *
            sCameraSystemOverride = nullptr;
    }

    ScopedCameraSystemServiceOverride::ScopedCameraSystemServiceOverride(
        smgpc::runtime::CameraSystemService &service)
        : _previous(std::exchange(sCameraSystemOverride, &service)) {
    }

    ScopedCameraSystemServiceOverride::~ScopedCameraSystemServiceOverride() {
        sCameraSystemOverride = _previous;
    }

    smgpc::runtime::CameraSystemService *
    active_camera_system_for_camera_util() {
        if (sCameraSystemOverride != nullptr) {
            return sCameraSystemOverride;
        }
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr ? &runtime->camera_system() : nullptr;
    }

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

namespace MR {
    MirrorCamera* getMirrorCamera() {
        if (!MR::isExistSceneObj(SceneObj_MirrorCamera))
            aurora::throw_host_exception<std::logic_error>("Mirror camera access requires the actual scene MirrorCamera owner");
        return MR::getSceneObj<MirrorCamera>(SceneObj_MirrorCamera);
    }
}
