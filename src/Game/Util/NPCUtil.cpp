#include "Game/Util/NPCUtil.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/NPCFunction.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/RailUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Util/TalkUtil.hpp"
#include <JSystem/JMath/JMATrigonometric.hpp>
#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>

namespace {
    static s32 sStarAppearSeStep = 103;
    static s32 sStarAppearSeStepCaretaker = 32;
    static s32 sStarAppearSeStepPenguinCoach = 95;
    static s32 sStarAppearSeStepTeresaRacer = 89;
    static s32 sStarAppearSeStepTrickRabbit = 22;

    inline bool isTrampleReactionTrigger(const NPCActor* pActor) {
        return !pActor->_DD && pActor->_E2;
    }

    inline bool isSpinReactionTrigger(const NPCActor* pActor) {
        return !pActor->_DE && pActor->_E3;
    }

    inline bool isPointingReactionTrigger(const NPCActor* pActor) {
        return !pActor->_DF && pActor->_E4;
    }

    inline bool isHitReactionTrigger(const NPCActor* pActor) {
        return !pActor->_E0 && pActor->_E5;
    }

    inline bool hasScaleController(const NPCActor* pActor) {
        return pActor->mScaleController != nullptr && pActor->mDelegator != nullptr;
    }
};  // namespace

namespace MR {
    bool getNPCItemData(NPCActorItem* pItem, s32 itemType) {
        return NPCFunction::getNPCItemData(pItem, itemType);
    }

    void decidePose(NPCActor* pActor, const TVec3f& rUp, const TVec3f& rFront, const TVec3f& rPosition, f32 upRate, f32 frontRate, f32 positionRate) {
        blendVec(&pActor->mPosition, pActor->mPosition, rPosition, positionRate);

        if (upRate == 1.0f && frontRate == 1.0f) {
            makeQuatUpFront(&pActor->_A0, rUp, rFront);
        } else {
            blendQuatUpFront(&pActor->_A0, rUp, rFront, upRate, frontRate);
        }
    }

    void setNPCActorPos(NPCActor* pActor, const char* pName) {
        TPos3f mtx;
        mtx.identity();
        findNamePos(pName, mtx.toMtxPtr());
        pActor->setBaseMtx(mtx);
        mtx.getTrans(pActor->mPosition);
        resetPosition(pActor);
        onCalcShadowOneTimeAll(pActor);
    }

    void followRailPose(NPCActor* pActor, f32 frontRate, f32 positionRate) {
        const TVec3f& position = getRailPos(pActor);
        const TVec3f& front = getRailDirection(pActor);
        decidePose(pActor, -pActor->mGravity, front, position, 1.0f, frontRate, positionRate);
    }

    void followRailPoseOnGround(NPCActor* pActor, f32 frontRate) {
        followRailPoseOnGround(pActor, pActor, frontRate);
    }

#pragma dont_inline on
    void followRailPoseOnGround(NPCActor* pActor, const LiveActor* pRailActor, f32 frontRate) {
        TVec3f position(getRailPos(pRailActor));
        TVec3f gravity(pActor->mGravity);
        TVec3f ray(gravity);
        ray.scale(1000.0f);
        TVec3f offset(gravity);
        offset.scale(10.0f);
        getFirstPolyOnLineToMap(&position, nullptr, getRailPos(pRailActor) - offset, ray);
        const TVec3f& front = getRailDirection(pRailActor);
        decidePose(pActor, -gravity, front, position, 1.0f, frontRate, 1.0f);
    }
#pragma dont_inline reset

    void startNPCTalkCamera(const TalkMessageCtrl* pTalkCtrl, MtxPtr pActorMtx, f32 scale, s32 frame) {
        startNPCTalkCamera(pTalkCtrl, pActorMtx, getPlayerBaseMtx(), scale, frame);
    }

