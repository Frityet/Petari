#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/MapObj/ClipAreaHolder.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/LiveActor/ClippingJudge.hpp"
#include "Game/LiveActor/ClippingActorHolder.hpp"
#include "Game/LiveActor/ClippingActorInfo.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/ViewGroupCtrl.hpp"
#include "Game/LiveActor/ShadowVolumeSphere.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "OriginalAsyncHeapSelection.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "resource/TextEncoding.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    class ProbeActor final : public LiveActor {
    public:
        ProbeActor() : LiveActor("ActorPhysicsProbe") {
        }

        void calcAnim() override {
            ++calc_anim_count;
            LiveActor::calcAnim();
        }

        int calc_anim_count = 0;
    };

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, std::string_view message) {
        require(std::abs(actual - expected) < 0.0001F, message);
    }
}  // namespace

int main() {
    return smgpc::test::run_stage_resource_process("game-actor-physics", [] {
    auto passed = 0;
    auto domain = smgpc::scene::current_scene_allocation_domain();
    require(bool(domain), "actor physics requires the actual original scene heap");
    smgpc::test::OriginalAsyncHeapSelection game(domain);

    {
        ProbeActor actor;
        actor.makeActorAppeared();
        actor.initHitSensor(2);
        auto* first = MR::addHitSensor(&actor, "first", 1U, 1U, 10.0F, {});
        auto* second = MR::addHitSensor(&actor, "second", 1U, 1U, 10.0F, {});
        first->addHitSensor(second);
        actor.initBinder(50.0F, 0.0F, 8U);
        actor.mBinder->_158 = 1.0F;
        actor.mBinder->mWallInfo.mParentTriangle.mNormals[0].set(1.0F, 0.0F, 0.0F);
        require(MR::isBindedWall(&actor), "the actor begins with an actual original Binder wall contact");
        require(MR::getWallNormal(&actor) == &actor.mBinder->mWallInfo.mParentTriangle.mNormals[0],
                "wall normal queries return the original Binder triangle storage");
        actor.mFlag.mIsNoCalcAnim = true;
        actor.calc_anim_count = 0;

        MR::resetPosition(&actor, TVec3f{10.0F, 20.0F, 30.0F});
        require(first->mSensorCount == 0U, "resetPosition must clear real HitSensor contacts");
        require(!MR::isBindedWall(&actor),
                "resetPosition must clear original Binder contact immediately without another integration tick");
        require(actor.calc_anim_count == 1 && actor.mFlag.mIsNoCalcAnim,
                "resetPosition must perform calcAnimDirect semantics and restore the no-calc flag");
        require(actor.mPosition.epsilonEquals(TVec3f{10.0F, 20.0F, 30.0F}, 0.0001F),
                "the position overload must set the requested position before reset");
        ++passed;
    }

    {
        ProbeActor actor;
        actor.mRotation.set(90.0F, 0.0F, 0.0F);
        actor.mScale.zero();
        auto axis = TVec3f{};
        MR::calcActorAxisY(&axis, &actor);
        require(axis.epsilonEquals(TVec3f{0.0F, 0.0F, 1.0F}, 0.0001F),
                "calcActorAxisY must use the actor's rotation matrix rather than scaled render axes");

        actor.initBinder(50.0F, 0.0F, 8U);
        actor.mBinder->_C8 = 0.0F;
        actor.mBinder->mGroundInfo.mParentTriangle.mNormals[0].set(0.0F, 1.0F, 0.0F);
        actor.mBinder->mFixReactionVector.set(0.0F, 1.0F, 0.0F);
        actor.mRotation.zero();
        actor.mGravity.set(0.0F, -1.0F, 0.0F);
        actor.mVelocity.zero();
        MR::addVelocityMoveToDirection(&actor, TVec3f{1.0F, 0.0F, 0.0F}, 3.0F);
        require(actor.mVelocity.epsilonEquals(TVec3f{3.0F, 0.0F, 0.0F}, 0.0001F),
                "directional acceleration must use the exact gravity-plane then ground-plane calculation");

        actor.mBinder->clear();
        actor.mBinder->_158 = 0.0F;
        actor.mBinder->mWallInfo.mParentTriangle.mNormals[0].set(1.0F, 0.0F, 0.0F);
        actor.mBinder->mFixReactionVector.set(1.0F, 0.0F, 0.0F);
        actor.mVelocity.set(-4.0F, 2.0F, 0.0F);
        require_near(MR::calcHitPowerToWall(&actor), 4.0F,
                     "wall hit power must use the real recorded wall normal");
        require(MR::reboundVelocityFromCollision(&actor, 0.5F, 1.0F, 0.25F),
                "a collision above threshold must rebound from Binder's fix-reaction vector");
        require(actor.mVelocity.epsilonEquals(TVec3f{2.0F, 0.5F, 0.0F}, 0.0001F),
                "rebound velocity must match the decompiled normal/tangent calculation");
        ++passed;
    }

    {
        ProbeActor actor;
        actor.makeActorAppeared();
        actor.mPosition.set(0.0F, 0.0F, 20000.0F);
        MR::setClippingTypeSphere(&actor, 100.0F);
        auto* clipping = MR::getClippingDirector()->mActorHolder->find(&actor);
        require(clipping && clipping->mActor == &actor && clipping->_4 == &actor.mPosition &&
                    clipping->_8 == 100.0F && clipping->mFarClipLevel == 6,
                "sphere configuration must preserve the original ClippingActorInfo 100m default");

        // Supply explicit test planes to the actual original judge. Camera
        // reconstruction is exercised by OriginalCameraDirectorTests.
        ClippingJudge judge("Actor clipping plane fixture");
        auto* director = MR::getClippingDirector();
        struct RestoreJudge {
            ClippingDirector* director;
            ClippingJudge* previous;
            ~RestoreJudge() { director->mJudge = previous; }
        } restore{director, director->mJudge};
        director->mJudge = &judge;
        ViewGroupCtrlDataEntry view_group{};
        clipping->_14 = &view_group;
        const TVec3f normals[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        const TVec3f points[] = {{-30000, 0, 0}, {30000, 0, 0}, {0, -30000, 0}, {0, 30000, 0}, {0, 0, 500}, {0, 0, 30000}};
        for (unsigned i = 0; i < 6; ++i) judge.mFrustum.mPlanes[i].set(normals[i], points[i]);
        judge.mClipFrustums[6] = judge.mFrustum;
        judge.mClipFrustums[6].mPlanes[5].set(TVec3f(0, 0, -1), TVec3f(0, 0, 10000));
        clipping->judgeClipping();
        require(actor.mFlag.mIsClipped,
                "a registered actor begins with the original 100m clipping distance");
        MR::setClippingFarMax(&actor);
        clipping->judgeClipping();
        require(!actor.mFlag.mIsClipped,
                "an explicit maximum clipping distance must use the current camera far plane");
        MR::setClippingFar100m(&actor);
        clipping->judgeClipping();
        require(actor.mFlag.mIsClipped, "the original clipping evaluator must consume the configured 100m level");
        actor.mPosition.z = 5000.0F;
        clipping->judgeClipping();
        require(!actor.mFlag.mIsClipped, "a configured actor must be restored when its sphere re-enters the frustum");
        ++passed;
    }

    {
        auto* holder = MR::getSceneObj<ShadowControllerHolder>(SceneObj_ShadowControllerHolder);
        require(holder != nullptr, "the original scene owns its shadow controller holder");
        const auto shadow_baseline = holder->_C.size();
        const auto queued_baseline = holder->_18.size();
        {
            ProbeActor actor;
            require(!MR::isExistShadow(&actor, nullptr),
                    "an actor without a controller list must not report a fabricated shadow");
            MR::initShadowVolumeSphere(&actor, 50.0F);

            require(actor.mShadowControllerList && actor.mShadowControllerList->getControllerCount() == 1U &&
                        actor.mShadowControllerList->mShadowList.mArray.size() == 1,
                    "sphere initialization must create one original controller in a one-slot list");
            auto* original = actor.mShadowControllerList->getController(0U);
            const auto* sphere = dynamic_cast<const ShadowVolumeSphere*>(original->getShadowDrawer());
            require(sphere && original->mName == smgpc::resource::encode_cp932("ボリューム影(球)"),
                    "sphere initialization must preserve the source controller name and shape kind");
            require_near(sphere->mRadius, 50.0F,
                         "sphere initialization must retain the authored shadow radius");
            require(original->mDropPos == &actor.mPosition && original->mDropDir == &actor.mGravity && original->_71,
                    "a new volume controller must follow the host transform and begin valid");
            require(MR::isExistShadow(&actor, nullptr) && MR::isExistShadow(&actor, "any-single-controller-name"),
                    "the exact single-controller lookup must succeed without requiring a name match");

            MR::onCalcShadowOneTime(&actor, nullptr);
            require(original->_60 == 2 && original->_65 == 0,
                    "one-time shadow calculation must update the owned controller");
            MR::onCalcShadow(&actor, nullptr);
            require(original->_60 == 1,
                    "continuous shadow calculation must replace one-time mode");
            MR::offCalcShadow(&actor, nullptr);
            require(original->_60 == 0,
                    "offCalcShadow must disable the owned controller");

            MR::onCalcShadowDropPrivateGravity(&actor, nullptr);
            require(original->_61 == 4,
                    "private-gravity calculation must be tracked per controller");
            MR::onCalcShadowDropPrivateGravityOneTime(&actor, nullptr);
            require(original->_61 == 5 && original->_66 == 0,
                    "one-time private gravity must replace continuous mode");
            MR::offCalcShadowDropPrivateGravity(&actor, nullptr);
            require(original->_61 == 3,
                    "private-gravity disable must remain controller-local");

            auto drop_position = TVec3f{4.0F, 5.0F, 6.0F};
            MR::setShadowDropPositionPtr(&actor, nullptr, &drop_position);
            MR::setShadowDropLength(&actor, nullptr, 250.0F);
            require(original->mDropPos == &drop_position && std::abs(original->mDropLength - 250.0F) < 0.0001F,
                    "shadow position and length setters must update the named controller state");
            MR::invalidateShadow(&actor, nullptr);
            require(!original->_71, "shadow invalidation must update the owned controller");
            MR::validateShadow(&actor, nullptr);
            require(original->_71, "shadow validation must update the owned controller");
        }
        {
            ProbeActor actor;
            MR::initShadowController(&actor, 2U);
            MR::addShadowSurfaceCircle(&actor, "first", 20.0F);
            MR::addShadowVolumeCylinder(&actor, "second", 30.0F);
            require(actor.mShadowControllerList->getController("first") == actor.mShadowControllerList->getController(0U) &&
                        actor.mShadowControllerList->getController("second") == actor.mShadowControllerList->getController(1U),
                    "multi-controller lookup must select the exact authored name");
            require(!MR::isExistShadow(&actor, "missing"),
                    "multi-controller lookup must reject a missing authored controller name");
        }
        require(holder->_C.size() == shadow_baseline && holder->_18.size() == queued_baseline,
                "LiveActor destruction must remove its original registered and queued shadow controllers");
        ++passed;
    }

    {
        ProbeActor actor;
        actor.initBinder(50.0F, 0.0F, 8U);
        auto center = TVec3f{};
        MR::setBinderExceptSensorType(&actor, &center, 10.0F);
        auto* clip_filter = dynamic_cast<ClipAreaCollisionFilter*>(actor.mBinder->mCollisionPartsFilter);
        require(clip_filter && clip_filter->_04 == &center && clip_filter->_08 == 10.0F,
                "Binder filtering must retain the original ClipArea filter and the caller's live center");
        MR::setBinderCollisionPartsFilter(&actor, nullptr);
        delete clip_filter;
        MR::initShadowVolumeSphere(&actor, 10.0F);
        MR::setClippingRangeIncludeShadow(&actor, &center, 100.0F);
        const auto* clipping = MR::getClippingDirector()->mActorHolder->find(&actor);
        require(clipping && clipping->mActor == &actor && clipping->_4 == &actor.mPosition && clipping->_8 == 100.0F &&
                    center.epsilonEquals(actor.mPosition, 0.0F),
                "an unprojected original shadow retains the actor's ordinary clipping sphere");

        require(!MR::isBindedGroundDamageFire(&actor),
                "an original Binder without ground contact cannot report a DamageFire floor");
        require(!MR::isPressedRoofAndGround(&actor),
                "an original Binder without opposing contacts cannot report crushing pressure");
        ++passed;
    }

    std::cout << "Game actor physics tests passed: " << passed << "/5\n";
    });
}
