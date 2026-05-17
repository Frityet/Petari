#include "Game/Util/CameraUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace {
    [[nodiscard]] std::string_view event_name(const char* pEventName) {
        return pEventName != nullptr ? std::string_view(pEventName) : std::string_view{};
    }

    [[nodiscard]] smgpc::game::CameraParamVec3 camera_vec3(const TVec3f& value) {
        return smgpc::game::CameraParamVec3{.x = value.x, .y = value.y, .z = value.z};
    }

    void sync_active_programmable_camera_pose(smgpc::game::RuntimeContext& runtime, const std::optional<smgpc::game::CameraPoseCompat>& pose) {
        if (pose.has_value()) {
            runtime.set_scene_camera_pose(*pose);
        }
    }
}  // namespace

namespace MR {
    void resetCameraMan() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->camera_system().reset_camera_man();
        }
    }

    void pauseOnCameraDirector() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->camera_system().pause_on_camera_director();
        }
    }

    void pauseOffCameraDirector() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->camera_system().pause_off_camera_director();
        }
    }

    void declareEventCameraProgrammable(const char* pEventName) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->camera_system().declare_event_camera_programmable(event_name(pEventName));
        }
    }

    void startGlobalEventCameraNoTarget(const char* pEventName, s32 frames) {
        (void)frames;
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->camera_system().start_global_event_camera_no_target(event_name(pEventName));
            sync_active_programmable_camera_pose(*runtime, runtime->camera_system().active_programmable_camera_pose());
        }
    }

    void endGlobalEventCamera(const char* pEventName, s32 frames, bool endForce) {
        (void)frames;
        (void)endForce;
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->camera_system().end_global_event_camera(event_name(pEventName));
        }
    }

    void setProgrammableCameraParam(const char* pEventName, const TVec3f& rWPoint, const TVec3f& rEye, const TVec3f& rUpVec,
                                    bool doZeroWOffset) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            sync_active_programmable_camera_pose(
                *runtime, runtime->camera_system().set_programmable_camera_param(event_name(pEventName), camera_vec3(rWPoint), camera_vec3(rEye),
                                                                                 camera_vec3(rUpVec), doZeroWOffset));
        }
    }

    void setProgrammableCameraParamFovy(const char* pEventName, f32 fovy) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            sync_active_programmable_camera_pose(*runtime,
                                                 runtime->camera_system().set_programmable_camera_fovy(event_name(pEventName), fovy));
        }
    }
}  // namespace MR
