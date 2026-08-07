#include "Game/Util/CameraUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "camera/CameraPose.hpp"
#include "core/RenderTypes.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cmath>
#include <stdexcept>

namespace {
    constexpr auto PI = 3.14159265358979323846F;

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

        const auto fovy = pose->fovy_degrees * PI / 180.0F;
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
        const auto fovy = pose->fovy_degrees * PI / 180.0F;
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

namespace MR {
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

        const auto fovy = pose->fovy_degrees * PI / 180.0F;
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
