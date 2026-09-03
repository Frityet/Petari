#include "Game/Util/CameraUtil.hpp"

#include "Game/Camera/CameraTargetArg.hpp"
#include "Game/Camera/CameraTargetMtx.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "camera/CameraPose.hpp"
#include "camera/EventCamera.hpp"
#include "compat/CameraUtilCompat.hpp"
#include "compat/J3dSystemCompat.hpp"
#include "core/RenderTypes.hpp"
#include "runtime/RuntimeContext.hpp"

#include <dolphin/gx.h>

#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
    constexpr auto cPi = 3.14159265358979323846F;
    TPos3f sCameraViewMatrix;
    TProj3f sCameraProjectionMatrix;

    struct CameraBasis {
        smgpc::camera::CameraParamVec3 forward;
        smgpc::camera::CameraParamVec3 right;
        smgpc::camera::CameraParamVec3 up;
    };

    [[nodiscard]] std::string_view event_name(const char* pEventName) {
        return pEventName != nullptr ? std::string_view(pEventName) : std::string_view{};
    }

    [[nodiscard]] smgpc::camera::CameraParamVec3 camera_vec3(const TVec3f& value) {
        return smgpc::camera::CameraParamVec3{.x = value.x, .y = value.y, .z = value.z};
    }

    [[nodiscard]] TVec3f tv_vec3(const smgpc::camera::CameraParamVec3& value) {
        return TVec3f{value.x, value.y, value.z};
    }

    [[nodiscard]] smgpc::camera::CameraParamVec3 subtract(const smgpc::camera::CameraParamVec3& a, const smgpc::camera::CameraParamVec3& b) {
        return smgpc::camera::CameraParamVec3{
            .x = a.x - b.x,
            .y = a.y - b.y,
            .z = a.z - b.z,
        };
    }

    [[nodiscard]] smgpc::camera::CameraParamVec3 add(const smgpc::camera::CameraParamVec3& a, const smgpc::camera::CameraParamVec3& b) {
        return smgpc::camera::CameraParamVec3{
            .x = a.x + b.x,
            .y = a.y + b.y,
            .z = a.z + b.z,
        };
    }

    [[nodiscard]] smgpc::camera::CameraParamVec3 scale(const smgpc::camera::CameraParamVec3& value, f32 factor) {
        return smgpc::camera::CameraParamVec3{
            .x = value.x * factor,
            .y = value.y * factor,
            .z = value.z * factor,
        };
    }

    [[nodiscard]] smgpc::camera::CameraParamVec3 cross(const smgpc::camera::CameraParamVec3& a, const smgpc::camera::CameraParamVec3& b) {
        return smgpc::camera::CameraParamVec3{
            .x = a.y * b.z - a.z * b.y,
            .y = a.z * b.x - a.x * b.z,
            .z = a.x * b.y - a.y * b.x,
        };
    }

    [[nodiscard]] f32 dot(const smgpc::camera::CameraParamVec3& a, const smgpc::camera::CameraParamVec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    [[nodiscard]] smgpc::camera::CameraParamVec3 normalized(const smgpc::camera::CameraParamVec3& value) {
        const auto length = std::sqrt(dot(value, value));
        if (length <= 0.000001F) {
            throw std::logic_error("Cannot derive a camera basis from a degenerate real camera pose.");
        }

        return scale(value, 1.0F / length);
    }

    [[nodiscard]] CameraBasis camera_basis(const smgpc::camera::CameraPose& pose) {
        const auto forward = normalized(subtract(pose.watch, pose.eye));
        const auto right = normalized(cross(forward, pose.up));
        const auto corrected_up = normalized(cross(right, forward));
        return CameraBasis{
            .forward = forward,
            .right = right,
            .up = corrected_up,
        };
    }

    [[nodiscard]] const smgpc::camera::CameraPose* active_camera_pose() {
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr || !runtime->scene_camera_pose().has_value()) {
            return nullptr;
        }

        return &*runtime->scene_camera_pose();
    }

    [[nodiscard]] const smgpc::camera::CameraPose& require_camera_pose() {
        const auto* pose = active_camera_pose();
        if (pose == nullptr) {
            throw std::logic_error("Camera state is unavailable.");
        }

        return *pose;
    }

    void sync_active_programmable_camera_pose(smgpc::runtime::RuntimeContext& runtime, const std::optional< smgpc::camera::CameraPose >& pose) {
        if (pose.has_value()) {
            runtime.set_scene_camera_pose(*pose);
        }
    }

    void sync_active_event_camera_pose(smgpc::runtime::RuntimeContext& runtime) {
        if (const auto pose = runtime.camera_system().active_event_camera_pose()) {
            runtime.set_scene_camera_pose(*pose);
        }
    }

    [[nodiscard]] smgpc::camera::EventCameraTarget event_target(
        smgpc::runtime::RuntimeContext& runtime, const CameraTargetArg& target) {
        if (target.mTargetMtx != nullptr) {
            return smgpc::camera::EventCameraTarget::target_matrix(
                *target.mTargetMtx);
        }
        if (target.mLiveActor != nullptr) {
            return smgpc::camera::EventCameraTarget::target_actor(
                *target.mLiveActor);
        }
        if (target.mMarioActor != nullptr) {
            return smgpc::camera::EventCameraTarget::target_player(
                runtime.player_system());
        }
        if (target.mTargetObj != nullptr) {
            throw std::logic_error(
                "Arbitrary CameraTargetObj event targets are not available in the bounded host provider.");
        }
        return smgpc::camera::EventCameraTarget::retain();
    }

    [[nodiscard]] std::size_t inferred_camera_animation_size(const void* data) {
        if (data == nullptr) {
            throw std::invalid_argument(
                "Animation event camera requires a real CANM resource.");
        }
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        const auto read_u32 = [bytes](std::size_t offset) {
            return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
                   static_cast<std::uint32_t>(bytes[offset + 3U]);
        };
        constexpr auto cHeaderSize = std::size_t{0x20U};
        const auto value_offset =
            cHeaderSize + static_cast<std::size_t>(read_u32(0x1cU));
        return value_offset + 4U +
               static_cast<std::size_t>(read_u32(value_offset));
    }

    [[nodiscard]] bool project_world_to_screen(TVec3f* pResult, const TVec3f& rWorldPos) {
        if (pResult == nullptr) {
            return false;
        }

        const auto* pose = active_camera_pose();
        if (pose == nullptr) {
            return false;
        }

        const auto world = smgpc::camera::CameraParamVec3{.x = rWorldPos.x, .y = rWorldPos.y, .z = rWorldPos.z};
        const auto camera = smgpc::camera::transform_world_to_camera(*pose, world);
        const auto depth = std::abs(camera.z);
        if (depth <= 0.0001F) {
            pResult->x = 0.0F;
            pResult->y = 0.0F;
            pResult->z = camera.z;
            return false;
        }

        const auto fovy = pose->fovy_degrees * cPi / 180.0F;
        const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
        const auto focal_x = focal_y / pose->aspect_ratio;
        const auto half_width = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F;
        const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
        pResult->x = (((camera.x / camera.z) * focal_x + pose->projection_offset_x) * half_width) + half_width;
        pResult->y = (((camera.y / camera.z) * focal_y + pose->projection_offset_y) * half_height) + half_height;
        pResult->z = camera.z;
        return camera.z > pose->near_clip && camera.z < pose->far_clip;
    }

    [[nodiscard]] bool unproject_screen_to_world(TVec3f* pResult, const TVec2f& rScreenPos, f32 distance) {
        if (pResult == nullptr) {
            return false;
        }

        const auto* pose = active_camera_pose();
        if (pose == nullptr) {
            return false;
        }

        const auto basis = camera_basis(*pose);
        const auto world_distance = distance < 0.0F ? pose->near_clip : distance;
        const auto fovy = pose->fovy_degrees * cPi / 180.0F;
        const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
        const auto focal_x = focal_y / pose->aspect_ratio;
        const auto half_width = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F;
        const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
        const auto view_x = (((rScreenPos.x - half_width) / half_width - pose->projection_offset_x) / focal_x) * world_distance;
        const auto view_y = (((rScreenPos.y - half_height) / half_height - pose->projection_offset_y) / focal_y) * world_distance;

        const auto world = add(add(add(pose->eye, scale(basis.right, view_x)), scale(basis.up, view_y)), scale(basis.forward, world_distance));
        pResult->set(world.x, world.y, world.z);
        return world_distance >= pose->near_clip && world_distance <= pose->far_clip;
    }
}  // namespace

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

    smgpc::runtime::CameraSystemService &
    require_camera_system_for_camera_util(std::string_view operation) {
        auto *camera_system = active_camera_system_for_camera_util();
        if (camera_system == nullptr) {
            throw std::logic_error(std::string(operation) +
                                   " requires the active RuntimeContext camera service.");
        }
        return *camera_system;
    }

    void declare_event_camera_animation(
        const ActorCameraInfo& info, std::string_view name,
        std::span<const std::uint8_t> resource) {
        auto* camera_system = active_camera_system_for_camera_util();
        if (camera_system == nullptr) {
            throw std::logic_error(
                "Animation event-camera declaration requires the active RuntimeContext.");
        }
        camera_system->declare_event_camera_animation(
            info.mZoneID, name,
            smgpc::camera::CameraAnimation::from_bytes(resource));
    }

}  // namespace smgpc::compat

