#include "Game/NPC/Rabbit.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Util.hpp"

namespace {
    static f32 cJumpSpeed = 20.0f;
    static f32 cProgressSpeed = 18.0f;
    static f32 cGravity = 3.0f;
    static f32 cFinalDisappearRadius = 300.0f;
    static f32 cDistEscape = 400.0f;
    static f32 cDistWait = 500.0f;
    static f32 cDistNear = 600.0f;
    static f32 cAppearHeight = 150.0f;
    static f32 cAppearHop = 10.0f;
    static f32 cRegistNormal = 0.98f;

    enum RabbitType {
        RABBIT_TYPE_NORMAL = 0,
        RABBIT_TYPE_TALK = 1,
        RABBIT_TYPE_DEMO_FIRST = 2,
        RABBIT_TYPE_DEMO = 3,
        RABBIT_TYPE_JUMP_V = 4,
        RABBIT_TYPE_JUMP_H = 5
    };
};  // namespace

namespace NrvRabbit {
    NEW_NERVE(RabbitNrvAppear, Rabbit, Appear);
    NEW_NERVE(RabbitNrvAppearLand, Rabbit, AppearLand);
    NEW_NERVE(RabbitNrvWait, Rabbit, Wait);
    NEW_NERVE(RabbitNrvForwardLand, Rabbit, ForwardLand);
    NEW_NERVE(RabbitNrvPreJump, Rabbit, PreJump);
    NEW_NERVE(RabbitNrvMove, Rabbit, Move);
    NEW_NERVE(RabbitNrvGoal, Rabbit, Goal);
    NEW_NERVE(RabbitNrvFinish, Rabbit, Finish);
    NEW_NERVE(RabbitNrvReaction, Rabbit, Reaction);
    NEW_NERVE(RabbitNrvTalk, Rabbit, Talk);
    NEW_NERVE(RabbitNrvJumpV, Rabbit, JumpV);
    NEW_NERVE(RabbitNrvJumpH, Rabbit, JumpH);
    NEW_NERVE(RabbitNrvBackwardLand, Rabbit, BackwardLand);
    NEW_NERVE(RabbitNrvPreJumpBack, Rabbit, PreJumpBack);
    NEW_NERVE(RabbitNrvNear, Rabbit, Near);
};  // namespace NrvRabbit

Rabbit::Rabbit(const char* pName) : NPCActor(pName) {}

