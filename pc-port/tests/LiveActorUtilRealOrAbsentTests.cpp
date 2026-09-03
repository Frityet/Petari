#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/MaterialCtrlCompat.hpp"
#include "render/J3dMatrix.hpp"
#include "runtime/RuntimeContext.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <functional>
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

    void require_unavailable(const std::function<void()>& operation, std::string_view message) {
        auto unavailable = false;
        try {
            operation();
        } catch (const std::logic_error&) {
            unavailable = true;
        }
        require(unavailable, message);
    }

    void require_near(float actual, float expected, std::string_view message) {
        require(std::abs(actual - expected) < 0.00001F, message);
    }

    [[nodiscard]] std::filesystem::path require_real_disc() {
        if (const auto* configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            const auto path = std::filesystem::path(configured);
            require(std::filesystem::is_regular_file(path),
                    "SMGPC_REAL_DISC must name the real RMGK01 image");
            return path;
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        require(!error, "the LiveActor animation proof requires a readable working directory");
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) && !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        throw std::runtime_error(
            "the LiveActor animation proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    void test_real_tico_bck_and_joint_refresh() {
        const auto disc_path = require_real_disc();
        aurora_dvd_close();
        const auto disc_string = disc_path.string();
        require(aurora_dvd_open(disc_string.c_str()),
                "the selected RMGK01 image must open through Aurora DVD");
        struct DiscCloseGuard final {
            ~DiscCloseGuard() { aurora_dvd_close(); }
        } disc_close_guard;
        DVDInit();

        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 640,
            .height = 456,
            .title = "SMG PC LiveActor BCK/joint proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        const auto renderer_context = smgpc::render::ScopedAuroraRendererContext(renderer);
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);

        auto actor = LiveActor("Tico BCK/joint proof");
        actor.initModelManagerWithAnm("Tico", "Tico", false);
        MR::startBck(&actor, "Wait", nullptr);
        auto* model = smgpc::compat::actor_model(&actor);
        auto* controller = MR::getBckCtrl(&actor);
        require(model != nullptr && controller->getEnd() > 8,
                "retail Tico Wait must expose a real model and nontrivial BCK controller");

        controller->mState = 0U;
        controller->setRate(1.0F);
        const auto raw_frame = static_cast<float>(controller->getEnd()) + 3.25F;
        MR::setBckFrame(&actor, raw_frame);
        require(controller->getFrame() == raw_frame && controller->getRate() == 1.0F &&
                    controller->getState() == 0U && model->bck_frame(0U) == raw_frame,
                "continuing setBckFrame must raw-store phase without changing rate or state");

        MR::setBckFrameAndStop(&actor, -2.5F);
        require(controller->getFrame() == -2.5F && controller->getRate() == 0.0F &&
                    controller->getState() == 0U && !MR::isBckStopped(&actor) &&
                    model->bck_frame(0U) == -2.5F &&
                    model->is_bck_stopped(0U) == std::optional<bool>{false},
                "setBckFrameAndStop must stop rate without clamping phase or fabricating terminal state");
        MR::setBckFrame(&actor, 6.0F);
        require(controller->getFrame() == 6.0F && controller->getRate() == 0.0F &&
                    !MR::isBckStopped(&actor),
                "continuing setBckFrame must preserve an already-zero playback rate");

        controller->setAttribute(J3DFrameCtrl::EMode_NONE);
        controller->mState = 0U;
        MR::setBckFrameAndStop(&actor, static_cast<float>(controller->getEnd()));
        actor.movement();
        actor.calcAnmMtx();
        require(controller->checkState(1U) && controller->getRate() == 0.0F &&
                    MR::isBckStopped(&actor),
                "a frozen one-shot at end must receive a terminal update without changing its zero rate");

        controller->mState = 1U;
        MR::setBckFrame(&actor, 5.0F);
        require(MR::isBckStopped(&actor) &&
                    model->is_bck_stopped(999999U) == std::optional<bool>{true},
                "setBckFrame must preserve the authoritative terminal state bit");

        MR::startBck(&actor, "Wait", nullptr);
        controller = MR::getBckCtrl(&actor);
        controller->setAttribute(J3DFrameCtrl::EMode_NONE);
        controller->setFrame(static_cast<float>(controller->getEnd()) - 0.25F);
        controller->setRate(1.0F);
        controller->mState = 0U;
        actor.movement();
        actor.calcAnmMtx();
        require(controller->checkState(1U) && controller->getRate() == 1.0F &&
                    MR::isBckStopped(&actor) &&
                    model->is_bck_stopped(0U) == std::optional<bool>{true},
                "a natural one-shot terminal update must propagate state bit 1 and retain its prior rate");

        MR::startBck(&actor, "Wait", nullptr);
        controller = MR::getBckCtrl(&actor);
        controller->setAttribute(J3DFrameCtrl::EMode_LOOP);
        controller->setFrame(static_cast<float>(controller->getEnd()) - 0.5F);
        controller->setRate(1.0F);
        require(MR::checkPassBckFrame(
                    &actor, static_cast<float>(controller->getEnd()) - 0.25F) &&
                    MR::checkPassBckFrame(&actor, 0.25F),
                "checkPassBckFrame must cover both intervals of a looping wrap");

        MR::setBckFrameAtRandom(&actor);
        require(controller->getFrame() == std::trunc(controller->getFrame()) &&
                    controller->getFrame() >= 0.0F &&
                    controller->getFrame() < static_cast<float>(controller->getEnd()) &&
                    controller->getRate() == 1.0F,
                "random BCK phase must truncate end*getRandom to an integer and keep playing");

        MR::setBckFrame(&actor, 4.0F);
        actor.mFlag.mIsStoppedAnim = true;
        for (auto index = 0; index < 4; ++index) {
            actor.movement();
            actor.calcAnim();
        }
        require(controller->getFrame() == 4.0F && model->bck_frame(999999U) == 4.0F,
                "stopped actor animation must not advance from global runtime time");

        actor.mFlag.mIsNoCalcAnim = true;
        controller->setFrame(7.0F);
        actor.calcAnim();
        require(model->bck_frame(0U) == 4.0F,
                "calcAnim must honor its no-calc guard before synchronizing model phase");
        actor.calcAnmMtx();
        require(model->bck_frame(0U) == 7.0F,
                "direct calcAnmMtx must synchronize phase independently of calcAnim's guard");
        actor.mFlag.mIsNoCalcAnim = false;
        actor.mFlag.mIsStoppedAnim = false;

        actor.mPosition.set(10.0F, 20.0F, 30.0F);
        actor.calcAnmMtx();
        constexpr auto joint_names = std::array{
            "AllRoot", "Body", "Eye", "LHand1", "LHand2", "RHand1", "RHand2",
            "Top", "Waist", "LFoot1", "LFoot2", "RFoot1", "RFoot2",
        };
        auto* root_joint = MR::getJointMtx(&actor, joint_names[0]);
        auto* body_joint = MR::getJointMtx(&actor, joint_names[1]);
        require(root_joint != nullptr && body_joint != nullptr,
                "retail Tico must expose its authored AllRoot and Body joints");
        const auto root_x = root_joint[0][3];
        const auto body_x = body_joint[0][3];
        for (const auto* name : joint_names) {
            require(MR::getJointMtx(&actor, name) != nullptr,
                    "every authored Tico joint must resolve before the cache refresh proof");
        }

        actor.mPosition.x += 100.0F;
        actor.calcAnmMtx();
        require(root_joint == MR::getJointMtx(&actor, "AllRoot") &&
                    body_joint == MR::getJointMtx(&actor, "Body"),
                "joint cache refresh must preserve every retained MtxPtr address");
        require_near(root_joint[0][3], root_x + 100.0F,
                     "AllRoot cache must refresh after the actor base changes");
        require_near(body_joint[0][3], body_x + 100.0F,
                     "Body cache must refresh after the actor base changes");

        auto body_at_frame_7 = std::array<float, 12U>{};
        for (auto row = 0U; row < 3U; ++row) {
            for (auto column = 0U; column < 4U; ++column) {
                body_at_frame_7[row * 4U + column] = body_joint[row][column];
            }
        }
        controller->setFrame(8.0F);
        actor.calcAnmMtx();
        auto body_animation_changed = false;
        for (auto row = 0U; row < 3U; ++row) {
            for (auto column = 0U; column < 4U; ++column) {
                body_animation_changed |=
                    std::abs(body_joint[row][column] -
                             body_at_frame_7[row * 4U + column]) > 0.00001F;
            }
        }
        require(body_joint == MR::getJointMtx(&actor, "Body") &&
                    body_animation_changed,
                "joint cache refresh must update animation in place when BCK phase changes");

        const auto view_only_x = body_joint[0][3];
        actor.mPosition.x += 50.0F;
        actor.calcViewAndEntry();
        require_near(body_joint[0][3], view_only_x,
                     "calcViewAndEntry must not recalculate animation or cached joints");
        actor.calcAnmMtx();
        require_near(body_joint[0][3], view_only_x + 50.0F,
                     "calcAnmMtx must refresh cached joints after the virtual base calculation");
    }
}