    void startNPCTalkCamera(const TalkMessageCtrl* pTalkCtrl, MtxPtr pActorMtx, MtxPtr pPlayerMtx, f32 scale, s32 frame) {
        TVec3f offset(getMessageBalloonFollowOffset(pTalkCtrl));
        if (getMessageBalloonFollowMatrix(pTalkCtrl) != nullptr) {
            pActorMtx = getMessageBalloonFollowMatrix(pTalkCtrl);
        }

        TVec3f up(pPlayerMtx[0][1], pPlayerMtx[1][1], pPlayerMtx[2][1]);
        TVec3f position(pActorMtx[0][3], pActorMtx[1][3], pActorMtx[2][3]);
        TVec3f playerPosition(pPlayerMtx[0][3], pPlayerMtx[1][3], pPlayerMtx[2][3]);
        if (normalizeOrZero(&up)) {
            up.set(0.0f, 1.0f, 0.0f);
        }

        f32 distance = PSVECDistance(&playerPosition, &position);
        f32 angle = JMAATan2(1.0f, JMACosDegree(67.5f));
        TVec3f horizontal;
        f32 height = vecKillElement(position - playerPosition, up, &horizontal);
        f32 distanceRate = max(((height + offset.y) / distance) / 0.75f, 1.0f);
        f32 axisX = height / 6.0f + offset.y;
        f32 axisY = max(2.0f * (distance * angle * distanceRate) * scale, 450.0f);
        startTalkCamera(position, up, axisX, axisY, frame);
    }

    void endNPCTalkCamera(bool isForce, s32 frame) {
        endTalkCamera(isForce, frame);
    }

    bool isActionLoopedOrStopped(const LiveActor* pActor) {
        if (getBckCtrl(pActor)->getAttribute() == 0) {
            return isBckStopped(pActor);
        }

        return isBckLooped(pActor);
    }

    void startMoveAction(NPCActor* pActor) {
        if (isExistRail(pActor)) {
            adjustmentRailCoordSpeed(pActor, pActor->_10C, pActor->_110);
            moveRailRider(pActor);

            if (pActor->_124) {
                followRailPoseOnGround(pActor, pActor, pActor->_114);
            } else {
                followRailPose(pActor, pActor->_114, pActor->_114);
            }

            if (isRailReachedGoal(pActor)) {
                reverseRailDirection(pActor);
            }
        }
    }

    bool tryStartTalkAction(NPCActor* pActor) {
        const char* pActionName;

        if (isTalkTalking(pActor->mMsgCtrl)) {
            if (pActor->mParam._1 && !pActor->turnToPlayer(pActor->mParam._8, pActor->mParam._C, pActor->mParam._10)) {
                pActionName = pActor->mParam._20;
            } else {
                pActionName = pActor->mParam._1C;
            }
        } else {
            return tryStartTurnAction(pActor);
        }

        if (!isNullOrEmptyString(pActionName)) {
            return tryStartAction(pActor, pActionName);
        }

        return false;
    }

    bool tryStartMoveTalkAction(NPCActor* pActor) {
        TalkMessageCtrl* pTalkCtrl = pActor->mMsgCtrl;
        if (!isExistRail(pActor)) {
            return tryStartTalkAction(pActor);
        }

        bool isMoveTalk = false;
        const char* pActionName;
        if (isTalkTalking(pTalkCtrl) && !isShortTalk(pTalkCtrl)) {
            if (pActor->mParam._1 && !pActor->turnToPlayer(pActor->mParam._8, pActor->mParam._C, pActor->mParam._10)) {
                pActionName = pActor->mParam._20;
            } else {
                pActionName = pActor->mParam._1C;
            }
        } else {
            if (isNearZero(pActor->_10C) && isNearZero(getRailCoordSpeed(pActor))) {
                return tryStartTalkAction(pActor);
            }

            startMoveAction(pActor);

            if (isTalkTalking(pTalkCtrl)) {
                pActionName = pActor->_120;
                isMoveTalk = true;
            } else {
                pActionName = pActor->_11C;
            }
        }

        if (!isNullOrEmptyString(pActionName)) {
            bool started = tryStartAction(pActor, pActionName);
            if (isMoveTalk) {
                setBckRate(pActor, pActor->_118);
            } else {
                setBckRate(pActor, 1.0f);
            }
            return started;
        }

        return false;
    }