namespace MR {
    void loadProjectionMtx() {
        GXSetProjection(getCameraProjectionMtx().mMtx, GX_PERSPECTIVE);
    }

    void loadViewMtx() {
        smgpc::compat::load_j3d_view_matrix(getCameraViewMtx().mMtx);
    }

    const TProj3f& getCameraProjectionMtx() {
        const auto& pose = require_camera_pose();
        sCameraProjectionMatrix.makePerspective(pose.fovy_degrees, pose.aspect_ratio,
                                                pose.near_clip, pose.far_clip);
        sCameraProjectionMatrix.mMtx[0][2] -= pose.projection_offset_x;
        sCameraProjectionMatrix.mMtx[1][2] -= pose.projection_offset_y;
        return sCameraProjectionMatrix;
    }

    const TPos3f& getCameraViewMtx() {
        const auto &pose = require_camera_pose();
        const auto basis = camera_basis(pose);
        const auto back = scale(basis.forward, -1.0F);

        sCameraViewMatrix.mMtx[0][0] = basis.right.x;
        sCameraViewMatrix.mMtx[0][1] = basis.right.y;
        sCameraViewMatrix.mMtx[0][2] = basis.right.z;
        sCameraViewMatrix.mMtx[0][3] = -dot(basis.right, pose.eye);
        sCameraViewMatrix.mMtx[1][0] = basis.up.x;
        sCameraViewMatrix.mMtx[1][1] = basis.up.y;
        sCameraViewMatrix.mMtx[1][2] = basis.up.z;
        sCameraViewMatrix.mMtx[1][3] = -dot(basis.up, pose.eye);
        sCameraViewMatrix.mMtx[2][0] = back.x;
        sCameraViewMatrix.mMtx[2][1] = back.y;
        sCameraViewMatrix.mMtx[2][2] = back.z;
        sCameraViewMatrix.mMtx[2][3] = -dot(back, pose.eye);
        return sCameraViewMatrix;
    }

