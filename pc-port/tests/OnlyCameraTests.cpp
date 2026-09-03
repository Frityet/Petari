#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraViewInterpolator.hpp"
#include "Game/Camera/OnlyCamera.hpp"
#include "camera/OriginalCameraView.hpp"
#include "compat/CameraViewRuntime.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > 0.0002F) {
            throw std::runtime_error(std::string(message) + ": actual=" + std::to_string(actual) +
                                     "; expected=" + std::to_string(expected));
        }
    }

    void require_vector(const TVec3f& actual, const TVec3f& expected, std::string_view message) {
        require_near(actual.x, expected.x, message);
        require_near(actual.y, expected.y, message);
        require_near(actual.z, expected.z, message);
    }

    void require_pose(const CameraPoseParam& actual, const CameraPoseParam& expected, std::string_view message) {
        require_vector(actual.mPos, expected.mPos, message);
        require_vector(actual.mWatchPos, expected.mWatchPos, message);
        require_vector(actual.mUpVec, expected.mUpVec, message);
        require_vector(actual.mWatchUpVec, expected.mWatchUpVec, message);
        require_vector(actual.mGlobalOffset, expected.mGlobalOffset, message);
        require_vector(actual.mLocalOffset, expected.mLocalOffset, message);
        require_near(actual.mFovy, expected.mFovy, message);
        require_near(actual.mFrontOffset, expected.mFrontOffset, message);
        require_near(actual.mUpperOffset, expected.mUpperOffset, message);
        require_near(actual.mRoll, expected.mRoll, message);
    }

    struct Manager {
        CameraMan original{"OnlyCameraTestManager"};
        std::unique_ptr<CameraPoseParam> pose{original.mPoseParam};
    };

    struct Fixture {
        Manager manager;
        OnlyCamera original{"OnlyCameraTest"};
        std::unique_ptr<CameraPoseParam> pose{original.mPoseParam};

        void set_geometry(const TVec3f& eye, const TVec3f& watch, const TVec3f& up = {0.0F, 1.0F, 0.0F}) {
            manager.pose->mPos.set(eye);
            manager.pose->mWatchPos.set(watch);
            manager.pose->mUpVec.set(up);
        }

        void calc() { original.calcPose(&manager.original); }
    };

    void test_first_frame_watch_distances() {
        struct Case {
            float requested_distance;
            TVec3f expected_offset;
        };
        const auto cases = std::array{
            Case{0.0F, {0.0F, 0.0F, -1.0F}},
            Case{0.5F, {300.0F, 0.0F, 0.0F}},
            Case{299.0F, {300.0F, 0.0F, 0.0F}},
            Case{300.0F, {300.0F, 0.0F, 0.0F}},
            Case{450.0F, {450.0F, 0.0F, 0.0F}},
        };
        const auto eye = TVec3f{10.0F, 20.0F, 30.0F};
        for (const auto& sample : cases) {
            auto fixture = Fixture{};
            fixture.set_geometry(eye, eye + TVec3f(sample.requested_distance, 0.0F, 0.0F));
            fixture.calc();
            require_vector(fixture.pose->mPos, eye, "the first pose must retain the requested eye");
            require_vector(fixture.pose->mWatchPos, eye + sample.expected_offset,
                           "the first pose must distinguish coincident targets from nonzero short targets");
            require_vector(fixture.original.mPos, eye, "the first pose must establish the desired-position reference");
            require(!fixture.original.mStartPose && !fixture.original.mCalcIdeal,
                    "the first calculation must consume its start state without enabling ideal movement");
        }
    }

    void test_later_close_watch_preserves_previous_direction_and_distance() {
        require(std::bit_cast<std::uint32_t>(TVec3f(1.0F, 0.0F, 0.0F).length()) == 0x3F7FFFFFU,
                "the retail PSVECMag unit witness must retain its one-step reciprocal-square-root rounding");
        const auto eye = TVec3f{10.0F, 20.0F, 30.0F};
        // 2^-20 is one ULP at the translated watch X=11, so this above-unit
        // displacement survives the position addition and subsequent subtraction.
        for (const float distance : {0.0F, 0.5F, 1.0F, 0x1.00001p0F, 299.0F, 300.0F, 450.0F}) {
            auto fixture = Fixture{};
            fixture.set_geometry({}, {0.0F, 0.0F, -450.0F});
            fixture.calc();
            fixture.set_geometry(eye, eye + TVec3f(distance, 0.0F, 0.0F));
            fixture.calc();
            const auto expected_offset = distance <= 1.0F ? TVec3f(0.0F, 0.0F, -450.0F)
                                                       : TVec3f(distance < 300.0F ? 300.0F : distance, 0.0F, 0.0F);
            require_vector(fixture.pose->mPos, eye, "inactive ideal movement must pass through the requested eye");
            require_vector(fixture.pose->mWatchPos, eye + expected_offset,
                           "the unit displacement rounds below the retail threshold; the next translated watch value must extend to 300");
        }
    }

    void test_first_and_safe_up_vectors_and_pose_fields() {
        auto fixture = Fixture{};
        fixture.set_geometry({}, {0.0F, 0.0F, -450.0F}, {0.0F, 3.0F, 4.0F});
        fixture.manager.pose->mWatchUpVec.set(2.0F, 3.0F, 4.0F);
        fixture.manager.pose->mGlobalOffset.set(5.0F, 6.0F, 7.0F);
        fixture.manager.pose->mLocalOffset.set(8.0F, 9.0F, 10.0F);
        fixture.manager.pose->mRoll = 0.25F;
        fixture.manager.pose->mFovy = 70.0F;
        fixture.manager.pose->mFrontOffset = 12.0F;
        fixture.manager.pose->mUpperOffset = 13.0F;
        const auto requested = *fixture.manager.pose;
        fixture.calc();
        require_vector(fixture.pose->mUpVec, {0.0F, 0.6F, 0.8F},
                       "a valid skewed first up vector must only be normalized");
        fixture.calc();
        require_vector(fixture.pose->mUpVec, {0.0F, 1.0F, 0.0F},
                       "a later safe pose must remove the up vector's component along the view direction");
        require_vector(fixture.pose->mWatchUpVec, requested.mWatchUpVec, "watch-up must retain the manager's raw value");
        require_vector(fixture.pose->mGlobalOffset, requested.mGlobalOffset, "global offset must be copied from the manager");
        require_vector(fixture.pose->mLocalOffset, requested.mLocalOffset, "local offset must be copied from the manager");
        require_near(fixture.pose->mRoll, 0.25F, "roll must be copied from the manager");
        require_near(fixture.pose->mFovy, 40.0F, "OnlyCamera must retain its own FOV field rather than copy the manager FOV");
        require_near(fixture.pose->mFrontOffset, 0.0F, "OnlyCamera must leave its own front-offset field untouched");
        require_near(fixture.pose->mUpperOffset, 0.0F, "OnlyCamera must leave its own upper-offset field untouched");
        require_pose(*fixture.manager.pose, requested, "pose processing must not modify the manager's raw pose");
    }

    void test_invalid_up_recovery_and_transport() {
        for (const auto bad_up : {TVec3f(0.0F, 0.0F, 0.0F), TVec3f(0.0F, -2.0F, 0.0F)}) {
            auto fixture = Fixture{};
            fixture.set_geometry({}, {0.0F, 450.0F, 0.0F}, bad_up);
            fixture.calc();
            require_vector(fixture.pose->mUpVec, {0.0F, 0.0F, 1.0F},
                           "first invalid up must rotate the canonical up from -Z to the requested +Y direction");
        }
        for (const auto direction : {TVec3f(0.0F, 0.0F, -450.0F), TVec3f(0.0F, 0.0F, 450.0F),
                                     TVec3f(0.0F, 450.0F, 0.0F)}) {
            auto fixture = Fixture{};
            fixture.set_geometry({}, {0.0F, 0.0F, -450.0F});
            fixture.calc();
            fixture.set_geometry({}, direction, {});
            fixture.calc();
            const auto expected_up = direction.y == 0.0F ? TVec3f(0.0F, 1.0F, 0.0F) : TVec3f(0.0F, 0.0F, 1.0F);
            require_vector(fixture.pose->mUpVec, expected_up,
                           "later invalid up must reuse the previous axis for parallel/opposite views and transport it for a turn");
            require_near(fixture.pose->mUpVec.dot(direction), 0.0F,
                         "recovered up must remain perpendicular to the requested view direction");
        }
    }

    void test_ideal_movement_accumulates_speed_and_preserves_target_motion() {
        auto fixture = Fixture{};
        fixture.set_geometry({}, {0.0F, 0.0F, -500.0F});
        fixture.calc();
        fixture.original.mCalcIdeal = true;
        fixture.original.mPos.set(100.0F, 0.0F, 0.0F);
        struct Frame {
            TVec3f requested_eye;
            TVec3f expected_eye;
            float expected_speed;
        };
        const auto frames = std::array{
            Frame{{110.0F, 20.0F, 0.0F}, {11.0F, 20.0F, 0.0F}, 1.0F},
            Frame{{120.0F, 25.0F, 0.0F}, {23.0F, 25.0F, 0.0F}, 2.0F},
            Frame{{120.0F, 25.0F, 0.0F}, {26.0F, 25.0F, 0.0F}, 3.0F},
        };
        for (const auto& frame : frames) {
            const auto watch = frame.requested_eye + TVec3f(0.0F, 0.0F, -500.0F);
            fixture.set_geometry(frame.requested_eye, watch);
            fixture.calc();
            require_vector(fixture.pose->mPos, frame.expected_eye,
                           "ideal movement must preserve desired translation and accelerate its remaining catch-up by one");
            require_vector(fixture.pose->mWatchPos, watch, "ideal eye correction must retain the requested watch position");
            require_vector(fixture.original.mPos, frame.requested_eye,
                           "ideal movement must retain the uncorrected desired position for the next frame");
            require_near(fixture.original.mSpeed, frame.expected_speed, "ideal catch-up speed must accumulate between frames");
            require(fixture.original.mCalcIdeal, "distant ideal movement must remain active");
        }
    }

    void test_ideal_movement_braking_cap_and_strict_endpoint() {
        struct Case {
            float distance;
            float speed;
            float expected_position;
            float expected_speed;
            bool remains_active;
        };
        constexpr auto cases = std::array{
            Case{40.0F, 10.0F, 9.0F, 9.0F, true},
            Case{50.0F, 10.0F, 11.0F, 11.0F, true},
            Case{5.5F, 3.5F, 4.5F, 4.5F, true},
            Case{10000.0F, 99.5F, 100.0F, 100.0F, true},
            Case{0.5F, 0.0F, 0.5F, 0.0F, false},
            Case{0.25F, 1.0F, 0.25F, 0.0F, false},
            Case{1.0F, 0.0F, 1.0F, 0.0F, false},
            Case{0x1.000002p0F, 0.0F, 1.0F, 1.0F, true},
        };
        require(std::bit_cast<std::uint32_t>(TVec3f(0x1.000002p0F, 0.0F, 0.0F).length()) == 0x3F800000U,
                "the float above one must produce an exact unit retail magnitude and exercise the strict endpoint comparison");
        for (const auto& sample : cases) {
            auto fixture = Fixture{};
            fixture.original.mCalcIdeal = true;
            fixture.original.mPos.set(sample.distance, 0.0F, 0.0F);
            fixture.original.mSpeed = sample.speed;
            auto requested = TVec3f{sample.distance, 0.0F, 0.0F};
            fixture.original.moveToIdealPosition(&requested);
            require_vector(requested, {sample.expected_position, 0.0F, 0.0F},
                           "ideal movement must preserve original braking, fractional-speed, cap and endpoint boundaries");
            require_near(fixture.original.mSpeed, sample.expected_speed, "ideal movement must preserve its original speed state");
            require(fixture.original.mCalcIdeal == sample.remains_active,
                    "ideal activity must use the rounded retail magnitude at the strict speed threshold");
            if (sample.distance == 0x1.000002p0F) {
                fixture.pose->mPos.set(requested);
                fixture.original.moveToIdealPosition(&requested);
                require(!fixture.original.mCalcIdeal && fixture.original.mSpeed == 0.0F,
                        "the calculation after an exact endpoint must stop the remaining ideal-movement state");
            }
        }
    }

    void test_reset_and_zero_frame_flag_source_semantics() {
        auto fixture = Fixture{};
        fixture.set_geometry({}, {0.0F, 450.0F, 0.0F}, {});
        fixture.calc();
        fixture.original.mCalcIdeal = true;
        fixture.original.mSpeed = 8.0F;
        fixture.original._24 = 99;
        fixture.original.mIsResetting = true;
        fixture.original.mIsZeroFrameMoveOff = true;
        fixture.set_geometry({10.0F, 20.0F, 30.0F}, {460.0F, 20.0F, 30.0F}, {});
        fixture.calc();
        require_vector(fixture.pose->mPos, {10.0F, 20.0F, 30.0F}, "reset must immediately use the requested eye");
        require_vector(fixture.pose->mUpVec, {0.0F, 1.0F, 0.0F},
                       "reset must recover up from the canonical frame instead of transporting the old +Z up");
        require(!fixture.original.mCalcIdeal && !fixture.original.mIsResetting && !fixture.original.mStartPose &&
                    fixture.original._24 == 0,
                "reset must consume the original reset fields and execute one start calculation");
        require_near(fixture.original.mSpeed, 8.0F, "the original reset clears ideal activity without clearing stored speed");
        require(fixture.original.mIsZeroFrameMoveOff, "the original start calculation must leave the zero-frame flag untouched");
        fixture.calc();
        require(!fixture.original.mIsZeroFrameMoveOff, "the original safe calculation must consume the zero-frame flag");
        require_vector(fixture.pose->mPos, {10.0F, 20.0F, 30.0F},
                       "the zero-frame flag must not introduce movement behavior absent from the original calculation");
    }

    constexpr auto isolated_view = smgpc::camera::OriginalCameraViewFlags{
        .interpolation_off = true, .collision_off = true, .zero_frame_move_off = true};

    void test_native_owner_manager_fov_feedback_and_raw_pose() {
        auto manager = Manager{};
        manager.pose->mPos.set(10.0F, 20.0F, 30.0F);
        manager.pose->mWatchPos.set(manager.pose->mPos);
        manager.pose->mFovy = 70.0F;
        const auto raw = *manager.pose;
        {
            auto owner = smgpc::camera::OriginalCameraView{};
            owner.original().mIsRepulsionOff = true;
            auto projection = smgpc::camera::CameraPose{};
            projection.aspect_ratio = 2.0F;
            projection.near_clip = 15.0F;
            const auto result = owner.update(manager.original, nullptr, projection, isolated_view);
            require_vector(owner.processed_pose().mWatchPos, {10.0F, 20.0F, 29.0F},
                           "the native view phase must execute OnlyCamera before constructing the camera matrix");
            require_near(owner.processed_pose().mFovy, 40.0F, "the processed pose must retain the original OnlyCamera FOV field");
            require_near(result.fovy_degrees, 70.0F, "rendered FOV must come directly from the manager");
            require(result.aspect_ratio == 2.0F && result.near_clip == 15.0F,
                    "the native owner must retain the caller's projection metadata");
            require_pose(*manager.pose, raw, "the native view phase must preserve every raw manager pose field");
            const auto expected_translation = std::array{raw.mPos.x, raw.mPos.y, raw.mPos.z};
            for (int row = 0; row != 3; ++row) {
                for (int column = 0; column != 4; ++column) {
                    const float expected = column == 3 ? expected_translation[row] : (row == column ? 1.0F : 0.0F);
                    require_near(manager.original.mMatrix.mMtx[row][column], expected,
                                 "the manager must receive the actual inverse view as matrix feedback");
                    require_near(owner.output().inverse_view.mMtx[row][column], expected,
                                 "matrix feedback must match the view phase's published output");
                }
            }
            require(owner.pose_processor().mIsZeroFrameMoveOff,
                    "the native zero-frame request must reach the original start-pose flag");
            (void)owner.update(manager.original, nullptr, projection, isolated_view);
            require(!owner.pose_processor().mIsZeroFrameMoveOff,
                    "a later native zero-frame request must be cleared by the original safe-pose calculation");
        }
        require_pose(*manager.pose, raw, "destroying the native owner must leave its borrowed manager and pose alive");
        require(smgpc::compat::bound_camera_view_output() == nullptr,
                "destroying a native owner must not leave a bound view-output pointer");
    }

    void test_native_owner_retains_pose_across_manager_lifetimes_and_reset() {
        auto owner = smgpc::camera::OriginalCameraView{};
        owner.original().mIsRepulsionOff = true;
        {
            auto first = Manager{};
            first.pose->mWatchPos.set(450.0F, 0.0F, 0.0F);
            (void)owner.update(first.original, nullptr, {}, isolated_view);
        }
        require_vector(owner.processed_pose().mWatchPos, {450.0F, 0.0F, 0.0F},
                       "the view owner must retain its processed pose after the input manager is destroyed");
        auto second = Manager{};
        second.pose->mPos.set(10.0F, 20.0F, 30.0F);
        second.pose->mWatchPos.set(second.pose->mPos);
        (void)owner.update(second.original, nullptr, {}, isolated_view);
        require_vector(owner.processed_pose().mWatchPos, {460.0F, 20.0F, 30.0F},
                       "a manager switch must retain the director's previous watch vector until an explicit pose reset");
        owner.request_pose_reset();
        require(owner.pose_processor().mIsResetting, "a native reset request must mark the original state without calculating early");
        require_vector(owner.processed_pose().mWatchPos, {460.0F, 20.0F, 30.0F},
                       "requesting reset must preserve the visible processed pose until the next camera phase");
        (void)owner.update(second.original, nullptr, {}, isolated_view);
        require_vector(owner.processed_pose().mWatchPos, {10.0F, 20.0F, 29.0F},
                       "the next native update must consume reset using the original first-pose fallback");
        require(!owner.pose_processor().mIsResetting, "the native pose reset must be consumed exactly once");
    }
}  // namespace

int main() {
    struct TestCase {
        std::string_view name;
        void (*run)();
    };
    const auto tests = std::array{
        TestCase{"first-frame watch distances", test_first_frame_watch_distances},
        TestCase{"later close watch", test_later_close_watch_preserves_previous_direction_and_distance},
        TestCase{"first/safe up and pose fields", test_first_and_safe_up_vectors_and_pose_fields},
        TestCase{"invalid up recovery", test_invalid_up_recovery_and_transport},
        TestCase{"ideal movement and target translation", test_ideal_movement_accumulates_speed_and_preserves_target_motion},
        TestCase{"ideal braking and endpoint boundaries", test_ideal_movement_braking_cap_and_strict_endpoint},
        TestCase{"reset and zero-frame flag", test_reset_and_zero_frame_flag_source_semantics},
        TestCase{"native manager FOV and matrix feedback", test_native_owner_manager_fov_feedback_and_raw_pose},
        TestCase{"native owner lifetime and reset", test_native_owner_retains_pose_across_manager_lifetimes_and_reset},
    };
    std::size_t passed = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            ++passed;
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        }
    }
    std::cout << passed << '/' << tests.size() << " OnlyCamera tests passed\n";
    return passed == tests.size() ? 0 : 1;
}