int main() {
    auto passed = 0;
    auto actor = LiveActor("animation-absence-test");
    actor.calcAndSetBaseMtx();

    require_unavailable([&] { (void)MR::isBckStopped(&actor); },
                        "an actor without a real BCK must not report a fabricated stopped state");
    require_unavailable([&] { (void)MR::getBckFrameMax(&actor); },
                        "an actor without a real BCK must not report a fabricated frame count");
    ++passed;

    require_unavailable([&] { (void)MR::isBtpStopped(&actor); },
                        "unsupported BTP playback must be explicitly unavailable");
    require_unavailable([&] { (void)MR::isBrkOneTimeAndStopped(&actor); },
                        "an actor without a real BRK must not report a fabricated stopped state");
    require_unavailable([&] { (void)MR::getBrkCtrl(&actor); },
                        "an actor without a real BRK must not expose a fabricated frame controller");
    ++passed;

    require(MR::getJointMtx(&actor, "MarioPosition") == nullptr,
            "a missing model joint must remain absent instead of substituting the actor base matrix");
    require(MR::getJointMtx(static_cast<const LiveActor*>(nullptr), "MarioPosition") == nullptr,
            "a missing actor must not manufacture a joint matrix");
    ++passed;

    require(MR::isLessStep(&actor, 1) && MR::isLessEqualStep(&actor, 0) &&
                !MR::isGreaterStep(&actor, 0) && MR::isGreaterEqualStep(&actor, 0) && !MR::isNewNerve(&actor) &&
                MR::calcNerveRate(&actor, 60) == 0.0F && MR::calcNerveRate(&actor, 0) == 1.0F,
            "LiveActor nerve comparisons must expose the complete retail comparison surface");
    require_unavailable([&] { (void)MR::isLessStep(nullptr, 1); },
                        "a missing actor must not fabricate a nerve-comparison result");
    require_unavailable([&] { (void)MR::isStep(nullptr, 0); },
                        "a missing actor must not fabricate an equal-step result");
    require_unavailable([&] { (void)MR::isGreaterEqualStep(nullptr, 0); },
                        "a missing actor must not fabricate a greater-equal-step result");
    require_unavailable([&] { (void)MR::isNewNerve(nullptr); },
                        "a missing actor must not fabricate a new-nerve state");
    require_unavailable([&] { (void)MR::calcNerveRate(nullptr, 60); },
                        "a missing actor must not fabricate a nerve rate");
    ++passed;

    require_unavailable([&] { (void)MR::initDLMakerProjmapEffectMtxSetter(&actor); },
                        "a projection material controller must not exist without a real actor model renderer");
    actor.initModelManagerWithAnm("", "", false);
    smgpc::compat::set_actor_base_matrix(&actor, smgpc::render::J3dMatrix3x4{{
        2.0F, 0.0F, 0.0F, 4.0F,
        0.0F, 4.0F, 0.0F, 8.0F,
        0.0F, 0.0F, 5.0F, 10.0F,
    }});
    auto* projection = MR::initDLMakerProjmapEffectMtxSetter(&actor);
    require(projection != nullptr,
            "a projection material controller must bind to the actor's real LiveActorModel renderer");
    projection->updateMtxUseBaseMtx();
    const auto& projection_matrix = smgpc::compat::projmap_effect_matrix(projection);
    require_near(projection_matrix.m[0U], 0.5F,
                 "projection material matrix must invert actor X scale");
    require_near(projection_matrix.m[5U], 0.25F,
                 "projection material matrix must invert actor Y scale");
    require_near(projection_matrix.m[10U], 0.2F,
                 "projection material matrix must invert actor Z scale");
    require_near(projection_matrix.m[3U], -2.0F,
                 "projection material matrix must invert actor X translation");
    require_near(projection_matrix.m[7U], -2.0F,
                 "projection material matrix must invert actor Y translation");
    require_near(projection_matrix.m[11U], -2.0F,
                 "projection material matrix must invert actor Z translation");
    projection->updateMtxUseBaseMtxWithLocalOffset(TVec3f{2.0F, 4.0F, 5.0F});
    require_near(projection_matrix.m[3U], -4.0F,
                 "projection material local offset must compose before inverse X translation");
    require_near(projection_matrix.m[7U], -6.0F,
                 "projection material local offset must compose before inverse Y translation");
    require_near(projection_matrix.m[11U], -7.0F,
                 "projection material local offset must compose before inverse Z translation");
    projection->update();
    ++passed;

    test_real_tico_bck_and_joint_refresh();
    ++passed;

    std::cout << "LiveActorUtil real-or-absent tests passed: " << passed << "/6\n";
    return 0;
}
