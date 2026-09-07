#include "CameraTargetTestSupport.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraViewInterpolator.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "camera/OriginalCameraView.hpp"
#include "compat/CameraViewRuntime.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageCollisionService.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
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

    void require_near(float actual, float expected, float tolerance, std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) + ": actual=" + std::to_string(actual) +
                                     "; expected=" + std::to_string(expected));
        }
    }

    void require_vector(const TVec3f& actual, const TVec3f& expected, std::string_view message) {
        require_near(actual.x, expected.x, 0.0001F, message);
        require_near(actual.y, expected.y, 0.0001F, message);
        require_near(actual.z, expected.z, 0.0001F, message);
    }

    CameraPoseParam make_pose(const TVec3f& eye, const TVec3f& watch, float fovy) {
        auto pose = CameraPoseParam{};
        pose.mPos.set(eye);
        pose.mWatchPos.set(watch);
        pose.mUpVec.set(0.0F, 1.0F, 0.0F);
        pose.mWatchUpVec.set(0.0F, 1.0F, 0.0F);
        pose.mFovy = fovy;
        return pose;
    }

    constexpr auto isolated_switching = smgpc::camera::OriginalCameraViewFlags{
        .interpolation_off = true, .collision_off = true};

    void test_cut_output_and_interpolation_off_gate() {
        auto owner = smgpc::camera::OriginalCameraView{};
        auto& original = owner.original();
        original.mIsRepulsionOff = true;
        owner.set_interpolation(0U);
        const auto pose = make_pose({0.0F, 0.0F, 600.0F}, {}, 40.0F);
        auto projection = smgpc::camera::CameraPose{};
        projection.aspect_ratio = 2.0F;
        projection.near_clip = 15.0F;
        projection.far_clip = 9000.0F;
        projection.projection_offset_x = 0.125F;
        projection.projection_offset_y = -0.25F;
        const auto result = owner.update(pose, nullptr, projection, isolated_switching);
        require_near(result.eye.z, 600.0F, 0.0001F, "a zero-frame cut must publish the requested eye immediately");
        require_near(result.fovy_degrees, 40.0F, 0.0001F, "a forced camera cut must also publish the requested FOV");
        require_near(owner.output().view.mMtx[2][3], -600.0F, 0.0001F,
                     "the original update must publish an actual view matrix with inverse camera translation");
        require_near(owner.output().inverse_view.mMtx[2][3], 600.0F, 0.0001F,
                     "the output context must retain the matching inverse view matrix");
        require(result.aspect_ratio == 2.0F && result.near_clip == 15.0F && result.far_clip == 9000.0F &&
                    result.projection_offset_x == 0.125F && result.projection_offset_y == -0.25F,
                "view interpolation must retain the caller's projection metadata");
        require(!original.mIsForceCameraChange && !original.mIsCollisionOff &&
                    smgpc::compat::bound_camera_view_output() == nullptr,
                "the original frame must consume one-shot cut flags and release its output binding");

        owner.set_interpolation(7U);
        original.mInterpolateTimer = 2U;
        original.mIsInterpolationOff = true;
        owner.set_interpolation(0U);
        require(original.mInterpolateTime == 7U && original.mInterpolateTimer == 2U &&
                    !original.mIsForceCameraChange,
                "setInterpolation must preserve the active timer when the original interpolation-off gate is set");
        (void)owner.update(pose, nullptr, projection, isolated_switching);
        require(!original.mIsInterpolationOff,
                "updateCameraMtx must clear the original one-frame interpolation-off gate");
        owner.set_interpolation(0U);
        require(!original.isInterpolating() && original.isInterpolatingNearlyEnd() && original.mIsForceCameraChange,
                "an ungated zero-frame request must restore the original forced-cut state");
    }

    void test_recursive_switching_timer_and_fov() {
        auto owner = smgpc::camera::OriginalCameraView{};
        auto& original = owner.original();
        original.mIsRepulsionOff = true;
        const auto before = make_pose({0.0F, 0.0F, 600.0F}, {}, 40.0F);
        const auto after = make_pose({100.0F, 0.0F, 1200.0F}, {100.0F, 0.0F, 0.0F}, 80.0F);
        (void)owner.update(before, nullptr, {}, isolated_switching);
        owner.set_interpolation(4U);
        struct Expected {
            float eye_x;
            float eye_z;
            float fovy;
            std::uint32_t timer;
            bool interpolating;
        };
        // The original feeds each result back into the next calculation.
        // Its successive squared rates are 0, 1/16, 1/4, 9/16, and 1.
        constexpr auto expected = std::array{
            Expected{0.0F, 600.0F, 40.0F, 1U, true},
            Expected{6.25F, 637.5F, 42.5F, 2U, true},
            Expected{29.6875F, 778.125F, 51.875F, 3U, true},
            Expected{69.23828125F, 1015.4296875F, 67.6953125F, 4U, false},
            Expected{100.0F, 1200.0F, 80.0F, 4U, false},
        };
        for (const auto& frame : expected) {
            const auto result = owner.update(after, nullptr, {}, isolated_switching);
            require_near(result.eye.x, frame.eye_x, 0.0003F, "camera switching must retain the original recursive watch interpolation");
            require_near(result.eye.z, frame.eye_z, 0.0003F, "camera switching must retain the original recursive distance interpolation");
            require_near(result.fovy_degrees, frame.fovy, 0.0001F, "camera switching must use the same squared rate for FOV");
            require(original.mInterpolateTimer == frame.timer && original.isInterpolating() == frame.interpolating,
                    "the original timer must saturate before its next rate-one calculation reaches the exact endpoint");
        }
    }

    void test_separate_anti_oscillation_position_and_fov_rates() {
        auto owner = smgpc::camera::OriginalCameraView{};
        owner.original().mIsRepulsionOff = true;
        const auto before = make_pose({0.0F, 0.0F, 600.0F}, {}, 40.0F);
        const auto after = make_pose({100.0F, 0.0F, 1000.0F}, {100.0F, 0.0F, 0.0F}, 80.0F);
        constexpr auto flags = smgpc::camera::OriginalCameraViewFlags{.collision_off = true};
        (void)owner.update(before, nullptr, {}, flags);
        const auto first = owner.update(after, nullptr, {}, flags);
        require_near(first.eye.x, 30.0F, 0.0001F, "anti-oscillation must advance position by the original 30 percent rate");
        require_near(first.eye.z, 720.0F, 0.0001F, "anti-oscillation must retain 70 percent of the previous camera translation");
        require_near(first.fovy_degrees, 44.0F, 0.0001F, "anti-oscillation must advance FOV by its separate 10 percent rate");
        const auto second = owner.update(after, nullptr, {}, flags);
        require_near(second.eye.x, 51.0F, 0.0001F, "anti-oscillation must continue from the preceding output position");
        require_near(second.eye.z, 804.0F, 0.0001F, "anti-oscillation must preserve position state between frames");
        require_near(second.fovy_degrees, 47.6F, 0.0001F, "FOV damping must preserve its independently accumulated state");
        owner.set_interpolation(0U);
        const auto cut = owner.update(after, nullptr, {}, flags);
        require_near(cut.eye.x, 100.0F, 0.0001F, "a forced cut must bypass position damping");
        require_near(cut.fovy_degrees, 80.0F, 0.0001F, "a forced cut must bypass FOV damping");
    }

    void test_target_identity_and_last_move_correction() {
        auto owner = smgpc::camera::OriginalCameraView{};
        auto& original = owner.original();
        const auto state = smgpc::camera::StageCameraTargetState{.last_move = {10.0F, 20.0F, 30.0F}};
        auto target = smgpc::tests::CameraTargetFixture([&] { return state; });
        auto replacement = smgpc::tests::CameraTargetFixture([&] { return state; });
        original.mTargetPosition.set(1.0F, 2.0F, 3.0F);
        original.mTargetMtx.setTrans(TVec3f{100.0F, 200.0F, 300.0F});
        original.updateCalcState(&target);
        require(original.mCalcState == CameraViewInterpolator::CalcState_Invalid,
                "a changed target identity must invalidate last-move correction before using the new target");
        original.setInterpolation(2U);
        original.updateCalcState(&target);
        require(original.mCalcState == CameraViewInterpolator::CalcState_Invalid,
                "a new target must remain invalid while its camera-switching timer is active");
        original.setInterpolation(0U);
        original.updateCalcState(&target);
        require(original.mCalcState == CameraViewInterpolator::CalcState_Ready,
                "the original target state may become ready once camera switching has finished");
        original.updateCalcState(&target);
        require_vector(original.mTargetPosition, {11.0F, 22.0F, 33.0F},
                       "a ready same-target frame must compensate the watched position by the target's actual last move");
        auto translated = TVec3f{};
        original.mTargetMtx.getTrans(translated);
        require_vector(translated, {110.0F, 220.0F, 330.0F},
                       "a ready same-target frame must also compensate the interpolated camera position");
        original.mIsCorrectErpPositionOn = false;
        original.updateCalcState(&target);
        original.mTargetMtx.getTrans(translated);
        require_vector(original.mTargetPosition, {21.0F, 42.0F, 63.0F},
                       "disabling camera-position correction must still compensate the watched target");
        require_vector(translated, {110.0F, 220.0F, 330.0F},
                       "the correction-off flag must preserve the preceding camera position for one frame");
        require(original.mIsCorrectErpPositionOn, "the original target update must restore the one-frame correction flag");
        original.updateCalcState(&replacement);
        require(original.mCalcState == CameraViewInterpolator::CalcState_Invalid,
                "an equal-valued replacement target must still invalidate correction by object identity");
        require_vector(original.mTargetPosition, {21.0F, 42.0F, 63.0F},
                       "changing target identity must not apply its last move to the old target position");
    }

    void test_nearly_end_distance_and_rotation_thresholds() {
        auto owner = smgpc::camera::OriginalCameraView{};
        auto& original = owner.original();
        original.setInterpolation(20U);
        auto candidate = TPos3f{};
        candidate.identity();
        candidate.setTrans(TVec3f{1.0F, 0.0F, 0.0F});
        original.checkNearlyEnd(candidate);
        require(original.isInterpolatingNearlyEnd(), "the original nearly-end distance threshold includes exactly one unit");
        candidate.setTrans(TVec3f{1.001F, 0.0F, 0.0F});
        original.checkNearlyEnd(candidate);
        require(!original.isInterpolatingNearlyEnd(), "a separation greater than one unit must fail the nearly-end test");
        candidate.identity();
        candidate.mMtx[0][0] = 0.86602540378F;
        candidate.mMtx[0][1] = -0.5F;
        candidate.mMtx[1][0] = 0.5F;
        candidate.mMtx[1][1] = 0.86602540378F;
        original.checkNearlyEnd(candidate);
        require(original.isInterpolatingNearlyEnd(), "a 30-degree difference is below the original one-radian rotation threshold");
        candidate.mMtx[0][0] = 0.5F;
        candidate.mMtx[0][1] = -0.86602540378F;
        candidate.mMtx[1][0] = 0.86602540378F;
        candidate.mMtx[1][1] = 0.5F;
        original.checkNearlyEnd(candidate);
        require(!original.isInterpolatingNearlyEnd(), "a 60-degree difference exceeds the original one-radian threshold");
    }

    void test_camera_service_keeps_raw_pose_and_damps_rendered_view() {
        auto scheduler = smgpc::runtime::SceneScheduler{};
        const auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding(scheduler);
        auto dvd = smgpc::runtime::DvdFileSystemService("/");
        auto demo = smgpc::compat::DemoSceneRuntime(dvd, {});
        auto holder = SceneObjHolder{};
        auto scene_binding = smgpc::scene::SceneObjHolderBinding(holder);
        require(holder.create(SceneObj_AreaObjContainer) != nullptr,
                "the service view test must provide the original repulsive-area query registry");
        auto collision = smgpc::scene::StageCollisionService{};
        collision.build();
        collision.activate();

        auto resolved = smgpc::camera::ResolvedStageStartCamera{};
        resolved.camera_key = "s:0000";
        resolved.camera_param.id = resolved.camera_key;
        resolved.camera_param.camera_type = "CAM_TYPE_XZ_PARA";
        resolved.camera_param.general.num1 = 1;
        resolved.camera_param.general.dist = 600.0F;
        resolved.camera_param.general.angle_a = 0.0F;
        resolved.camera_param.general.angle_b = 0.0F;
        resolved.camera_param.extra.w_offset = {};
        resolved.camera_param.extra.v_pan_use = 0;
        resolved.camera_param.extra.cam_int = 0;
        // Original CameraParallel subtracts 90 degrees from its azimuth.
        // At angleA0 and distance600, targetX600 places the eye at X0.
        resolved.target.position.x = 600.0F;
        resolved.target.ground_position = smgpc::camera::CameraParamVec3{600.0F, 0.0F, 0.0F};
        resolved.target.gravity = smgpc::camera::CameraParamVec3{0.0F, -1.0F, 0.0F};
        const auto initial = smgpc::camera::calculate_stage_camera_pose(
            resolved.start_info.zone_transform, resolved.camera_param, resolved.target);
        require(initial.has_value(), "the raw game-view fixture must calculate through original CameraParallel");
        resolved.calculation = *initial;
        auto camera = smgpc::runtime::CameraSystemService{};
        const auto generation = camera.set_authored_game_camera(resolved);
        auto target = resolved.target;
        camera.set_game_camera_target(generation, target);
        camera.begin_frame(1U);
        require_near(camera.game_camera_pose()->eye.x, 0.0F, 0.0001F,
                     "the service fixture must begin with its original raw camera at X0");
        require_near(camera.effective_camera_pose()->eye.x, 0.0F, 0.0001F,
                     "the initial cut must seed the persistent rendered view at X0");

        // A target position change with zero lastMove isolates view damping
        // from the separate target-motion correction state machine.
        target.position.x = 700.0F;
        target.ground_position->x = 700.0F;
        camera.set_game_camera_target(generation, target);
        camera.begin_frame(2U);
        require_near(camera.game_camera_pose()->eye.x, 100.0F, 0.0001F,
                     "game_camera_pose must retain the original controller's complete translation");
        require_near(camera.effective_camera_pose()->eye.x, 30.0F, 0.0002F,
                     "effective_camera_pose must expose the original interpolator's damped render translation");
        camera.begin_frame(2U);
        require_near(camera.effective_camera_pose()->eye.x, 30.0F, 0.0002F,
                     "repeating a service camera phase must not damp its rendered view a second time");
        camera.begin_frame(3U);
        require_near(camera.game_camera_pose()->eye.x, 100.0F, 0.0001F,
                     "render damping must never feed its output back into the raw game manager pose");
        require_near(camera.effective_camera_pose()->eye.x, 51.0F, 0.0002F,
                     "the service must retain original view damping state across phases");
        camera.clear_stage_start_camera(generation);
    }

    void write_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
        for (auto index = 0U; index < 4U; ++index) {
            bytes[offset + index] = static_cast<std::uint8_t>(value >> ((3U - index) * 8U));
        }
    }

    void write_u16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_float(std::vector<std::uint8_t>& bytes, std::size_t offset, float value) {
        write_u32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    std::vector<std::uint8_t> make_wall_prism() {
        auto bytes = std::vector<std::uint8_t>(0x88U, 0U);
        write_u32(bytes, 0x00U, 0x38U);
        write_u32(bytes, 0x04U, 0x44U);
        write_u32(bytes, 0x08U, 0x64U);
        write_u32(bytes, 0x0cU, 0x84U);
        write_float(bytes, 0x10U, 1000.0F);
        const auto normal = [&](std::size_t offset, const TVec3f& vector) {
            write_float(bytes, offset, vector.x);
            write_float(bytes, offset + 4U, vector.y);
            write_float(bytes, offset + 8U, vector.z);
        };
        normal(0x44U, {0.0F, 1.0F, 0.0F});
        normal(0x50U, {1.0F, 0.0F, 0.0F});
        normal(0x5cU, {0.0F, 0.0F, 1.0F});
        normal(0x68U, {0.70710678118F, 0.0F, 0.70710678118F});
        write_float(bytes, 0x74U, 0.70710678118F);
        write_u16(bytes, 0x7cU, 1U);
        write_u16(bytes, 0x7eU, 2U);
        write_u16(bytes, 0x80U, 3U);
        return bytes;
    }

    std::vector<std::uint8_t> make_camera_attributes(std::string_view code) {
        constexpr auto data_offset = std::size_t{0x1cU};
        constexpr auto strings_offset = data_offset + 4U;
        auto bytes = std::vector<std::uint8_t>(strings_offset + code.size() + 1U, 0U);
        write_u32(bytes, 0U, 1U);
        write_u32(bytes, 4U, 1U);
        write_u32(bytes, 8U, data_offset);
        write_u32(bytes, 12U, 4U);
        write_u32(bytes, 0x10U, smgpc::resource::jmap_hash("Camera_through"));
        write_u32(bytes, 0x14U, 0xffffffffU);
        bytes[0x1bU] = static_cast<std::uint8_t>(smgpc::resource::BcsvFieldType::StringOffset);
        std::memcpy(bytes.data() + strings_offset, code.data(), code.size());
        return bytes;
    }

    void test_camera_binder_wall_filter_and_through_distance() {
        const auto kcl = make_wall_prism();
        constexpr auto wall_matrix = std::array<float, 12U>{
            0.0F, 1.0F, 0.0F, 0.0F,
            -10000.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 10000.0F, 0.0F};
        using CollisionService = smgpc::scene::StageCollisionService;
        alignas(CollisionService) std::array<std::byte, sizeof(CollisionService)> storage{};
        auto preceding_generation = std::uint64_t{};
        auto preceding_revision = std::uint64_t{};
        for (const auto through : {false, true}) {
            // Deliberately reuse the address with a fresh service lifetime:
            // equal source indices/revisions must not reuse the old .pa rows.
            auto service_owner = std::unique_ptr<CollisionService, void (*)(CollisionService*)>(
                std::construct_at(reinterpret_cast<CollisionService*>(storage.data())),
                [](CollisionService* service) { std::destroy_at(service); });
            auto& collision = *service_owner;
            const auto attributes = make_camera_attributes(through ? "Through" : "NoThrough");
            require(collision.register_kcl(kcl, wall_matrix, "camera-wall.kcl", {}, attributes).accepted,
                    "the camera wall test must register real KCL geometry with its own Camera_through attribute");
            if (through) {
                require(collision.generation() != preceding_generation && collision.revision() == preceding_revision,
                        "same-address collision reconstruction must get a fresh lifetime identity even at an equal resource revision");
            }
            preceding_generation = collision.generation();
            preceding_revision = collision.revision();
            collision.build();
            collision.activate();
            auto owner = smgpc::camera::OriginalCameraView{};
            auto& original = owner.original();
            auto output = smgpc::compat::CameraViewOutput{};
            output.fovy = 90.0F;
            const auto binding = smgpc::compat::ScopedCameraViewOutput(output);
            const auto from = TVec3f{500.0F, -1000.0F, 1000.0F};
            const auto to = TVec3f{50.0F, -1000.0F, 1000.0F};
            auto resolved = TVec3f{};
            const auto collided = original.calcBinder(&resolved, to, from);
            const auto expected_radius = std::sqrt(20000.0F + (1600.0F / 9.0F) * (1600.0F / 9.0F));
            require_near(original.mBinder->mRadius, expected_radius, 0.02F,
                         "the original camera Binder radius must enclose the 90-degree 16:9 near plane at distance100");
            require(original.mBinder->_C == nullptr && original.mBinder->_10 == &original.mPosition &&
                        original.mBinder->_14 == &original.mGravity,
                    "the camera Binder must retain its original null matrix and live position/gravity owners");
            require(collided != through && (original.mBinder->mPlaneNum != 0) != through,
                    std::string("the constructor's CameraThrough filter must exclude only the authored Through wall: code=") +
                        (through ? "Through" : "NoThrough") + "; collided=" + std::to_string(collided) +
                        "; planes=" + std::to_string(original.mBinder->mPlaneNum));
            if (through) {
                require_vector(resolved, {-450.0F, 0.0F, 0.0F},
                               "a CameraThrough wall must leave the camera's entire requested movement available");
            } else {
                require(resolved.x > -450.0F && resolved.x < 0.0F,
                        "an opaque wall must stop the camera sphere before the requested endpoint");
                original.mTargetMtx.identity();
                original.mTargetMtx.setTrans(to);
                original.mTargetPosition.set(0.0F, -1000.0F, 1000.0F);
                auto preceding = TPos3f{};
                preceding.identity();
                preceding.setTrans(from);
                require(original.calcCollision(preceding),
                        "the original collision stage must consume the accepted camera Binder contact");
                auto adjusted = TVec3f{};
                original.mTargetMtx.getTrans(adjusted);
                require_near(adjusted.x, from.x + resolved.x * 0.25F, 0.0002F,
                             "camera collision must apply the original quarter-step response to the Binder result");
                require(!original.mIsCollided, "the original accepted-contact branch must clear its collision-state flag");
            }
            resolved.set(99.0F, 98.0F, 97.0F);
            require(!original.calcBinder(&resolved, from + TVec3f{3400.0F, 0.0F, 0.0F}, from) &&
                        original.mBinder->mPlaneNum == 0,
                    "the original 3400-unit through-distance boundary must clear contacts and bypass binding");
            require_vector(resolved, {99.0F, 98.0F, 97.0F},
                           "the through-distance early return must leave the caller's output vector untouched");
        }
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"cut output and interpolation-off gate", test_cut_output_and_interpolation_off_gate},
        TestCase{"recursive camera switching", test_recursive_switching_timer_and_fov},
        TestCase{"separate anti-oscillation rates", test_separate_anti_oscillation_position_and_fov_rates},
        TestCase{"target identity and last-move correction", test_target_identity_and_last_move_correction},
        TestCase{"nearly-end thresholds", test_nearly_end_distance_and_rotation_thresholds},
        TestCase{"raw controller and damped service view", test_camera_service_keeps_raw_pose_and_damps_rendered_view},
        TestCase{"camera Binder wall and Through filter", test_camera_binder_wall_filter_and_through_distance},
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