    bool tryStartTurnAction(NPCActor* pActor) {
        const char* pActionName;

        if (isNearPlayer(pActor, pActor->mParam._4)) {
            if (pActor->mParam._0) {
                if (pActor->turnToPlayer(pActor->mParam._8, pActor->mParam._C, pActor->mParam._10)) {
                    pActionName = pActor->mParam._14;
                } else {
                    pActionName = pActor->mParam._18;
                }
            } else {
                pActionName = pActor->mParam._14;
            }
        } else if (pActor->mParam._0 || pActor->mParam._1) {
            if (pActor->turnToDefault(pActor->mParam._8)) {
                pActionName = pActor->mParam._14;
            } else {
                pActionName = pActor->mParam._18;
            }
        } else {
            pActionName = pActor->mParam._14;
        }

        if (isNullOrEmptyString(pActionName)) {
            return false;
        }

        return tryStartAction(pActor, pActionName);
    }

    bool tryStartMoveTurnAction(NPCActor* pActor) {
        if (!isExistRail(pActor)) {
            return tryStartTurnAction(pActor);
        }

        startMoveAction(pActor);
        if (isNullOrEmptyString(pActor->_11C)) {
            return false;
        }

        return tryStartAction(pActor, pActor->_11C);
    }

    bool tryStartReaction(NPCActor* pActor) {
        const char* pActionName = nullptr;
        bool started = false;

        if (pActor->_128) {
            if (isTrampleReactionTrigger(pActor)) {
                pActionName = pActor->_134;
            } else if (isHitReactionTrigger(pActor)) {
                pActionName = pActor->_13C;
            } else if (isSpinReactionTrigger(pActor)) {
                pActionName = pActor->_130;
            } else if (pActor->_E4) {
                pActionName = pActor->_138;
            }
        }

        if (!isNullOrEmptyString(pActionName)) {
            if (isTrampleReactionTrigger(pActor) || isHitReactionTrigger(pActor)) {
                stopBck(pActor);
                startAction(pActor, pActionName);
                started = true;
            } else if (isSpinReactionTrigger(pActor)) {
                started = tryStartAction(pActor, pActionName);
            } else if (pActor->_E4) {
                if ((pActor->_134 != nullptr && isActionStart(pActor, pActor->_134)) ||
                    (pActor->_13C != nullptr && isActionStart(pActor, pActor->_13C)) ||
                    (pActor->_130 != nullptr && isActionStart(pActor, pActor->_130))) {
                    return started;
                }

                if (isPointingReactionTrigger(pActor)) {
                    started = tryStartAction(pActor, pActionName);
                } else if (isActionLoopedOrStopped(pActor)) {
                    startAction(pActor, pActionName);
                }
            }
        } else if (hasScaleController(pActor)) {
            if (isPointingReactionTrigger(pActor) || isSpinReactionTrigger(pActor) || isTrampleReactionTrigger(pActor) ||
                isHitReactionTrigger(pActor)) {
                started = true;
            }
        }

        return started;
    }

    bool tryTalkNearPlayerAndStartTalkAction(NPCActor* pActor) {
        tryStartTalkAction(pActor);
        return tryTalkNearPlayer(pActor->mMsgCtrl);
    }

    bool tryTalkNearPlayerAtEndAndStartTalkAction(NPCActor* pActor) {
        tryStartTalkAction(pActor);
        return tryTalkNearPlayerAtEnd(pActor->mMsgCtrl);
    }

    bool tryTalkNearPlayerAtEndAndStartMoveTalkAction(NPCActor* pActor) {
        tryStartMoveTalkAction(pActor);
        return tryTalkNearPlayerAtEnd(pActor->mMsgCtrl);
    }

