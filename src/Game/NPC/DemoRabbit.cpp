#include "Game/NPC/DemoRabbit.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/Util.hpp"

namespace MR {
    void timeKeepDemoFadeOut();
    void timeKeepDemoFadeIn();
};  // namespace MR

namespace NrvDemoRabbit {
    NEW_NERVE(DemoRabbitNrvAppear, DemoRabbit, Appear);
    NEW_NERVE(DemoRabbitNrvDemo, DemoRabbit, Demo);
    NEW_NERVE(DemoRabbitNrvTalk0, DemoRabbit, Talk);
    NEW_NERVE(DemoRabbitNrvTalk1, DemoRabbit, Talk);
    NEW_NERVE(DemoRabbitNrvWait, DemoRabbit, Wait);
    NEW_NERVE(DemoRabbitNrvGuide, DemoRabbit, Guide);
    NEW_NERVE(DemoRabbitNrvGoal, DemoRabbit, Goal);
    NEW_NERVE(DemoRabbitNrvRunaway, DemoRabbit, Runaway);
    NEW_NERVE(DemoRabbitNrvChange, DemoRabbit, Change);
    NEW_NERVE(DemoRabbitNrvStartBGM, DemoRabbit, StartBGM);
};  // namespace NrvDemoRabbit

DemoRabbit::DemoRabbit(const char* pName) : NPCActor(pName) {
}
DemoRabbit::~DemoRabbit() {
}

void DemoRabbit::makeArchiveList(NameObjArchiveListCollector* pArchiveList, const JMapInfoIter& rIter) {
    if (MR::getDemoCastID(rIter) == 0) {
        pArchiveList->addArchive("TrickRabbitBaby");
    } else {
        pArchiveList->addArchive("TrickRabbit");
    }
}