    const TVec3f getCamPos() {
        return tv_vec3(require_camera_pose().eye);
    }

    TVec3f getCamXdir() {
        return tv_vec3(camera_basis(require_camera_pose()).right);
    }

    TVec3f getCamYdir() {
        return tv_vec3(camera_basis(require_camera_pose()).up);
    }

    TVec3f getCamZdir() {
        return tv_vec3(camera_basis(require_camera_pose()).forward);
    }

    f32 getAspect() {
        return require_camera_pose().aspect_ratio;
    }

    f32 getNearZ() {
        return require_camera_pose().near_clip;
    }

    f32 getFarZ() {
        return require_camera_pose().far_clip;
    }

    f32 getFovy() {
        return require_camera_pose().fovy_degrees;
    }

    void startStartPosCamera(bool immediate) {
        smgpc::compat::require_camera_system_for_camera_util(
            "Start-position camera restore")
            .start_start_position_camera(immediate);
    }

    void endStartPosCamera() {
        smgpc::compat::require_camera_system_for_camera_util(
            "Start-position camera termination")
            .end_start_position_camera();
    }

    bool isStartPosCameraEnd() {
        return smgpc::compat::require_camera_system_for_camera_util(
                   "Start-position camera state query")
            .is_start_position_camera_end();
    }

