#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraViewInterpolator.hpp"
#include "Game/Util/CameraUtil.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

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

    CameraPoseParam make_pose(const TVec3f& eye, const TVec3f& watch, float fovy) {
        auto pose = CameraPoseParam{};
        pose.mPos.set(eye);
        pose.mWatchPos.set(watch);
        pose.mUpVec.set(0.0F, 1.0F, 0.0F);
        pose.mWatchUpVec.set(0.0F, 1.0F, 0.0F);
        pose.mFovy = fovy;
        return pose;
    }

    void update_camera(CameraDirector& director, CameraViewInterpolator& original,
                       const CameraPoseParam& pose, bool anti_oscillation) {
        TPos3f matrix;
        director.calcViewMtxFromPoseParam(&matrix, &pose);
        original.mIsAntiOscillation = anti_oscillation;
        original.mIsCollisionOff = true;
        original.updateCameraMtx(matrix, pose.mWatchPos, nullptr, pose.mFovy);
    }

    void test_cut_output_and_interpolation_off_gate(CameraDirector& director, CameraViewInterpolator& original) {
        original.mIsRepulsionOff = true;
        original.setInterpolation(0U);
        const auto pose = make_pose({0.0F, 0.0F, 600.0F}, {}, 40.0F);
        update_camera(director, original, pose, false);
        require_near(MR::getCameraInvViewMtx().mMtx[2][3], 600.0F, 0.0001F, "a zero-frame cut must publish the requested eye immediately");
        require_near(MR::getFovy(), 40.0F, 0.0001F, "a forced camera cut must also publish the requested FOV");
        require_near(MR::getCameraViewMtx().mMtx[2][3], -600.0F, 0.0001F,
                     "the original update must publish an actual view matrix with inverse camera translation");
        require_near(MR::getCameraInvViewMtx().mMtx[2][3], 600.0F, 0.0001F,
                     "the output context must retain the matching inverse view matrix");
        require(!original.mIsForceCameraChange && !original.mIsCollisionOff,
                "the original frame must consume one-shot cut flags");

        original.setInterpolation(7U);
        original.mInterpolateTimer = 2U;
        original.mIsInterpolationOff = true;
        original.setInterpolation(0U);
        require(original.mInterpolateTime == 7U && original.mInterpolateTimer == 2U &&
                    !original.mIsForceCameraChange,
                "setInterpolation must preserve the active timer when the original interpolation-off gate is set");
        update_camera(director, original, pose, false);
        require(!original.mIsInterpolationOff,
                "updateCameraMtx must clear the original one-frame interpolation-off gate");
        original.setInterpolation(0U);
        require(!original.isInterpolating() && original.isInterpolatingNearlyEnd() && original.mIsForceCameraChange,
                "an ungated zero-frame request must restore the original forced-cut state");
    }

    void test_recursive_switching_timer_and_fov(CameraDirector& director, CameraViewInterpolator& original) {
        original.mIsRepulsionOff = true;
        const auto before = make_pose({0.0F, 0.0F, 600.0F}, {}, 40.0F);
        const auto after = make_pose({100.0F, 0.0F, 1200.0F}, {100.0F, 0.0F, 0.0F}, 80.0F);
        update_camera(director, original, before, false);
        original.setInterpolation(4U);
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
            update_camera(director, original, after, false);
            require_near(MR::getCameraInvViewMtx().mMtx[0][3], frame.eye_x, 0.0003F, "camera switching must retain the original recursive watch interpolation");
            require_near(MR::getCameraInvViewMtx().mMtx[2][3], frame.eye_z, 0.0003F, "camera switching must retain the original recursive distance interpolation");
            require_near(MR::getFovy(), frame.fovy, 0.0001F, "camera switching must use the same squared rate for FOV");
            require(original.mInterpolateTimer == frame.timer && original.isInterpolating() == frame.interpolating,
                    "the original timer must saturate before its next rate-one calculation reaches the exact endpoint");
        }
    }

    void test_separate_anti_oscillation_position_and_fov_rates(CameraDirector& director, CameraViewInterpolator& original) {
        original.mIsRepulsionOff = true;
        const auto before = make_pose({0.0F, 0.0F, 600.0F}, {}, 40.0F);
        const auto after = make_pose({100.0F, 0.0F, 1000.0F}, {100.0F, 0.0F, 0.0F}, 80.0F);
        update_camera(director, original, before, true);
        update_camera(director, original, after, true);
        require_near(MR::getCameraInvViewMtx().mMtx[0][3], 30.0F, 0.0001F, "anti-oscillation must advance position by the original 30 percent rate");
        require_near(MR::getCameraInvViewMtx().mMtx[2][3], 720.0F, 0.0001F, "anti-oscillation must retain 70 percent of the previous camera translation");
        require_near(MR::getFovy(), 44.0F, 0.0001F, "anti-oscillation must advance FOV by its separate 10 percent rate");
        update_camera(director, original, after, true);
        require_near(MR::getCameraInvViewMtx().mMtx[0][3], 51.0F, 0.0001F, "anti-oscillation must continue from the preceding output position");
        require_near(MR::getCameraInvViewMtx().mMtx[2][3], 804.0F, 0.0001F, "anti-oscillation must preserve position state between frames");
        require_near(MR::getFovy(), 47.6F, 0.0001F, "FOV damping must preserve its independently accumulated state");
        original.setInterpolation(0U);
        update_camera(director, original, after, true);
        require_near(MR::getCameraInvViewMtx().mMtx[0][3], 100.0F, 0.0001F, "a forced cut must bypass position damping");
        require_near(MR::getFovy(), 80.0F, 0.0001F, "a forced cut must bypass FOV damping");
    }

    void test_nearly_end_distance_and_rotation_thresholds(CameraDirector&, CameraViewInterpolator& original) {
        original.mTargetMtx.identity();
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

} // namespace

int main() {
    return smgpc::test::run_stage_resource_process("camera-view-interpolator", [] {
        auto* director = MR::getCameraDirector();
        require(director && director->mViewInterpolator, "the original process must own its camera interpolator");
        auto& original = *director->mViewInterpolator;
        const auto saved_interpolator = original;
        const auto saved_view = MR::getCameraViewMtx();
        const auto saved_fovy = MR::getFovy();
        const auto restore = [&] {
            original = saved_interpolator;
            MR::setCameraViewMtx(saved_view, false, false, original.mTargetPosition);
            MR::setFovy(saved_fovy);
        };
        struct TestCase {
            std::string_view name;
            void (*run)(CameraDirector&, CameraViewInterpolator&);
        };
        const auto tests = std::array{
            TestCase{"cut output and interpolation gate", test_cut_output_and_interpolation_off_gate},
            TestCase{"recursive switching timer and FOV", test_recursive_switching_timer_and_fov},
            TestCase{"separate damping rates", test_separate_anti_oscillation_position_and_fov_rates},
            TestCase{"nearly-end thresholds", test_nearly_end_distance_and_rotation_thresholds},
        };
        try {
            for (const auto& test : tests) {
                original = saved_interpolator;
                original.mIsInterpolationOff = false;
                original.setInterpolation(0U);
                original.mIsRepulsionOff = true;
                original.mRate = 0.7F;
                original.mTargetObj = nullptr;
                original.mCalcState = CameraViewInterpolator::CalcState_Invalid;
                test.run(*director, original);
                std::cout << "[PASS] " << test.name << '\n';
            }
        } catch (...) {
            restore();
            throw;
        }
        restore();
    });
}