void DemoRabbit::init(const JMapInfoIter& rIter) {
    NPCActorCaps caps("DemoRabbit");
    caps.setDefault();
    caps.mObjectName = "TrickRabbit";
    caps.mRailRider = true;
    caps.mMessage = true;
    caps.mMakeActor = false;
    caps.mWaitNerve = &NrvDemoRabbit::DemoRabbitNrvAppear::sInstance;

    if (MR::tryRegisterDemoCast(this, rIter)) {
        if (MR::getDemoCastID(rIter) == 0) {
            caps.mObjectName = "TrickRabbitBaby";
            caps.mWaitNerve = &NrvDemoRabbit::DemoRabbitNrvAppear::sInstance;

            MR::invalidateClipping(this);
            MR::registerDemoActionNerve(this, &NrvDemoRabbit::DemoRabbitNrvTalk0::sInstance, "チコとの出会い[ウサギ会話]");
            MR::registerDemoActionNerve(this, &NrvDemoRabbit::DemoRabbitNrvGuide::sInstance, "チコとの出会い[ウサギ逃走]");
            MR::registerDemoActionFunctor(this, MR::Functor(this, &DemoRabbit::fadeOut), "ウサギ追いかけ[フェードアテト]");
            MR::registerDemoActionFunctor(this, MR::Functor(this, &DemoRabbit::fadeIn), "ウサギ追いかけ[フェードイン]");
            MR::registerDemoActionNerve(this, &NrvDemoRabbit::DemoRabbitNrvTalk1::sInstance, "ウサギ追いかけ[会話]");
            MR::registerDemoActionNerve(this, &NrvDemoRabbit::DemoRabbitNrvRunaway::sInstance, "ウサギ追いかけ[逃走]");
        } else {
            MR::registerDemoActionNerve(this, &NrvDemoRabbit::DemoRabbitNrvRunaway::sInstance, "ウサギ追いかけ[逃走]");
            caps.mWaitNerve = &NrvDemoRabbit::DemoRabbitNrvDemo::sInstance;
        }
    }

    initialize(rIter, caps);

    if (mMsgCtrl != nullptr) {
        MR::setDistanceToTalk(mMsgCtrl, 1300.0f);
        MR::offRootNodeAutomatic(mMsgCtrl);
    }

    MR::onCalcShadow(this, nullptr);
    MR::onCalcGravity(this);

    mFrontVec.set< f32 >((2.0f * (_A0.x * _A0.z)) + (2.0f * (_A0.w * _A0.y)), (2.0f * (_A0.y * _A0.z)) - (2.0f * (_A0.w * _A0.x)),
                         (1.0f - (2.0f * (_A0.x * _A0.x))) - (2.0f * (_A0.y * _A0.y)));

    if (isNerve(&NrvDemoRabbit::DemoRabbitNrvAppear::sInstance)) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

void DemoRabbit::initAfterPlacement() {
    if (isNerve(&NrvDemoRabbit::DemoRabbitNrvAppear::sInstance) && MR::isOnGameEventFlagEndTicoGuideDemo()) {
        MR::forwardNode(mMsgCtrl);
        makeActorAppeared();
        setNerve(&NrvDemoRabbit::DemoRabbitNrvStartBGM::sInstance);
    }
}

void DemoRabbit::control() {
    MR::blendQuatUpFront(&_A0, -mGravity, mFrontVec, 0.1f, 0.2f);

    if (MR::isBindedGround(this)) {
        mNoGroundTimer = 0;
    } else {
        mNoGroundTimer++;
    }
}

void DemoRabbit::fadeOut() {
    MR::timeKeepDemoFadeOut();
}

void DemoRabbit::fadeIn() {
    TVec3f railEndPos;
    MR::calcRailEndPos(&railEndPos, this);

    TVec3f gravity;
    MR::calcGravityVector(this, railEndPos, &gravity, nullptr, 0);

    TVec3f startOffset(gravity);
    startOffset.scale(100.0f);

    TVec3f startPos(railEndPos);
    startPos.sub(startOffset);

    TVec3f endOffset(gravity);
    endOffset.scale(1000.0f);

    MR::getFirstPolyOnLineToMap(&railEndPos, nullptr, startPos, endOffset);
    mPosition.set< f32 >(railEndPos);
    MR::timeKeepDemoFadeIn();
}

void DemoRabbit::updateStopVelocity() {
    if (mNoGroundTimer < 5) {
        MR::attenuateVelocity(this, 0.9f);
    } else {
        MR::attenuateVelocity(this, 0.99f);
    }

    MR::addVelocityToGravityOrGround(this, 0.1f);
    MR::reboundVelocityFromCollision(this, 1.0f, 1.0f, 0.0f);

    TVec3f gravityOffset(mGravity);
    gravityOffset.scale(10.0f);

    TVec3f hitNormal;
    MR::getFirstPolyNormalOnLineToMap(&hitNormal, mPosition, gravityOffset, nullptr, nullptr);
    MR::vecKillElement(mVelocity, hitNormal, &hitNormal);
    mVelocity.sub(hitNormal);
}

void DemoRabbit::updateNormalVelocity() {
    MR::attenuateVelocity(this, 0.9f);

    if (MR::isBindedWall(this)) {
        MR::addVelocityToGravityOrGround(this, 0.1f);
    } else if (MR::isBindedGround(this)) {
        MR::addVelocityToGravityOrGround(this, 0.1f);
    } else {
        MR::addVelocityToGravityOrGround(this, 1.0f);
    }

    MR::reboundVelocityFromCollision(this, -0.05f, 0.0f, 1.0f);
}

void DemoRabbit::updateRun(const TVec3f& rTargetPos, bool) {
    f32 turnDegree = MR::calcNerveValue(this, 30, 10.0f, 1.5f);
    TVec3f targetDir(rTargetPos);
    targetDir.sub(mPosition);
    MR::turnDirectionDegree(this, &mFrontVec, targetDir, turnDegree);

    f32 speed = MR::calcNerveValue(this, 900, 1.55f, 1.2f);

    while (MR::isExistMapCollision(mPosition, mFrontVec.scaleInline(200.0f))) {
        f32 frontLength = mFrontVec.length();
        TVec3f newFront(mFrontVec + -mGravity);
        newFront.scale(0.5f);
        mFrontVec.set< f32 >(newFront);
        mFrontVec.setLength(frontLength);
    }

    if (mNoGroundTimer < 5) {
        MR::addVelocityMoveToDirection(this, mFrontVec, speed);
    } else {
        MR::addVelocityMoveToDirection(this, mFrontVec, speed * 0.5f);
    }
}

void DemoRabbit::updateJump() {
    if (MR::isOnGround(this)) {
        const TVec3f* groundNormal = MR::getGroundNormal(this);
        f32 gravitySpeed = mVelocity.dot(*groundNormal);
        TVec3f normalVelocity(*groundNormal);
        normalVelocity.scale(gravitySpeed);
        mVelocity.sub(normalVelocity);
    }

    if (MR::isBindedWall(this) && MR::calcHitPowerToWall(this) <= 0.0f) {
        MR::addVelocityJump(this, 30.0f);
    }
}

bool DemoRabbit::tryGuide() {
    if (MR::isNearPlayer(this, 800.0f)) {
        MR::invalidateClipping(this);
        setNerve(&NrvDemoRabbit::DemoRabbitNrvGuide::sInstance);
        return true;
    }

    return false;
}

bool DemoRabbit::tryWait() {
    if (!MR::isNearPlayer(this, 1100.0f)) {
        MR::validateClipping(this);
        setNerve(&NrvDemoRabbit::DemoRabbitNrvWait::sInstance);
        return true;
    }

    return false;
}

bool DemoRabbit::tryGoal() {
    if (MR::isOnGround(this) && MR::isRailReachedNearGoal(this, 100.0f)) {
        MR::validateClipping(this);
        setNerve(&NrvDemoRabbit::DemoRabbitNrvGoal::sInstance);
        return true;
    }

    return false;
}

void DemoRabbit::exeAppear() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Appear");
        MR::addVelocityJump(this, 20.0f);
        MR::calcVecToPlayerH(&mFrontVec, this, &mGravity);
        MR::startSound(this, "SE_SM_DEMORABBIT_APPEAR", -1, -1);
        MR::startSound(this, "SE_SM_DEMORABBIT_SMOKE", -1, -1);
    }

    MR::addVelocityToGravity(this, 1.0f);
    MR::attenuateVelocity(this, 0.99f);

    if (MR::isGreaterStep(this, 5) && MR::isBindedGround(this)) {
        mVelocity.set< f32 >(0.0f, 0.0f, 0.0f);
        setNerve(&NrvDemoRabbit::DemoRabbitNrvDemo::sInstance);
    }
}

