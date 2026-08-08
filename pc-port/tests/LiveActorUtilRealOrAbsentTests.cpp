#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "render/J3dMatrix.hpp"

#include <cmath>
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
    actor.setBaseMatrix(smgpc::render::J3dMatrix3x4{{
        2.0F, 0.0F, 0.0F, 4.0F,
        0.0F, 4.0F, 0.0F, 8.0F,
        0.0F, 0.0F, 5.0F, 10.0F,
    }});
    auto* projection = MR::initDLMakerProjmapEffectMtxSetter(&actor);
    require(projection != nullptr,
            "a projection material controller must bind to the actor's real LiveActorModel renderer");
    auto translation = TVec3f{};
    projection->getBaseTrans(&translation);
    require_near(translation.x, 4.0F, "projection material translation must preserve actor X");
    require_near(translation.y, 8.0F, "projection material translation must preserve actor Y");
    require_near(translation.z, 10.0F, "projection material translation must preserve actor Z");
    projection->updateMtxUseBaseMtx();
    require_near(projection->mBaseMtx.mMtx[0][0], 0.5F,
                 "projection material matrix must invert actor X scale");
    require_near(projection->mBaseMtx.mMtx[1][1], 0.25F,
                 "projection material matrix must invert actor Y scale");
    require_near(projection->mBaseMtx.mMtx[2][2], 0.2F,
                 "projection material matrix must invert actor Z scale");
    require_near(projection->mBaseMtx.mMtx[0][3], -2.0F,
                 "projection material matrix must invert actor X translation");
    require_near(projection->mBaseMtx.mMtx[1][3], -2.0F,
                 "projection material matrix must invert actor Y translation");
    require_near(projection->mBaseMtx.mMtx[2][3], -2.0F,
                 "projection material matrix must invert actor Z translation");
    projection->updateMtxUseBaseMtxWithLocalOffset(TVec3f{2.0F, 4.0F, 5.0F});
    require_near(projection->mBaseMtx.mMtx[0][3], -4.0F,
                 "projection material local offset must compose before inverse X translation");
    require_near(projection->mBaseMtx.mMtx[1][3], -6.0F,
                 "projection material local offset must compose before inverse Y translation");
    require_near(projection->mBaseMtx.mMtx[2][3], -7.0F,
                 "projection material local offset must compose before inverse Z translation");
    projection->update();
    ++passed;

    std::cout << "LiveActorUtil real-or-absent tests passed: " << passed << "/5\n";
    return 0;
}