    bool tryTalkForceAndStartMoveTalkAction(NPCActor* pActor) {
        tryStartMoveTalkAction(pActor);
        return tryTalkForce(pActor->mMsgCtrl);
    }

    bool tryStartReactionAndPushNerve(NPCActor* pActor, const Nerve* pNerve) {
        if (tryStartReaction(pActor)) {
            pActor->pushNerve(pNerve);
            return true;
        }

        return false;
    }

    bool tryStartReactionAndPopNerve(NPCActor* pActor) {
        if (tryStartReaction(pActor)) {
            pActor->pushNerve(pActor->popNerve());
            return false;
        }

        if (pActor->isScaleAnim()) {
            return false;
        }

        if (isActionLoopedOrStopped(pActor)) {
            pActor->popNerve();
            return true;
        }

        return false;
    }
};  // namespace MR

namespace NrvTakeOutStar {
    NEW_NERVE(TakeOutStarNrvAnim, TakeOutStar, Anim);
    NEW_NERVE(TakeOutStarNrvDemo, TakeOutStar, Demo);
    NEW_NERVE(TakeOutStarNrvTerm, TakeOutStar, Term);
};  // namespace NrvTakeOutStar

namespace NrvFadeStarter {
    NEW_NERVE(FadeStarterNrvFade, FadeStarter, Fade);
    NEW_NERVE(FadeStarterNrvTerm, FadeStarter, Term);
};  // namespace NrvFadeStarter

namespace NrvDemoStarter {
    NEW_NERVE(DemoStarterNrvInit, DemoStarter, Init);
    NEW_NERVE(DemoStarterNrvFade, DemoStarter, Fade);
    NEW_NERVE(DemoStarterNrvWait, DemoStarter, Wait);
    NEW_NERVE(DemoStarterNrvTerm, DemoStarter, Term);
};  // namespace NrvDemoStarter

TakeOutStar::TakeOutStar(NPCActor* pActor, const char* pActionName, const char* pAnimName, const Nerve* pNerve)
    : NerveExecutor("パワースター取り出しデモ実行者"), mActor(pActor), mNerve(pNerve), mActionName(pActionName), mAnimName(pAnimName) {
    mStarModel = MR::createPowerStarDemoModel(mActor, "パワースターデモモデル", pActor->getBaseMtx());
    mStarModel->makeActorDead();

    initNerve(&NrvTakeOutStar::TakeOutStarNrvAnim::sInstance);
}

bool TakeOutStar::takeOut() {
    if (isNerve(&NrvTakeOutStar::TakeOutStarNrvTerm::sInstance)) {
        return true;
    }

    updateNerve();

    return false;
}

bool TakeOutStar::isFirstStep() {
    return isNerve(&NrvTakeOutStar::TakeOutStarNrvAnim::sInstance) && MR::isFirstStep(this);
}

bool TakeOutStar::isLastStep() {
    return isNerve(&NrvTakeOutStar::TakeOutStarNrvTerm::sInstance);
}