    bool isStartAnimCameraEnd() {
        throw std::logic_error(
            "Start-animation camera completion is unavailable without the exact CameraDirector start-camera owner.");
    }

    void resetCameraMan() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().reset_camera_man();
        }
    }

    void pauseOnCameraDirector() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().pause_on_camera_director();
        }
    }

    void pauseOffCameraDirector() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().pause_off_camera_director();
        }
    }

    void declareEventCamera(const ActorCameraInfo* pInfo,
                            const char* pEventName) {
        if (pInfo == nullptr) {
            throw std::invalid_argument(
                "Event-camera declaration requires ActorCameraInfo.");
        }
        if (auto* camera_system =
                smgpc::compat::active_camera_system_for_camera_util()) {
            camera_system->declare_event_camera(pInfo->mZoneID,
                                                event_name(pEventName));
        }
    }

    void declareEventCameraAnim(const ActorCameraInfo* pInfo,
                                const char* pEventName, void* pData) {
        if (pInfo == nullptr) {
            throw std::invalid_argument(
                "Animation event-camera declaration requires ActorCameraInfo.");
        }
        smgpc::compat::declare_event_camera_animation(
            *pInfo, event_name(pEventName),
            std::span<const std::uint8_t>(
                static_cast<const std::uint8_t*>(pData),
                inferred_camera_animation_size(pData)));
    }

    void startEventCamera(const ActorCameraInfo* pInfo,
                          const char* pEventName,
                          const CameraTargetArg& rTarget, s32 frames) {
        if (pInfo == nullptr) {
            throw std::invalid_argument(
                "Event-camera start requires ActorCameraInfo.");
        }
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().start_event_camera(
                pInfo->mZoneID, event_name(pEventName),
                event_target(*runtime, rTarget), frames);
            sync_active_event_camera_pose(*runtime);
        }
    }

    void startEventCameraNoTarget(const ActorCameraInfo* pInfo,
                                  const char* pEventName, s32 frames) {
        startEventCamera(pInfo, pEventName, CameraTargetArg{}, frames);
    }

    void startEventCameraTargetPlayer(const ActorCameraInfo* pInfo,
                                      const char* pEventName, s32 frames) {
        if (pInfo == nullptr) {
            throw std::invalid_argument(
                "Player-target event-camera start requires ActorCameraInfo.");
        }
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().start_event_camera(
                pInfo->mZoneID, event_name(pEventName),
                smgpc::camera::EventCameraTarget::target_player(
                    runtime->player_system()),
                frames);
            sync_active_event_camera_pose(*runtime);
        }
    }

    void startEventCameraAnim(const ActorCameraInfo* pInfo,
                              const char* pEventName,
                              const CameraTargetArg& rTarget, s32 frames,
                              f32 speed) {
        if (pInfo == nullptr) {
            throw std::invalid_argument(
                "Animation event-camera start requires ActorCameraInfo.");
        }
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().start_event_camera(
                pInfo->mZoneID, event_name(pEventName),
                event_target(*runtime, rTarget), frames, speed);
            sync_active_event_camera_pose(*runtime);
        }
    }

    void endEventCamera(const ActorCameraInfo* pInfo, const char* pEventName,
                        bool endForce, s32 frames) {
        if (pInfo == nullptr) {
            throw std::invalid_argument(
                "Event-camera end requires ActorCameraInfo.");
        }
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().end_event_camera(
                pInfo->mZoneID, event_name(pEventName), endForce, frames);
            const auto pose = runtime->camera_system().active_event_camera_pose()
                                  ? runtime->camera_system().active_event_camera_pose()
                              : runtime->camera_system().active_programmable_camera_pose()
                                  ? runtime->camera_system().active_programmable_camera_pose()
                                  : runtime->camera_system().game_camera_pose();
            if (pose.has_value()) {
                runtime->set_scene_camera_pose(*pose);
            }
        }
    }

    void endEventCameraAtLanding(const ActorCameraInfo*, const char*, s32) {
        throw std::logic_error(
            "Landing-delayed event-camera release requires the unavailable retail landing owner.");
    }

    bool isEventCameraActive() {
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr &&
               runtime->camera_system().active_event_camera_key().has_value();
    }

    bool isEventCameraActive(const ActorCameraInfo* pInfo,
                             const char* pEventName) {
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr && pInfo != nullptr &&
               runtime->camera_system().is_event_camera_active(
                   pInfo->mZoneID, event_name(pEventName));
    }

    bool isAnimCameraEnd(const ActorCameraInfo* pInfo,
                         const char* pEventName) {
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime == nullptr || pInfo == nullptr ||
               runtime->camera_system().is_event_camera_animation_end(
                   pInfo->mZoneID, event_name(pEventName));
    }

    s32 getAnimCameraFrame(const ActorCameraInfo* pInfo,
                           const char* pEventName) {
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr && pInfo != nullptr
                   ? runtime->camera_system().event_camera_animation_frame(
                         pInfo->mZoneID, event_name(pEventName))
                   : 0;
    }

    u32 getEventCameraFrames(const ActorCameraInfo* pInfo,
                             const char* pEventName) {
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr && pInfo != nullptr
                   ? static_cast<u32>(runtime->camera_system().event_camera_frames(
                         pInfo->mZoneID, event_name(pEventName)))
                   : 0U;
    }

    void setCameraTargetToPlayer(CameraTargetArg* pTarget) {
        if (pTarget == nullptr) {
            return;
        }
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        pTarget->mTargetObj = nullptr;
        pTarget->mTargetMtx = nullptr;
        pTarget->mLiveActor = nullptr;
        pTarget->mMarioActor =
            runtime != nullptr
                ? reinterpret_cast<MarioActor*>(
                      runtime->player_system().attached_actor())
                : nullptr;
    }

    void declareEventCameraProgrammable(const char* pEventName) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().declare_event_camera_programmable(event_name(pEventName));
        }
    }

    void startGlobalEventCameraNoTarget(const char* pEventName, s32 frames) {
        (void)frames;
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().start_global_event_camera_no_target(event_name(pEventName));
            sync_active_programmable_camera_pose(*runtime, runtime->camera_system().active_programmable_camera_pose());
        }
    }

    void endGlobalEventCamera(const char* pEventName, s32 frames, bool endForce) {
        (void)frames;
        (void)endForce;
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().end_global_event_camera(event_name(pEventName));
        }
    }

    void setProgrammableCameraParam(const char* pEventName, const TVec3f& rWPoint, const TVec3f& rEye, const TVec3f& rUpVec, bool doZeroWOffset) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            sync_active_programmable_camera_pose(
                *runtime, runtime->camera_system().set_programmable_camera_param(event_name(pEventName), camera_vec3(rWPoint), camera_vec3(rEye),
                                                                                 camera_vec3(rUpVec), doZeroWOffset));
        }
    }

    void setProgrammableCameraParamFovy(const char* pEventName, f32 fovy) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            sync_active_programmable_camera_pose(*runtime, runtime->camera_system().set_programmable_camera_fovy(event_name(pEventName), fovy));
        }
    }

    bool calcScreenPosition(TVec2f* pResult, const TVec3f& rWorldPos) {
        if (active_camera_pose() == nullptr) {
            return false;
        }

        auto screen_pos = TVec3f{};
        const auto visible = project_world_to_screen(&screen_pos, rWorldPos);
        if (pResult != nullptr) {
            pResult->x = screen_pos.x;
            pResult->y = screen_pos.y;
        }
        return visible;
    }

    bool calcScreenPosition(TVec3f* pResult, const TVec3f& rWorldPos) {
        return project_world_to_screen(pResult, rWorldPos);
    }

    bool calcNormalizedScreenPosition(TVec3f* pResult, const TVec3f& rWorldPos) {
        if (active_camera_pose() == nullptr) {
            return false;
        }

        auto screen_pos = TVec3f{};
        const auto visible = calcScreenPosition(&screen_pos, rWorldPos);
        if (pResult != nullptr) {
            const auto half_width = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F;
            const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
            pResult->x = (screen_pos.x - half_width) / half_width;
            pResult->y = (screen_pos.y - half_height) / half_height;
            pResult->z = screen_pos.z;
        }
        return visible;
    }

    bool calcNormalizedScreenPositionFromView(TVec3f* pResult, const TVec3f& rViewPos) {
        const auto* pose = active_camera_pose();
        if (pResult == nullptr || pose == nullptr) {
            return false;
        }

        if (std::abs(rViewPos.z) <= 0.0001F) {
            pResult->set(0.0F, 0.0F, rViewPos.z);
            return false;
        }

        const auto fovy = pose->fovy_degrees * cPi / 180.0F;
        const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
        const auto focal_x = focal_y / pose->aspect_ratio;
        pResult->x = (rViewPos.x / rViewPos.z) * focal_x;
        pResult->y = (rViewPos.y / rViewPos.z) * focal_y;
        pResult->z = rViewPos.z;
        return std::abs(pResult->x) <= 1.0F && std::abs(pResult->y) <= 1.0F &&
               rViewPos.z >= pose->near_clip && rViewPos.z <= pose->far_clip;
    }

    bool calcWorldPositionFromScreen(TVec3f* pResult, const TVec2f& rScreenPos, f32 distance) {
        return unproject_screen_to_world(pResult, rScreenPos, distance);
    }

    bool calcWorldPositionFromCenterScreen(TVec3f* pResult, const TVec2f& rCenterScreenPos, f32 distance) {
        const auto screen_pos = TVec2f{rCenterScreenPos.x + (static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F),
                                       rCenterScreenPos.y + (static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F)};
        return calcWorldPositionFromScreen(pResult, screen_pos, distance);
    }

    bool calcWorldRayDirectionFromScreen(TVec3f* pResult, const TVec2f& rScreenPos) {
        const auto ret = unproject_screen_to_world(pResult, rScreenPos, -1.0F);
        if (ret && pResult != nullptr) {
            const auto cam_pos = getCamPos();
            pResult->x -= cam_pos.x;
            pResult->y -= cam_pos.y;
            pResult->z -= cam_pos.z;
        }
        return ret;
    }

    f32 calcCameraDistanceZ(const TVec3f& rWorldPos) {
        const auto camera = smgpc::camera::transform_world_to_camera(require_camera_pose(), camera_vec3(rWorldPos));
        return std::abs(camera.z);
    }
}  // namespace MR