void Rabbit::init(const JMapInfoIter& rIter) {
    const char* pObjectName = nullptr;
    MR::getObjectName(&pObjectName, rIter);
    MR::initDefaultPose(this, rIter);

    initModelManagerWithAnm("MoonRabbit", nullptr, false);
    MR::connectToSceneNpc(this);
    MR::initLightCtrl(this);
    initSound(4, false);
    initBinder(10.0f, 10.0f, 0);
    MR::onCalcGravity(this);
    initHitSensor(1);

    TVec3f sensorOffset(0.0f, 0.0f, 0.0f);
    MtxPtr spineMtx = MR::getJointMtx(this, "Spine");
    MR::addHitSensorMtxNpc(this, "body", 8, 60.0f, spineMtx, sensorOffset);
    MR::initStarPointerTargetAtJoint(this, "Spine", 60.0f, TVec3f(0.0f, 60.0f, 0.0f));
    initEffectKeeper(0, nullptr, false);
    MR::initShadowFromCSV(this, "Shadow");

    mLodCtrl = MR::createLodCtrlNPC(this, rIter);
    MR::useStageSwitchWriteDead(this, rIter);

    s32 btkFrame = 0;
    MR::getJMapInfoArg0NoInit(rIter, &btkFrame);
    MR::startBtk(this, "MoonRabbit");
    MR::setBtkFrameAndStop(this, static_cast< f32 >(btkFrame));

    s32 messageId;
    if (MR::getJMapInfoMessageID(rIter, &messageId)) {
        mMsgCtrl = MR::createTalkCtrl(this, rIter, pObjectName, TVec3f(0.0f, 160.0f, 0.0f), nullptr);
        MR::onRootNodeAutomatic(mMsgCtrl);
        MR::useStageSwitchReadA(this, rIter);
        MR::useStageSwitchReadB(this, rIter);
    }

    s32 rabbitType = 0;
    if (MR::tryRegisterDemoCast(this, rIter)) {
        if (MR::getDemoCastID(rIter) == 0) {
            mRabbitType = RABBIT_TYPE_DEMO_FIRST;
        }
        else {
            mRabbitType = RABBIT_TYPE_DEMO;
        }
    }
    else {
        MR::getJMapInfoArg1NoInit(rIter, &rabbitType);

        switch (rabbitType) {
        case 0:
            if (rIter.isValid()) {
                initRailRider(rIter);
                MR::moveCoordAndTransToNearestRailPos(this);
            }

            mRabbitType = RABBIT_TYPE_NORMAL;
            initNerve(&NrvRabbit::RabbitNrvWait::sInstance);
            break;
        case 1:
            mRabbitType = RABBIT_TYPE_TALK;
            initNerve(&NrvRabbit::RabbitNrvTalk::sInstance);
            break;
        case 2:
            mRabbitType = RABBIT_TYPE_JUMP_V;
            initNerve(&NrvRabbit::RabbitNrvTalk::sInstance);
            break;
        case 3:
            mRabbitType = RABBIT_TYPE_JUMP_H;
            initNerve(&NrvRabbit::RabbitNrvTalk::sInstance);
            break;
        }
    }

    mJumpOffset = 0.0f;
    mJumpVelocity = 0.0f;
    mRailNormalFactor = 1.0f;
    mOnGround = true;
    mJumpLanding = false;
    mWaitTimer = 0;

    if (mRabbitType == RABBIT_TYPE_DEMO_FIRST || mRabbitType == RABBIT_TYPE_DEMO) {
        makeActorDead();
    }
    else if (MR::useStageSwitchReadAppear(this, rIter)) {
        MR::syncStageSwitchAppear(this);
        makeActorDead();
        pushNerve(&NrvRabbit::RabbitNrvAppear::sInstance);
    }
    else {
        makeActorAppeared();
        MR::startBck(this, "Wait2", nullptr);
        MR::emitEffect(this, "Light");
    }

    MR::setClippingFar100m(this);

    mParam._14 = "Wait";
    mParam._18 = "TurnSmall";
    mParam._1C = "Talk";
    mParam._20 = "TurnSmall";
    _130 = "Spin";
    _134 = "Press";
    _138 = "Pointing";
    _13C = "Reaction";
    _12C = 450.0f;

    if (mRabbitType == RABBIT_TYPE_JUMP_H) {
        mParam._0 = false;
        mParam._1 = false;
    }
}

void Rabbit::control() {
    if (_D8) {
        MR::startSound(this, "SE_SM_NPC_TRAMPLED", -1, -1);
        MR::startSound(this, "SE_SV_RABBIT_TRAMPLED", -1, -1);
    }

    NPCActor::control();
    updateJump();

    if (MR::calcDistanceToPlayer(this) > 5000.0f) {
        MR::validateClipping(this);
    }
    else if (MR::calcDistanceToPlayer(this) < 3000.0f) {
        MR::invalidateClipping(this);
    }
}

void Rabbit::exeAppear() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Appear", nullptr);
    }

    ParabolicPath path;
    TVec3f up(0.0f, cAppearHeight, 0.0f);
    TVec3f negGravity = -mGravity;
    TVec3f zero(0.0f, 0.0f, 0.0f);
    path.initFromUpVector(up, zero, negGravity, cAppearHop);

    f32 frameMax = MR::getBckFrameMax(this);
    TVec3f pos;
    path.calcPosition(&pos, static_cast< f32 >(getNerveStep()) / frameMax);
    mJumpOffset = -pos.y;

    if (MR::isBckStopped(this)) {
        mJumpOffset = 0.0f;
        setNerve(&NrvRabbit::RabbitNrvAppearLand::sInstance);
    }
}

