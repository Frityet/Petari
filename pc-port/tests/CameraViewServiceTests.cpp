#include "CameraTargetTestSupport.hpp"
#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "camera/CameraAnimation.hpp"
#include "camera/OriginalAnimationCamera.hpp"
#include "camera/OriginalGameCamera.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageCollisionService.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > 0.001F) {
            throw std::runtime_error(std::string(message) + ": actual=" + std::to_string(actual) +
                                     "; expected=" + std::to_string(expected));
        }
    }

    void require_view(const smgpc::camera::CameraPose& pose, float eye_x, float eye_z,
                      float fovy, std::string_view message) {
        require_near(pose.eye.x, eye_x, message);
        require_near(pose.eye.y, 0.0F, message);
        require_near(pose.eye.z, eye_z, message);
        require_near(pose.fovy_degrees, fovy, message);
        require_near(pose.watch.x, pose.eye.x, message);
        require_near(pose.watch.y, pose.eye.y, message);
        require(pose.watch.z < pose.eye.z, "the interpolated view must retain its negative-Z viewing direction");
        require_near(pose.up.x, 0.0F, message);
        require_near(pose.up.y, 1.0F, message);
        require_near(pose.up.z, 0.0F, message);
    }

    void write_be32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
        for (auto index = 0U; index < 4U; ++index) {
            bytes[offset + index] = static_cast<std::uint8_t>(value >> ((3U - index) * 8U));
        }
    }

    smgpc::camera::CameraAnimation make_constant_animation() {
        constexpr auto header_size = std::size_t{0x20U};
        constexpr auto table_size = std::size_t{8U * 8U};
        constexpr auto values_offset = header_size + table_size;
        constexpr auto values = std::array{100.0F, 0.0F, 1200.0F, 100.0F, 0.0F, 0.0F, 0.0F, 80.0F};
        auto bytes = std::vector<std::uint8_t>(values_offset + 4U + values.size() * sizeof(float), 0U);
        std::copy_n("ANDO", 4U, bytes.begin());
        std::copy_n("CANM", 4U, bytes.begin() + 4U);
        write_be32(bytes, 0x08U, 1U);
        write_be32(bytes, 0x10U, 1U);
        write_be32(bytes, 0x18U, 32U);
        write_be32(bytes, 0x1cU, table_size);
        write_be32(bytes, values_offset, values.size() * sizeof(float));
        for (auto index = std::size_t{}; index < values.size(); ++index) {
            write_be32(bytes, header_size + index * 8U, 1U);
            write_be32(bytes, header_size + index * 8U + 4U, index);
            write_be32(bytes, values_offset + 4U + index * sizeof(float), std::bit_cast<std::uint32_t>(values[index]));
        }
        return smgpc::camera::CameraAnimation::from_bytes(bytes);
    }

    struct CameraScene {
        CameraScene() : scheduler_binding(scheduler), dvd("/"), demo(dvd, {}),
                        scene_binding(holder), target([this] { return target_state; }) {
            require(holder.create(SceneObj_AreaObjContainer) != nullptr &&
                        holder.create(SceneObj_PlanetGravityManager) != nullptr,
                    "camera view integration requires real area and gravity registries");
            collision.build();
            collision.activate();
            before.eye = {0.0F, 0.0F, 600.0F};
            before.watch = {};
            before.fovy_degrees = 40.0F;
            before.aspect_ratio = 2.0F;
            before.near_clip = 15.0F;
            before.far_clip = 9000.0F;
            before.projection_offset_x = 0.125F;
            before.projection_offset_y = -0.25F;
            camera.set_game_camera_pose(before);
            camera.declare_event_camera_animation(0, "view-transition", make_constant_animation());
        }

        void start(std::int32_t interpolation_frames) {
            camera.start_event_camera(0, "view-transition",
                                      smgpc::camera::EventCameraTarget::target_object(target), interpolation_frames);
        }

        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding scheduler_binding;
        smgpc::runtime::DvdFileSystemService dvd;
        smgpc::compat::DemoSceneRuntime demo;
        SceneObjHolder holder;
        smgpc::scene::SceneObjHolderBinding scene_binding;
        smgpc::scene::StageCollisionService collision;
        smgpc::camera::StageCameraTargetState target_state{
            .ground_position = smgpc::camera::CameraParamVec3{},
            .gravity = smgpc::camera::CameraParamVec3{0.0F, -1.0F, 0.0F}};
        smgpc::tests::CameraTargetFixture target;
        smgpc::camera::CameraPose before;
        smgpc::runtime::CameraSystemService camera;
    };

    void test_manual_view_seeds_event_transition_once_per_phase() {
        auto scene = CameraScene{};
        scene.start(4);
        require_view(*scene.camera.effective_camera_pose(), 0.0F, 600.0F, 40.0F,
                     "requesting an event must retain the previous published manual view");
        require(scene.target.movement_count == 0U, "event request must not advance its target before the camera phase");

        scene.camera.begin_frame(1U);
        require_view(*scene.camera.active_event_camera_pose(), 100.0F, 1200.0F, 80.0F,
                     "the actual original CANM controller must calculate the event's complete raw pose");
        require_view(*scene.camera.effective_camera_pose(), 0.0F, 600.0F, 40.0F,
                     "the original first switching rate is zero and must preserve the previous scene view");
        require(scene.target.movement_count == 1U, "the target must advance once before the first interpolated camera view");
        scene.camera.begin_frame(1U);
        require_view(*scene.camera.effective_camera_pose(), 0.0F, 600.0F, 40.0F,
                     "repeating the camera phase must not advance switching or publish the next rate");
        require(scene.target.movement_count == 1U, "repeating the camera phase must not move its target twice");

        scene.camera.begin_frame(2U);
        require_view(*scene.camera.effective_camera_pose(), 6.25F, 637.5F, 42.5F,
                     "the next actual switching rate must be one sixteenth from the retained scene view");
        scene.camera.pause_on_camera_director();
        scene.camera.begin_frame(3U);
        require_view(*scene.camera.effective_camera_pose(), 6.25F, 637.5F, 42.5F,
                     "pausing the director must freeze the rendered switching state");
        require(scene.target.movement_count == 2U, "a paused phase must not move the event target");
        scene.camera.pause_off_camera_director();
        scene.camera.begin_frame(3U);
        require_view(*scene.camera.effective_camera_pose(), 29.6875F, 778.125F, 51.875F,
                     "resuming the skipped phase must continue the original recursive switching state");
        require(scene.target.movement_count == 3U, "resuming a skipped phase must advance the target exactly once");
        require_view(*scene.camera.game_camera_pose(), 0.0F, 600.0F, 40.0F,
                     "event interpolation must never replace the underlying manual game pose");
    }

    void test_event_finish_cut_returns_to_manual_view() {
        auto scene = CameraScene{};
        scene.start(0);
        scene.camera.begin_frame(1U);
        require_view(*scene.camera.effective_camera_pose(), 100.0F, 1200.0F, 80.0F,
                     "a zero-frame event start must cut to the actual CANM view");
        scene.camera.end_event_camera(0, "view-transition", false, 0);
        require(!scene.camera.active_event_camera_key(), "ending the event must retire its requested and calculated owner");
        scene.camera.begin_frame(2U);
        const auto after = scene.camera.effective_camera_pose();
        require(after.has_value(), "ending an event must retain the underlying manual view");
        require_view(*after, 0.0F, 600.0F, 40.0F,
                     "a zero-frame event finish must return to the manual view rather than leave an event cache");
        require(after->aspect_ratio == scene.before.aspect_ratio && after->near_clip == scene.before.near_clip &&
                    after->far_clip == scene.before.far_clip &&
                    after->projection_offset_x == scene.before.projection_offset_x &&
                    after->projection_offset_y == scene.before.projection_offset_y,
                "the returning manual source must restore its own projection metadata");
        require(scene.target.movement_count == 1U, "returning to a manual source must not keep advancing the ended event target");
        scene.camera.clear_game_camera_pose();
        require(!scene.camera.effective_camera_pose(),
                "retiring the remaining scene camera must retire its cached rendered view");
        scene.start(0);
        scene.camera.begin_frame(3U);
        require(scene.camera.effective_camera_pose().has_value(),
                "an event can own the view without an underlying manual camera");
        scene.camera.end_event_camera(0, "view-transition", false, 0);
        require(!scene.camera.effective_camera_pose(),
                "a completed event must not publish an orphaned cached view when no scene source remains");
    }

    void test_event_finish_interpolates_back_to_manual_view() {
        auto scene = CameraScene{};
        scene.start(0);
        scene.camera.begin_frame(1U);
        scene.camera.end_event_camera(0, "view-transition", false, 4);
        scene.camera.begin_frame(2U);
        require_view(*scene.camera.effective_camera_pose(), 100.0F, 1200.0F, 80.0F,
                     "positive finish interpolation must begin at the last event view with its zero first rate");
        scene.camera.begin_frame(3U);
        const auto after = *scene.camera.effective_camera_pose();
        require(after.eye.x > 0.0F && after.eye.x < 100.0F && after.eye.z > 600.0F && after.eye.z < 1200.0F &&
                    after.fovy_degrees > 40.0F && after.fovy_degrees < 80.0F,
                "subsequent finish phases must move and change FOV toward the existing manual game view");
        scene.camera.begin_frame(3U);
        require_view(*scene.camera.effective_camera_pose(), after.eye.x, after.eye.z, after.fovy_degrees,
                     "a repeated finish phase must preserve its accumulated rendered result");
        require(scene.target.movement_count == 1U, "finish interpolation must not retain an ended event's target movement");
    }

    TPos3f make_matrix(float eye_x) {
        auto matrix = TPos3f{};
        matrix.identity();
        matrix.mMtx[0][0] = 0.0F;
        matrix.mMtx[0][1] = -1.0F;
        matrix.mMtx[1][0] = 1.0F;
        matrix.mMtx[1][1] = 0.0F;
        matrix.setTrans(TVec3f{eye_x, 22.0F, 33.0F});
        return matrix;
    }

    void require_matrix(const TPos3f& actual, const TPos3f& expected, std::string_view message) {
        for (auto row = 0U; row < 3U; ++row) {
            for (auto column = 0U; column < 4U; ++column) {
                require_near(actual.mMtx[row][column], expected.mMtx[row][column], message);
            }
        }
    }

    void test_original_controller_reset_retains_manager_matrix_seed() {
        auto scene = CameraScene{};
        const auto matrix = make_matrix(11.0F);
        auto pose = CameraPoseParam{};
        pose.mPos.set(10.0F, 0.0F, 600.0F);
        pose.mWatchPos.set(10.0F, 0.0F, 0.0F);
        pose.mFovy = 63.0F;
        auto param = smgpc::camera::CameraParamChunk{};
        param.camera_type = "CAM_TYPE_XZ_PARA";
        param.general.num1 = 1;
        param.general.dist = 600.0F;
        param.general.angle_a = 0.0F;
        param.general.angle_b = 0.0F;
        param.extra.w_offset = {};
        param.extra.v_pan_use = 0;

        auto game = smgpc::camera::OriginalGameCamera({}, param, scene.target, 45.0F, &pose, false, &matrix);
        require_matrix(game.manager().mMatrix, matrix,
                       "the original static reset must retain the manager's distinct rendered matrix seed");
        require(game.target_object() == nullptr, "resetting a controller must not invent a returned used target");
        (void)game.calc(scene.target);
        require(game.target_object() == &scene.target,
                "the actual Parallel calc return must become the static camera's used target");

        auto animation = smgpc::camera::OriginalAnimationCamera(make_constant_animation(), scene.target, 1.0F, &pose, &matrix);
        require_matrix(animation.manager().mMatrix, matrix,
                       "the original animation reset must retain the manager's rendered matrix seed");
        require_near(animation.pose_param().mFovy, 63.0F,
                     "animation reset must copy the raw manager FOV independently of its matrix seed");
        require(animation.current_frame() == 0.0F && animation.target_object() == nullptr,
                "construction must complete original reset without calculating or fabricating a used target");
    }

    void test_pending_event_matrix_seed_ownership_and_handoff() {
        auto scene = CameraScene{};
        auto event = smgpc::camera::EventCameraRuntime{};
        for (const auto name : {"A", "B", "C"}) {
            event.declare_animation(0, name, make_constant_animation());
        }
        auto matrix = make_matrix(11.0F);
        const auto original_matrix = matrix;
        event.start(0, "A", smgpc::camera::EventCameraTarget::target_object(scene.target), 0,
                    1.0F, nullptr, &matrix);
        matrix.identity();
        const auto unrelated_game_matrix = make_matrix(-999.0F);
        event.start(0, "B", smgpc::camera::EventCameraTarget::retain(), 0,
                    1.0F, nullptr, &unrelated_game_matrix);
        event.begin_frame(1U, false);
        require(event.view_manager() != nullptr, "the requested event must construct its original manager in the camera phase");
        require_matrix(event.view_manager()->mMatrix, original_matrix,
                       "replacing an uncalculated request must preserve an owned copy of the first game matrix seed");

        const auto rendered_matrix = make_matrix(55.0F);
        event.view_manager()->mMatrix.set(rendered_matrix);
        event.start(0, "C", smgpc::camera::EventCameraTarget::retain(), 0,
                    1.0F, nullptr, &unrelated_game_matrix);
        event.start(0, "A", smgpc::camera::EventCameraTarget::retain(), 0,
                    1.0F, nullptr, &unrelated_game_matrix);
        event.begin_frame(2U, false);
        require_matrix(event.view_manager()->mMatrix, rendered_matrix,
                       "event changes must inherit the calculated manager's last rendered matrix instead of a game seed");
    }

    void test_animation_used_target_is_distinct_from_moved_input() {
        auto scene = CameraScene{};
        auto event = smgpc::camera::EventCameraRuntime{};
        event.declare_animation(0, "moving-input", make_constant_animation());
        event.start(0, "moving-input", smgpc::camera::EventCameraTarget::target_object(scene.target), 0);
        event.begin_frame(1U, false);
        require(scene.target.movement_count == 1U && event.view_target() == nullptr,
                "CameraAnim must move its selected input while returning no view-correction target");
        scene.target_state.position.x = 25.0F;
        scene.target_state.last_move.x = 25.0F;
        event.begin_frame(2U, false);
        require(scene.target.movement_count == 2U && event.view_target() == nullptr,
                "a moving animation input must not become the director's used target on a later phase");
        require_view(*event.active_pose(), 125.0F, 1200.0F, 80.0F,
                     "actual CANM calculation must still use the live selected target transform");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"manual view seeds original event interpolation", test_manual_view_seeds_event_transition_once_per_phase},
        TestCase{"event finish cuts back to manual view", test_event_finish_cut_returns_to_manual_view},
        TestCase{"event finish interpolates back to manual view", test_event_finish_interpolates_back_to_manual_view},
        TestCase{"controller reset retains manager matrix seed", test_original_controller_reset_retains_manager_matrix_seed},
        TestCase{"pending event matrix seed ownership", test_pending_event_matrix_seed_ownership_and_handoff},
        TestCase{"animation used target versus moved input", test_animation_used_target_is_distinct_from_moved_input},
    };
    auto failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[pass] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
    std::cout << tests.size() - static_cast<std::size_t>(failures) << '/' << tests.size() << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