void DemoRabbit::exeDemo() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Wait");
    }

    MR::turnDirectionToPlayerDegree(this, &mFrontVec, 10.0f);
    updateStopVelocity();
}

void DemoRabbit::exeTalk() {
    if (MR::isFirstStep(this)) {
        if (isNerve(&NrvDemoRabbit::DemoRabbitNrvTalk1::sInstance)) {
            MR::forwardNode(mMsgCtrl);
        }

        MR::tryTalkTimeKeepDemoMarioPuppetable(mMsgCtrl);

        if (isNerve(&NrvDemoRabbit::DemoRabbitNrvTalk0::sInstance)) {
            MR::forwardNode(mMsgCtrl);
        }
    }

    MR::turnDirectionToPlayerDegree(this, &mFrontVec, 10.0f);
    updateStopVelocity();
}

void DemoRabbit::exeWait() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Wait");
    }

    MR::tryTalkNearPlayer(mMsgCtrl);
    MR::turnDirectionToPlayerDegree(this, &mFrontVec, 10.0f);
    updateStopVelocity();
    tryGuide();
}

void DemoRabbit::exeGoal() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Change");
    }

    MR::turnDirectionToPlayerDegree(this, &mFrontVec, 10.0f);
    updateStopVelocity();

    if (!MR::isDemoActive()) {
        if (MR::isNearPlayer(mMsgCtrl, 500.0f)) {
            MR::startTimeKeepDemoMarioPuppetable(this, "チコガイドデモ", "ウサギ追いかけ[フェードアテト]");
        } else {
            MR::tryTalkNearPlayer(mMsgCtrl);
        }
    }
}

void DemoRabbit::exeGuide() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Run");
        MR::startSound(this, "SE_SM_RABBIT_JUMP", -1, -1);
    }

    if (MR::checkPassBckFrame(this, 3.0f)) {
        MR::startSound(this, "SE_SM_RABBIT_JUMP", -1, -1);
    }

    TVec3f railFrontPos;
    MR::calcRailPosFrontCoord(&railFrontPos, this, 200.0f);

    if (MR::isRailReachedNearGoal(this, 130.0f)) {
        TVec3f targetOffset(mGravity);
        targetOffset.scale(2.0f);

        TVec3f targetPos(railFrontPos);
        targetPos.sub(targetOffset);

        updateRun(targetPos, true);
    } else {
        TVec3f targetOffset(mGravity);
        targetOffset.scale(2.0f);

        TVec3f targetPos(railFrontPos);
        targetPos.sub(targetOffset);

        updateRun(targetPos, false);
    }
    updateNormalVelocity();
    updateJump();
    MR::moveCoordToNearestPos(this, mPosition);

    if (!tryGoal()) {
        tryWait();
    }
}

void DemoRabbit::exeRunaway() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Run");
        MR::startSound(this, "SE_SM_RABBIT_JUMP", -1, -1);
    }

    MR::invalidateClipping(this);

    if (MR::checkPassBckFrame(this, 3.0f)) {
        MR::startSound(this, "SE_SM_RABBIT_JUMP", -1, -1);
    }

    TVec3f targetPos(mPosition + mPosition);
    targetPos.sub(*MR::getPlayerPos());

    updateRun(targetPos, false);
    updateNormalVelocity();
    updateJump();

    if (MR::isGreaterEqualStep(this, 120)) {
        setNerve(&NrvDemoRabbit::DemoRabbitNrvChange::sInstance);
    }
}

void DemoRabbit::exeChange() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Change");
        MR::startSound(this, "SE_SM_RABBIT_HIDE", -1, -1);
    }

    updateNormalVelocity();

    if (MR::isBckStopped(this)) {
        MR::startSound(this, "SE_SM_METAMORPHOSE_SMOKE", -1, -1);
        kill();
    }
}

void DemoRabbit::exeStartBGM() {
    if (MR::isFirstStep(this) && !MR::isPlayingStageBgm()) {
        MR::startStageBGM("MBGM_GALAXY_24", false);
    }

    setNerve(&NrvDemoRabbit::DemoRabbitNrvGuide::sInstance);
}