void Rabbit::exeAppearLand() {
    if (MR::isFirstStep(this)) {
        MR::tryStartBck(this, "AppearLand", nullptr);
    }

    if (MR::isBckStopped(this)) {
        popNerve();
    }
}

void Rabbit::exeWait() {
    if (MR::isBckOneTimeAndStopped(this) && mOnGround) {
        MR::startBck(this, "Wait2", nullptr);
    }

    if (getNerveStep() > 30) {
        TVec3f playerDir(*MR::getPlayerPos() - mPosition);
        MR::normalizeOrZero(&playerDir);

        if (!MR::isNearZero(playerDir, 0.001f)) {
            if (isNeedTurn(playerDir) && MR::isBckOneTimeAndStopped(this)) {
                MR::startBck(this, "Turn", nullptr);
            }

            TVec3f up = -mGravity;
            MR::blendQuatUpFront(&_A0, up, playerDir, 0.5f, 0.5f);
        }

        if (MR::isBckOneTimeAndStopped(this)) {
            MR::startBck(this, "Wait2", nullptr);
        }
    }

    if (MR::isNearPlayer(this, cDistEscape)) {
        setNerve(&NrvRabbit::RabbitNrvPreJump::sInstance);
    }
    else if (mWaitTimer != 0) {
        mWaitTimer--;
    }
    else if (!MR::isNearPlayer(this, cDistNear)) {
        f32 playerCoord = MR::calcNearestRailCoord(this, *MR::getPlayerPos());

        if (playerCoord < MR::getRailCoord(this) && MR::getRailCoord(this) > 3.0f * cProgressSpeed) {
            setNerve(&NrvRabbit::RabbitNrvNear::sInstance);
        }
    }
}

void Rabbit::exeGoal() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Wait", nullptr);
    }

    if (getNerveStep() > 30) {
        TVec3f playerDir(*MR::getPlayerPos() - mPosition);
        MR::normalizeOrZero(&playerDir);

        if (!MR::isNearZero(playerDir, 0.001f)) {
            TVec3f up = -mGravity;
            MR::blendQuatUpFront(&_A0, up, playerDir, 0.5f, 0.5f);
        }
    }

    if (MR::isNearPlayer(this, cFinalDisappearRadius)) {
        setNerve(&NrvRabbit::RabbitNrvFinish::sInstance);
    }
}

void Rabbit::exeFinish() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Change", nullptr);
        MR::startSound(this, "SE_SM_RABBIT_CHANGE_JUMP", -1, -1);
    }

    if (MR::isBckStopped(this)) {
        MR::startSound(this, "SE_SM_RABBIT_CHANGE_EFFECT", -1, -1);

        if (MR::isValidSwitchDead(this)) {
            MR::onSwitchDead(this);
        }

        kill();
    }
}

void Rabbit::calcAndSetBaseMtx() {
    TVec3f position(mPosition);
    TVec3f jumpOffset(mGravity * mJumpOffset);
    mPosition += jumpOffset;
    NPCActor::calcAndSetBaseMtx();
    mPosition = position;
}

void Rabbit::calcRailPos(TVec3f* pPos) {
    Triangle triangle;

    TVec3f start(*pPos);
    start += mGravity * -500.0f;

    TVec3f hitPos;
    TVec3f endOffset(mGravity * 1000.0f);
    if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, start, endOffset)) {
        *pPos = hitPos;
    }

    TVec3f diff(*pPos - mPosition);
    f32 normal = MR::max(1.0f, __fabsf(diff.y / 80.0f));
    mRailNormalFactor = MR::sqrt(normal);
}

bool Rabbit::isNeedTurn(const TVec3f& rDir) {
    TVec3f front;
    _A0.getZDir(front);
    return MR::diffAngleAbs(rDir, front) > 0.7853982f;
}