void TakeOutStar::exeAnim() {
    if (MR::isFirstStep(this)) {
        if (mNerve != nullptr) {
            mActor->pushNerve(mNerve);
        } else {
            mActor->tryPushNullNerve();
        }

        mStarModel->appear();
        MR::invalidateClipping(mStarModel);
        MR::requestMovementOn(mStarModel);
        MR::startBck(mStarModel, mAnimName, nullptr);
        MR::startAction(mActor, mActionName);
    }

    s32 step = ::sStarAppearSeStep;

    if (MR::isEqualString(mAnimName, "TakeOutStarCaretaker")) {
        step = ::sStarAppearSeStepCaretaker;
    } else if (MR::isEqualString(mAnimName, "TakeOutStarTeresaRacer")) {
        step = ::sStarAppearSeStepTeresaRacer;
    } else if (MR::isEqualString(mAnimName, "TakeOutStarPenguinCoach")) {
        step = ::sStarAppearSeStepPenguinCoach;
    } else if (MR::isEqualString(mAnimName, "TakeOutStarTrickRabbit")) {
        step = ::sStarAppearSeStepTrickRabbit;
    }

    if (MR::isGreaterStep(this, step)) {
        if (MR::isInWater(mStarModel, TVec3f(0.0f, 0.0f, 0.0f))) {
            MR::startLevelSound(mStarModel, "SE_OJ_LV_POW_STAR_EXIST_W");
        } else {
            MR::startLevelSound(mStarModel, "SE_OJ_LV_POW_STAR_EXIST");
        }
    }

    if (MR::isAnyAnimOneTimeAndStopped(mActor, mActionName)) {
        setNerve(&NrvTakeOutStar::TakeOutStarNrvDemo::sInstance);
    }
}

void TakeOutStar::exeDemo() {
    if (MR::isFirstStep(this)) {
        TVec3f trans;
        MR::extractMtxTrans(MR::getJointMtx(mStarModel, "PowerStar"), &trans);
        MR::appearPowerStarContinueCurrentDemo(mActor, trans);
        mStarModel->kill();
    }

    if (MR::isEndPowerStarAppearDemo(mActor)) {
        MR::validateClipping(mStarModel);
        mActor->popNerve();
        setNerve(&NrvTakeOutStar::TakeOutStarNrvTerm::sInstance);
    }
}

void TakeOutStar::exeTerm() {
}

FadeStarter::FadeStarter(NPCActor* pActor, s32 a2) : NerveExecutor("フェード開始制御"), mActor(pActor), _C(nullptr), _10(a2) {
    initNerve(&NrvFadeStarter::FadeStarterNrvFade::sInstance);
}

bool FadeStarter::update() {
    if (isNerve(&NrvFadeStarter::FadeStarterNrvTerm::sInstance)) {
        return true;
    }

    updateNerve();

    return false;
}

void FadeStarter::exeFade() {
    if (MR::isFirstStep(this)) {
        if (!mActor->isEmptyNerve()) {
            _C = mActor->popNerve();
        }

        mActor->tryPushNullNerve();
        MR::closeWipeFade(_10);
    }

    if (MR::isWipeActive()) {
        return;
    }

    mActor->popNerve();

    if (_C != nullptr) {
        mActor->pushNerve(_C);
        _C = nullptr;
    }

    MR::openWipeFade(_10);
    setNerve(&NrvFadeStarter::FadeStarterNrvTerm::sInstance);
}

void FadeStarter::exeTerm() {
}

DemoStarter::DemoStarter(NPCActor* pActor) : NerveExecutor("デモ開始制御"), mActor(pActor) {
    initNerve(&NrvDemoStarter::DemoStarterNrvInit::sInstance);
}

bool DemoStarter::update() {
    updateNerve();

    return isNerve(&NrvDemoStarter::DemoStarterNrvTerm::sInstance);
}

void DemoStarter::start() {
    if (isNerve(&NrvDemoStarter::DemoStarterNrvInit::sInstance)) {
        setNerve(&NrvDemoStarter::DemoStarterNrvFade::sInstance);
    }
}

void DemoStarter::exeInit() {
}

void DemoStarter::exeFade() {
    if (MR::isFirstStep(this)) {
        MR::invalidateClipping(mActor);
        MR::offPlayerControl();
        MR::closeWipeFade();
    }

    if (MR::isWipeActive()) {
        return;
    }

    setNerve(&NrvDemoStarter::DemoStarterNrvWait::sInstance);
}

void DemoStarter::exeWait() {
    if (MR::isLessStep(this, 30)) {
        return;
    }

    if (MR::canStartDemo()) {
        setNerve(&NrvDemoStarter::DemoStarterNrvTerm::sInstance);
    }
}

void DemoStarter::exeTerm() {
}
