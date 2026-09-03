#include "Game/Camera/Camera.hpp"
#include "Game/Camera/CameraHeightArrange.hpp"
#include "Game/Camera/CameraLocalUtil.hpp"
#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraParallel.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraTargetObj.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "compat/CameraLocalUtilRuntime.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/wpad.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
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

    template <typename Function>
    void require_missing_owner(Function&& function) {
        auto threw = false;
        try {
            function();
        } catch (const std::logic_error&) {
            threw = true;
        }
        require(threw, "a missing or mismatched original camera owner must be reported");
    }

    class TestManager final : public CameraMan {
    public:
        TestManager() : CameraMan("ScopeTestManager") {}
        ~TestManager() override { delete mPoseParam; }
    };

    class TestCamera final : public Camera {
    public:
        explicit TestCamera(CameraMan* manager) : Camera("ScopeTestCamera") { mCameraMan = manager; }
        ~TestCamera() override { delete mPoseParam; }
        CameraTargetObj* calc() override { return CameraLocalUtil::getTarget(this); }
    };

    class TestTarget final : public CameraTargetObj {
    public:
        TestTarget() : CameraTargetObj("ScopeTestTarget") {}
        const TVec3f& getPosition() const override { return position; }
        const TVec3f& getUpVec() const override { return up; }
        const TVec3f& getFrontVec() const override { return front; }
        const TVec3f& getSideVec() const override { return side; }
        const TVec3f& getLastMove() const override { return move; }
        const TVec3f& getGroundPos() const override { return position; }
        const TVec3f& getGravityVector() const override { return gravity; }

        TVec3f position{0.0F, 0.0F, 0.0F};
        TVec3f up{0.0F, 1.0F, 0.0F};
        TVec3f front{0.0F, 0.0F, 1.0F};
        TVec3f side{1.0F, 0.0F, 0.0F};
        TVec3f move{0.0F, 0.0F, 0.0F};
        TVec3f gravity{0.0F, -1.0F, 0.0F};
    };

    class TestParallel final : public CameraParallel {
    public:
        explicit TestParallel(CameraMan& manager) : CameraParallel("PadRoundTestCamera") { mCameraMan = &manager; }
        ~TestParallel() override {
            delete mVPan->mNextParam;
            delete mVPan->mCurrParam;
            delete mVPan;
            delete mPoseParam;
        }
    };

    void test_scope_identity_and_nested_restoration() {
        using smgpc::compat::OriginalCameraMode;
        using smgpc::compat::ScopedCameraTargetBinding;
        auto first_manager = TestManager{};
        auto second_manager = TestManager{};
        auto first_camera = TestCamera(&first_manager);
        auto second_camera = TestCamera(&second_manager);
        auto invalid_camera = TestCamera(nullptr);
        auto first_target = TestTarget{};
        auto second_target = TestTarget{};
        require_missing_owner([&] { (void)first_camera.calc(); });
        require_missing_owner([&] { (void)CameraLocalUtil::getTarget(&first_manager); });
        require_missing_owner([] { (void)MR::isFirstPersonCamera(); });
        {
            const auto outer = ScopedCameraTargetBinding(first_camera, first_target, OriginalCameraMode::Game);
            require(first_camera.calc() == &first_target && CameraLocalUtil::getTarget(&first_manager) == &first_target &&
                        !MR::isFirstPersonCamera(), "the outer scope must retain its exact target and original game mode");
            require_missing_owner([&] { (void)second_camera.calc(); });
            require_missing_owner([&] { (void)CameraLocalUtil::getTarget(&second_manager); });
            require_missing_owner([&] {
                const auto invalid = ScopedCameraTargetBinding(invalid_camera, second_target, OriginalCameraMode::Subjective);
            });
            require(first_camera.calc() == &first_target && !MR::isFirstPersonCamera(),
                    "a failed scope must preserve its caller's binding");
            try {
                const auto inner = ScopedCameraTargetBinding(second_camera, second_target, OriginalCameraMode::Subjective);
                require(second_camera.calc() == &second_target &&
                            CameraLocalUtil::getTarget(&second_manager) == &second_target && MR::isFirstPersonCamera(),
                        "the inner scope must retain its distinct target and subjective mode");
                require_missing_owner([&] { (void)first_camera.calc(); });
                throw std::runtime_error("unwind the inner camera calculation");
            } catch (const std::runtime_error& error) {
                require(std::string_view(error.what()) == "unwind the inner camera calculation",
                        "the nested test failed before exception restoration");
            }
            require(first_camera.calc() == &first_target && !MR::isFirstPersonCamera(),
                    "unwinding a nested scope must restore the original target and mode");
        }
        require_missing_owner([&] { (void)first_camera.calc(); });
        require_missing_owner([] { (void)MR::isFirstPersonCamera(); });
    }

    void test_subjective_mode_suppresses_original_pad_helpers() {
        auto scheduler = smgpc::runtime::SceneScheduler{};
        const auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding(scheduler);
        auto dvd = smgpc::runtime::DvdFileSystemService(std::filesystem::path{});
        auto demo = smgpc::compat::DemoSceneRuntime(dvd, {});
        auto manager = TestManager{};
        auto camera = TestCamera(&manager);
        auto target = TestTarget{};
        const auto binding = smgpc::compat::ScopedCameraTargetBinding(
            camera, target, smgpc::compat::OriginalCameraMode::Subjective);
        // Active reset and round buttons must be suppressed by the original
        // subjective gate before their input checks.
        auto& wpad = aurora::wpad_service();
        wpad.clear();
        wpad.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_C | WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT);
        require(!CameraLocalUtil::testCameraPadButtonReset() && !CameraLocalUtil::testCameraPadTriggerReset() &&
                    !CameraLocalUtil::testCameraPadTriggerRoundLeft() && !CameraLocalUtil::testCameraPadTriggerRoundRight(),
                "subjective mode must suppress held reset, reset trigger, and both round triggers");
        wpad.clear();
    }

    void test_original_parallel_pad_round_trajectory() {
        auto scheduler = smgpc::runtime::SceneScheduler{};
        const auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding(scheduler);
        auto dvd = smgpc::runtime::DvdFileSystemService(std::filesystem::path{});
        auto demo = smgpc::compat::DemoSceneRuntime(dvd, {});
        auto& wpad = aurora::wpad_service();
        wpad.clear();
        auto manager = TestManager{};
        auto camera = TestParallel(manager);
        auto target = TestTarget{};
        const auto binding = smgpc::compat::ScopedCameraTargetBinding(
            camera, target, smgpc::compat::OriginalCameraMode::Game);
        camera.setParam(TVec2f(0.0F, 0.0F), 3000.0F, true);
        manager.mRequestLOfsReset = true;
        wpad.set_button_mask(WPAD_CHAN0, 0U);
        camera.reset();

        const auto step = [&](std::uint32_t held) {
            wpad.begin_frame();
            wpad.set_button_mask(WPAD_CHAN0, held);
            require(camera.calc() == &target, "the original camera must retain the bound target while processing pad input");
        };
        const auto near = [](float lhs, float rhs) { return std::fabs(lhs - rhs) < 0.00001F; };
        const auto round_interval = MR::toRadian(45.0F);
        step(WPAD_BUTTON_RIGHT);
        require(camera.mIsRounding && near(camera.mRoundAngle, 0.0F) && near(camera.mRoundTarget, round_interval),
                "the right trigger must schedule the original one-eighth round before moving its angle");
        for (auto frame = 1; frame <= 10; ++frame) {
            step(WPAD_BUTTON_RIGHT);
            const auto expected = std::min(static_cast<float>(frame) * 0.08F, round_interval);
            require(near(camera.mRoundAngle, expected), "the original camera must advance its retained round angle by the retail per-frame rate");
        }
        require(!camera.mIsRounding && near(camera.mRoundAngle, round_interval), "rounding must stop at the original one-eighth target");
        step(WPAD_BUTTON_RIGHT);
        require(!camera.mIsRounding && near(camera.mRoundTarget, round_interval), "a held right button must not create a second trigger");

        step(0U);
        step(WPAD_BUTTON_C);
        require(camera.mIsRounding && near(camera.mRoundTarget, 0.0F) && near(camera.mRoundAngle, round_interval),
                "the original reset trigger must schedule return to the authored angle");
        for (auto frame = 0; frame < 10; ++frame) {
            step(0U);
        }
        require(!camera.mIsRounding && near(camera.mRoundAngle, 0.0F), "the original reset trajectory must settle at the authored angle");
        wpad.clear();
    }
}  // namespace

int main() {
    try {
        test_scope_identity_and_nested_restoration();
        test_subjective_mode_suppresses_original_pad_helpers();
        test_original_parallel_pad_round_trajectory();
        std::cout << "[pass] original camera local target scope and mode gates\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "[fail] " << exception.what() << '\n';
        return 1;
    }
}