void Rabbit::updateJump() {
    if (isNerve(&NrvRabbit::RabbitNrvAppear::sInstance)) {
        return;
    }

    if (mOnGround) {
        mJumpOffset = 0.0f;
        return;
    }

    if (mJumpOffset >= 0.0f) {
        if (isNerve(&NrvRabbit::RabbitNrvMove::sInstance) || isNerve(&NrvRabbit::RabbitNrvNear::sInstance)) {
            mJumpVelocity = -cJumpSpeed * mRailNormalFactor;
        }
        else {
            mJumpVelocity = 0.0f;
        }

        mJumpOffset = 0.0f;

        if (mJumpLanding) {
            mOnGround = true;
            mJumpLanding = false;
            return;
        }
    }
    else {
        mJumpVelocity += cGravity;

        if (mJumpVelocity >= 0.0f) {
            mJumpLanding = true;
        }
    }

    mJumpOffset += mJumpVelocity;

    if (mJumpOffset >= 0.0f) {
        mJumpOffset = 0.0f;
    }
}

void Rabbit::exeForwardLand() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "JumpEnd", nullptr);
    }

    if (MR::isBckStopped(this)) {
        if (!MR::isNearPlayer(this, cDistWait)) {
            mWaitTimer = 120;
            setNerve(&NrvRabbit::RabbitNrvWait::sInstance);
        }
        else {
            setNerve(&NrvRabbit::RabbitNrvPreJump::sInstance);
        }
    }
}

void Rabbit::exePreJump() {
    if (MR::isFirstStep(this)) {
        MR::setRailDirectionToEnd(this);
        MR::startBck(this, "JumpStart", nullptr);
        MR::startSound(this, "SE_SM_RABBIT_HOP", -1, -1);
    }

    TVec3f railDirection(MR::getRailDirection(this));
    TVec3f up = -mGravity;
    MR::blendQuatUpFront(&_A0, up, railDirection, 0.5f, 0.5f);

    if (MR::isBckStopped(this)) {
        setNerve(&NrvRabbit::RabbitNrvMove::sInstance);
    }
}

void Rabbit::exeMove() {
    if (MR::isFirstStep(this)) {
        MR::setRailDirectionToEnd(this);
        MR::startBck(this, "Jump", nullptr);
        MR::startSound(this, "SE_SM_RABBIT_JUMP", -1, -1);
        mOnGround = false;
        mRailMoveSpeed = cProgressSpeed;

        f32 playerDistance = PSVECDistance(MR::getPlayerPos(), &mPosition);
        f32 speedRate = 1.0f;

        if (playerDistance < 100.0f) {
            speedRate = 2.0f;
        }
        else if (playerDistance < 150.0f) {
            speedRate = 1.8f;
        }
        else if (playerDistance < 200.0f) {
            speedRate = 1.6f;
        }
        else if (playerDistance < 250.0f) {
            speedRate = 1.4f;
        }
        else if (playerDistance < 300.0f) {
            speedRate = 1.2f;
        }
        else if (playerDistance < 350.0f) {
            speedRate = 1.1f;
        }

        mRailMoveSpeed *= speedRate;
        MR::moveCoordToNearestPos(this, mPosition);
        MR::moveCoord(this, 20.0f * mRailMoveSpeed);

        TVec3f railPos(MR::getRailPos(this));
        calcRailPos(&railPos);
        mRailMoveDirection = railPos - mPosition;
        MR::normalizeOrZero(&mRailMoveDirection);
    }

    mPosition += mRailMoveDirection * mRailMoveSpeed;
    mRailMoveSpeed *= cRegistNormal;

    if (MR::isRailReachedGoal(this)) {
        TVec3f railDiff(mPosition - MR::getRailPos(this));

        if (PSVECMag(&railDiff) < 10.0f) {
            setNerve(&NrvRabbit::RabbitNrvGoal::sInstance);
            return;
        }
    }

    if (mOnGround) {
        setNerve(&NrvRabbit::RabbitNrvForwardLand::sInstance);
    }
}

void Rabbit::exeBackwardLand() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "JumpEnd", nullptr);
    }

    if (MR::isBckStopped(this)) {
        if (MR::isNearPlayer(this, cDistWait)) {
            setNerve(&NrvRabbit::RabbitNrvWait::sInstance);
        }
        else {
            setNerve(&NrvRabbit::RabbitNrvPreJumpBack::sInstance);
        }
    }
}

