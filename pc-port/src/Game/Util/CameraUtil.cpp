#include "Game/Util/CameraUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "camera/CameraPose.hpp"
#include "core/RenderTypes.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cmath>

namespace {
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

    [[nodiscard]] smgpc::camera::CameraParamVec3 normalized_or(const smgpc::camera::CameraParamVec3& value,
                                                               const smgpc::camera::CameraParamVec3& fallback) {
        const auto length = std::sqrt(dot(value, value));
        if (length <= 0.000001F) {
            return fallback;
        }

        return scale(value, 1.0F / length);
    }

    [[nodiscard]] CameraBasis camera_basis(const smgpc::camera::CameraPose& pose) {
        const auto forward = normalized_or(subtract(pose.watch, pose.eye), {0.0F, 0.0F, 1.0F});
        const auto right = normalized_or(cross(forward, pose.up), {1.0F, 0.0F, 0.0F});
        const auto corrected_up = normalized_or(cross(right, forward), {0.0F, 1.0F, 0.0F});
        return CameraBasis{
            .forward = forward,
            .right = right,
            .up = corrected_up,
        };
    }

    void sync_active_programmable_camera_pose(smgpc::runtime::RuntimeContext& runtime, const std::optional< smgpc::camera::CameraPose >& pose) {
        if (pose.has_value()) {
            runtime.set_scene_camera_pose(*pose);
        }
    }

    [[nodiscard]] bool project_world_to_screen(TVec3f* pResult, const TVec3f& rWorldPos) {
        if (pResult == nullptr) {
            return false;
        }

        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr || !runtime->scene_camera_pose().has_value()) {
            pResult->x = 0.0F;
            pResult->y = 0.0F;
            pResult->z = 0.0F;
            return false;
        }

        constexpr auto PI = 3.14159265358979323846F;
        const auto& pose = *runtime->scene_camera_pose();
        const auto world = smgpc::camera::CameraParamVec3{.x = rWorldPos.x, .y = rWorldPos.y, .z = rWorldPos.z};
        const auto camera = smgpc::camera::transform_world_to_camera(pose, world);
        const auto depth = std::abs(camera.z);
        if (depth <= 0.0001F) {
            pResult->x = 0.0F;
            pResult->y = 0.0F;
            pResult->z = camera.z;
            return false;
        }

        const auto fovy = pose.fovy_degrees * PI / 180.0F;
        const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
        const auto focal_x = focal_y / pose.aspect_ratio;
        const auto half_width = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F;
        const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
        pResult->x = ((camera.x / camera.z) * focal_x * half_width) + half_width;
        pResult->y = ((camera.y / camera.z) * focal_y * half_height) + half_height;
        pResult->z = camera.z;
        return camera.z > pose.near_clip && camera.z < pose.far_clip;
    }

    [[nodiscard]] bool unproject_screen_to_world(TVec3f* pResult, const TVec2f& rScreenPos, f32 distance) {
        if (pResult == nullptr) {
            return false;
        }

        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr || !runtime->scene_camera_pose().has_value()) {
            pResult->set(0.0F, 0.0F, 0.0F);
            return false;
        }

        constexpr auto PI = 3.14159265358979323846F;
        const auto& pose = *runtime->scene_camera_pose();
        const auto basis = camera_basis(pose);
        const auto world_distance = distance < 0.0F ? pose.near_clip : distance;
        const auto fovy = pose.fovy_degrees * PI / 180.0F;
        const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
        const auto focal_x = focal_y / pose.aspect_ratio;
        const auto half_width = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F;
        const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
        const auto view_x = ((rScreenPos.x - half_width) / (focal_x * half_width)) * world_distance;
        const auto view_y = ((rScreenPos.y - half_height) / (focal_y * half_height)) * world_distance;

        const auto world = add(add(add(pose.eye, scale(basis.right, view_x)), scale(basis.up, view_y)), scale(basis.forward, world_distance));
        pResult->set(world.x, world.y, world.z);
        return world_distance >= pose.near_clip && world_distance <= pose.far_clip;
    }
}  // namespace

namespace MR {
    const TVec3f getCamPos() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->scene_camera_pose().has_value()) {
            return tv_vec3(runtime->scene_camera_pose()->eye);
        }

        return TVec3f{};
    }

    TVec3f getCamXdir() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->scene_camera_pose().has_value()) {
            return tv_vec3(camera_basis(*runtime->scene_camera_pose()).right);
        }

        return TVec3f{1.0F, 0.0F, 0.0F};
    }

    TVec3f getCamYdir() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->scene_camera_pose().has_value()) {
            return tv_vec3(camera_basis(*runtime->scene_camera_pose()).up);
        }

        return TVec3f{0.0F, 1.0F, 0.0F};
    }

    TVec3f getCamZdir() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->scene_camera_pose().has_value()) {
            return tv_vec3(camera_basis(*runtime->scene_camera_pose()).forward);
        }

        return TVec3f{0.0F, 0.0F, 1.0F};
    }

    f32 getAspect() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->scene_camera_pose().has_value()) {
            return runtime->scene_camera_pose()->aspect_ratio;
        }

        return static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) /
               static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight);
    }

    f32 getNearZ() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->scene_camera_pose().has_value()) {
            return runtime->scene_camera_pose()->near_clip;
        }

        return 1.0F;
    }

    f32 getFarZ() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->scene_camera_pose().has_value()) {
            return runtime->scene_camera_pose()->far_clip;
        }

        return 800000.0F;
    }

    f32 getFovy() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->scene_camera_pose().has_value()) {
            return runtime->scene_camera_pose()->fovy_degrees;
        }

        return 45.0F;
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

    bool calcWorldPositionFromScreen(TVec3f* pResult, const TVec2f& rScreenPos, f32 distance) {
        return unproject_screen_to_world(pResult, rScreenPos, distance);
    }

    bool calcWorldRayDirectionFromScreen(TVec3f* pResult, const TVec2f& rScreenPos) {
        const auto ret = unproject_screen_to_world(pResult, rScreenPos, -1.0F);
        if (pResult != nullptr) {
            const auto cam_pos = getCamPos();
            pResult->x -= cam_pos.x;
            pResult->y -= cam_pos.y;
            pResult->z -= cam_pos.z;
        }
        return ret;
    }
}  // namespace MR