void Rabbit::exePreJumpBack() {
    if (MR::isFirstStep(this)) {
        MR::setRailDirectionToStart(this);
        MR::startBck(this, "JumpStart", nullptr);
        MR::startSound(this, "SE_SM_RABBIT_HOP", -1, -1);
    }

    if (MR::isBckStopped(this)) {
        setNerve(&NrvRabbit::RabbitNrvNear::sInstance);
    }
}

void Rabbit::exeNear() {
    if (MR::isFirstStep(this)) {
        MR::setRailDirectionToStart(this);
        mOnGround = false;
        MR::startBck(this, "Jump", nullptr);
        MR::startSound(this, "SE_SM_RABBIT_JUMP", -1, -1);
        mRailMoveSpeed = cProgressSpeed;
        MR::moveCoordToNearestPos(this, mPosition);
        MR::moveCoord(this, 20.0f * mRailMoveSpeed);

        TVec3f railPos(MR::getRailPos(this));
        calcRailPos(&railPos);
        mRailMoveDirection = railPos - mPosition;
        MR::normalizeOrZero(&mRailMoveDirection);
    }

    mPosition += mRailMoveDirection * mRailMoveSpeed;
    mRailMoveSpeed *= cRegistNormal;

    if (MR::isNearPlayer(this, cDistWait)) {
        setNerve(&NrvRabbit::RabbitNrvWait::sInstance);
    }
    else if (MR::isRailReachedGoal(this)) {
        TVec3f railDiff(mPosition - MR::getRailPos(this));

        if (PSVECMag(&railDiff) < 10.0f) {
            setNerve(&NrvRabbit::RabbitNrvWait::sInstance);
        }
    }
    else {
        MR::calcNearestRailCoord(this, *MR::getPlayerPos());

        if (mOnGround) {
            setNerve(&NrvRabbit::RabbitNrvBackwardLand::sInstance);
        }
    }
}

void Rabbit::exeReaction() {
    if (_D8) {
        MR::startSound(this, "SE_SM_NPC_TRAMPLED", -1, -1);
        MR::startSound(this, "SE_SV_RABBIT_TRAMPLED", -1, -1);
    }

    if (isPointingSe()) {
        MR::startDPDHitSound();
    }

    if (_E4) {
        MR::startLevelSound(this, "SE_SM_LV_RABBIT_POINT", -1, -1, -1);
    }

    if (_D9) {
        MR::startSound(this, "SE_SV_RABBIT_SPIN", -1, -1);
        MR::startSound(this, "SE_SM_RABBIT_SPIN", -1, -1);
    }

    if (_DB) {
        MR::limitedStarPieceHitSound();
        MR::startSound(this, "SE_SM_RABBIT_STAR_PIECE_HIT", -1, -1);
        MR::startSound(this, "SE_SV_RABBIT_STAR_PIECE_HIT", -1, -1);
    }

    MR::tryStartReactionAndPopNerve(this);
}

void Rabbit::exeTalk() {
    MR::isFirstStep(this);

    if (MR::tryStartReactionAndPushNerve(this, &NrvRabbit::RabbitNrvReaction::sInstance)) {
        return;
    }

    if (MR::tryTalkNearPlayerAndStartTalkAction(this) && !MR::isShortTalk(mMsgCtrl)) {
        return;
    }

    if (mRabbitType == RABBIT_TYPE_JUMP_V) {
        if (MR::isGreaterStep(this, 180)) {
            pushNerve(&NrvRabbit::RabbitNrvJumpV::sInstance);
        }
    }
    else if (mRabbitType == RABBIT_TYPE_JUMP_H) {
        if (MR::isGreaterStep(this, 180)) {
            pushNerve(&NrvRabbit::RabbitNrvJumpH::sInstance);
        }
    }
}

void Rabbit::exeJumpV() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "SpinLecture");
    }

    if (MR::isBckStopped(this)) {
        popNerve();
    }
}

void Rabbit::exeJumpH() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "JumpSpinLectureMove");
    }

    if (MR::isBckLooped(this)) {
        popNerve();
    }
}

Rabbit::~Rabbit() {}
