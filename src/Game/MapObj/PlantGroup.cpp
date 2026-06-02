#include "Game/MapObj/PlantGroup.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/MapObj/CutBushModelObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util.hpp"
#include "JSystem/JGeometry/TMatrix.hpp"
#include "JSystem/JGeometry/TUtil.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"
#include <revolution/mtx.h>

#define PLANT_GROUP_DEFAULT_COUNT 7
#define PLANT_GROUP_TYPE_CUT_BUSH 0
#define PLANT_GROUP_TYPE_FLOWER 1
#define PLANT_GROUP_TYPE_FLOWER_BLUE 2
#define PLANT_GROUP_HINT_INTERVAL 300
#define PLANT_MEMBER_PUSH_NONE 3
#define PLANT_MEMBER_PUSH_EMIT 1
#define PLANT_MEMBER_PUSH_POINTER 2

class PlantMember : public CutBushModelObj {
public:
    PlantMember(const char* pName, const char* pModelName, bool useLightCtrl, MtxPtr pMtx)
        : CutBushModelObj(pName, pModelName, useLightCtrl, pMtx) {}

    virtual ~PlantMember();
    virtual void init(const JMapInfoIter&);

    bool tryEmitHint();
    void exeHint();
    void exeShakeWeak();
    void exeShakeMiddle();
    void exeShakeStrong();
    bool generateItem(PlantGroup*);
    void initPosture();
    bool tryShake(HitSensor*);
    bool tryPush(const TVec3f&, f32, s32);
    void animControl(PlantGroup*);

    /* 0x90 */ f32 mPostureScale;
    /* 0x94 */ u8 _94[4];
    /* 0x98 */ s32 mPushType;
    /* 0x9C */ bool _9C;
    /* 0x9D */ bool mHasItem;
    /* 0x9E */ u8 _9E[2];
};

namespace NrvPlantMember {
    NERVE(HostTypeNrvWait);
    NERVE(HostTypeNrvHint);
    NERVE(HostTypeNrvShakeWeak);
    NERVE(HostTypeNrvShakeMiddle);
    NERVE(HostTypeNrvShakeStrong);
};

PlantGroup::PlantGroup(const char* pName) : LiveActor(pName) {
    mMembers = nullptr;
    mMemberCount = PLANT_GROUP_DEFAULT_COUNT;
    mPlantType = PLANT_GROUP_TYPE_CUT_BUSH;
    mIsStarPiece = false;
    mHintEffectPosition.set(0.0f, 0.0f, 0.0f);
    mHintEffectRotation.set(0.0f, 0.0f, 0.0f);
    mHintEffectTimer = PLANT_GROUP_HINT_INTERVAL;
    mHintStartIndex = 0;
}

void PlantGroup::init(const JMapInfoIter& rIter) {
    MR::connectToScene(this, MR::MovementType_MapObj, -1, -1, -1);
    MR::initDefaultPos(this, rIter);

    const char* objectName = nullptr;
    MR::getObjectName(&objectName, rIter);

    if (MR::isEqualString(objectName, "FlowerGroup")) {
        mPlantType = PLANT_GROUP_TYPE_FLOWER;
    } else if (MR::isEqualString(objectName, "FlowerBlueGroup")) {
        mPlantType = PLANT_GROUP_TYPE_FLOWER_BLUE;
    } else {
        mPlantType = PLANT_GROUP_TYPE_CUT_BUSH;
    }

    s32 memberCount = -1;
    MR::getJMapInfoArg0NoInit(rIter, &memberCount);

    if (memberCount > 0) {
        mMemberCount = memberCount;
    } else if (memberCount == -1) {
        mMemberCount = PLANT_GROUP_DEFAULT_COUNT;
    }

    s32 itemCount = 0;
    MR::getJMapInfoArg1NoInit(rIter, &itemCount);

    if (itemCount < 0) {
        itemCount = 0;
    }

    s32 itemType = -1;
    MR::getJMapInfoArg2NoInit(rIter, &itemType);

    if (itemType == 1) {
        mIsStarPiece = true;
        MR::declareStarPiece(this, itemCount);
    } else {
        mIsStarPiece = false;
        MR::declareCoin(this, itemCount);
    }

    initMember(itemCount, rIter);
    initSound(4, false);
    initEffectKeeper(0, "Bush", false);
    MR::setEffectHostSRT(this, "HintShakeLeaf", &mHintEffectPosition, &mHintEffectRotation, nullptr);

    MR::initStarPointerTarget(this, 100.0f, TVec3f(0.0f, 0.0f, 0.0f));
    MR::useStageSwitchSleep(this, rIter);
    MR::useStageSwitchReadAppear(this, rIter);

    if (MR::isValidSwitchAppear(this)) {
        makeActorDead();
        MR::syncStageSwitchAppear(this);
    } else {
        makeActorAppeared();
    }

    MR::tryRegisterDemoCast(this, rIter);
    mHintEffectTimer = MR::getRandom((s32)3, (s32)10) * 10;
}

void PlantGroup::makeActorAppeared() {
    LiveActor::makeActorAppeared();

    for (s32 i = 0; i < mMemberCount; i++) {
        mMembers[i]->makeActorAppeared();
    }
}

void PlantGroup::makeActorDead() {
    LiveActor::makeActorDead();

    for (s32 i = 0; i < mMemberCount; i++) {
        mMembers[i]->makeActorDead();
    }
}

void PlantGroup::initMember(s32 itemCount, const JMapInfoIter&) {
    mMembers = new PlantMember*[mMemberCount];

    for (s32 i = 0; i < mMemberCount; i++) {
        if (mPlantType == PLANT_GROUP_TYPE_FLOWER) {
            mMembers[i] = new PlantMember("\x89\xD4", "Flower", false, nullptr);
        } else if (mPlantType == PLANT_GROUP_TYPE_FLOWER_BLUE) {
            mMembers[i] = new PlantMember("\x90\xC2\x82\xA2\x89\xD4", "FlowerBlue", false, nullptr);
        } else {
            mMembers[i] = new PlantMember("\x91\x90", "CutBush", true, nullptr);
        }

        mMembers[i]->initWithoutIter();
        MR::invalidateClipping(mMembers[i]);

        if (i < itemCount) {
            mMembers[i]->mHasItem = true;
        } else {
            mMembers[i]->mHasItem = false;
        }
    }

    for (s32 i = 0; i < mMemberCount; i++) {
        s32 swapIndex = MR::getRandom((s32)0, i + 1);
        bool hasItem = mMembers[i]->mHasItem;
        mMembers[i]->mHasItem = mMembers[swapIndex]->mHasItem;
        mMembers[swapIndex]->mHasItem = hasItem;
    }

    initHitSensor(1);
    MR::addHitSensorMapObj(this, "\x8B\xAB\x8A\x45\x8B\x85", 16, 100.0f, TVec3f(gZeroVec));
}

s32 PlantGroup::placeOnCollisionFormCircle(TVec3f* pAverage, const TVec3f& rUp, const TVec3f& rAxisA, const TVec3f& rAxisB) {
    pAverage->set(0, 0, 0);

    s32 ring = 0;
    s32 ringSlots = 0;
    s32 hitCount = 0;
    f32 angle = 0.0f;
    f32 angleStep = TWO_PI;
    f32 radius = 160.0f * ring;

    for (s32 i = 0; i < mMemberCount; i++) {
        TVec3f radial(rAxisA);
        radial.scale(MR::cos(angle));

        TVec3f side(rAxisB);
        side.scale(MR::sin(angle));
        radial += side;
        radial.scale(radius);

        TVec3f lineStart(mPosition);
        lineStart += radial;

        TVec3f down(rUp);
        down.scale(100.0f);
        lineStart -= down;

        TVec3f lineOffset(rUp);
        lineOffset.scale(1000.0f);

        if (MR::getFirstPolyOnLineToMap(&mMembers[i]->mPosition, nullptr, lineStart, lineOffset)) {
            *pAverage += mMembers[i]->mPosition;
            hitCount++;
        } else {
            mMembers[i]->kill();
        }

        mMembers[i]->initPosture();

        angle += angleStep;

        if (angle >= TWO_PI) {
            ringSlots += 6;
            ring++;
            angle = 0.0f;
            angleStep = TWO_PI / ringSlots;
            radius = 160.0f * ring;
        }
    }

    pAverage->scale(1.0f / hitCount);
    return hitCount;
}

f32 PlantGroup::calcBoundingSphereRadius(const TVec3f& rAverage) {
    TVec3f min(rAverage);
    TVec3f max(rAverage);

    for (s32 i = 0; i < mMemberCount; i++) {
        PlantMember* member = mMembers[i];

        if (!MR::isDead(member)) {
            if (member->mPosition.x < min.x) {
                min.x = member->mPosition.x;
            } else if (max.x < member->mPosition.x) {
                max.x = member->mPosition.x;
            }

            if (member->mPosition.y < min.y) {
                min.y = member->mPosition.y;
            } else if (max.y < member->mPosition.y) {
                max.y = member->mPosition.y;
            }

            if (member->mPosition.z < min.z) {
                min.z = member->mPosition.z;
            } else if (max.z < member->mPosition.z) {
                max.z = member->mPosition.z;
            }

            MR::isNoCalcAnim(member);
            member->calcAnim();
        }
    }

    TVec3f center(max);
    center += min;
    center.scale(0.5f);
    mPosition.set(center);

    TVec3f span(max);
    span -= min;

    if (span.x < span.y) {
        if (span.y < span.z) {
            return 0.5f * span.z;
        }

        return 0.5f * span.y;
    }

    if (span.x < span.z) {
        return 0.5f * span.z;
    }

    return 0.5f * span.x;
}

void PlantGroup::initAfterPlacement() {
    TVec3f axisA;
    axisA.set(1, 0, 0);

    TVec3f axisB;
    axisB.set(0, 1, 0);

    TVec3f gravity;
    MR::calcGravityVector(this, mPosition, &gravity, nullptr, 0);
    MR::makeAxisCrossPlane(&axisA, &axisB, gravity);

    TVec3f average;
    placeOnCollisionFormCircle(&average, gravity, axisA, axisB);

    f32 radius = calcBoundingSphereRadius(average);
    f32 scale = mScale.y;
    getSensor("\x8B\xAB\x8A\x45\x8B\x85")->mRadius = 160.0f + radius * scale;
    MR::setStarPointerTargetRadius3d(this, 160.0f + radius * scale);
    MR::setClippingTypeSphere(this, 160.0f + radius * scale);
}

void PlantGroup::control() {
    bool isPointed = MR::isStarPointerPointing2POnPressButton(this, nullptr, false, false);

    for (s32 i = 0; i < mMemberCount; i++) {
        mMembers[i]->animControl(this);
        mMembers[i]->calcAnim();

        if (isPointed) {
            TVec2f* pointerVelocity = MR::getStarPointerScreenVelocity(1);
            f32 velocityX = pointerVelocity->x;
            f32 velocityY = pointerVelocity->y;

            if (36.0f < velocityX * velocityX + velocityY * velocityY) {
                TVec3f pointerPosition(*MR::getStarPointerWorldPosUsingDepth(1));

                if (mMembers[i]->tryPush(pointerPosition, 100.0f, PLANT_MEMBER_PUSH_POINTER)) {
                    MR::tryRumblePadVeryWeak(this, 1);
                    break;
                }
            }
        }
    }

    emitHintEffect();
}

void PlantGroup::emitHintEffect() {
    mHintEffectTimer--;

    if (mHintEffectTimer > 0) {
        return;
    }

    mHintEffectTimer = PLANT_GROUP_HINT_INTERVAL;
    s32 index = (mMemberCount + mHintStartIndex) % mMemberCount;

    while (true) {
        PlantMember* member = mMembers[index];

        if (member->mHasItem && member->tryEmitHint()) {
            mHintStartIndex = (index + mMemberCount + 1) % mMemberCount;
            mHintEffectPosition.set(member->mPosition);
            mHintEffectRotation.set(member->mRotation);
            MR::emitEffect(this, "HintShakeLeaf");
            return;
        }

        index = (index + mMemberCount + 1) % mMemberCount;

        if (index == mHintStartIndex) {
            break;
        }
    }
}

bool PlantGroup::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor*) {
    if (msg - 0x31 <= 2) {
        for (s32 i = 0; i < mMemberCount; i++) {
            mMembers[i]->tryShake(pSender);
        }
    }

    return false;
}

void PlantGroup::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (!MR::isSensorPlayer(pReceiver) && !MR::isSensorEnemy(pReceiver)) {
        return;
    }

    TVec3f horizontalVelocity(pReceiver->mHost->mVelocity);
    MR::vecKillElement(horizontalVelocity, pReceiver->mHost->mGravity, &horizontalVelocity);

    if (horizontalVelocity.squared() < 2.0f) {
        return;
    }

    s32 pushType = 0;

    if (MR::isSensorPlayer(pReceiver) || MR::isSensorNpc(pReceiver)) {
        pushType = PLANT_MEMBER_PUSH_EMIT;
    } else if (MR::sendArbitraryMsg(ACTMES_PLANT_GROUP_EMIT_ITEM, pReceiver, pSender)) {
        pushType = PLANT_MEMBER_PUSH_EMIT;
    }

    for (s32 i = 0; i < mMemberCount; i++) {
        mMembers[i]->tryPush(pReceiver->mPosition, pReceiver->mRadius, pushType);
    }
}

void PlantGroup::startClipped() {
    LiveActor::startClipped();

    for (s32 i = 0; i < mMemberCount; i++) {
        mMembers[i]->startClipped();
    }
}

void PlantGroup::endClipped() {
    LiveActor::endClipped();

    for (s32 i = 0; i < mMemberCount; i++) {
        mMembers[i]->endClipped();
    }
}

void PlantMember::init(const JMapInfoIter&) {
    _9C = false;
    mPostureScale = 1.0f;
    mPushType = PLANT_MEMBER_PUSH_NONE;
    mHasItem = false;
    MR::onCalcAnim(this);
    initNerve(&NrvPlantMember::HostTypeNrvWait::sInstance);
    initAfterPlacement();
}

bool PlantMember::tryEmitHint() {
    if (isNerve(&NrvPlantMember::HostTypeNrvWait::sInstance)) {
        setNerve(&NrvPlantMember::HostTypeNrvHint::sInstance);
        return true;
    }

    return false;
}

void PlantMember::exeHint() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "HintShake", nullptr);
    }

    if (MR::isBckStopped(this)) {
        setNerve(&NrvPlantMember::HostTypeNrvWait::sInstance);
    }
}

void PlantMember::exeShakeWeak() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Shake", nullptr);
        MR::setBckFrame(this, 3.0f);
        MR::startSound(this, "SE_OJ_LEAVES_SWING", -1, -1);
        MR::setBckRate(this, 0.5f);
    }

    if (MR::isBckStopped(this)) {
        setNerve(&NrvPlantMember::HostTypeNrvWait::sInstance);
    }
}

void PlantMember::exeShakeMiddle() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Shake", nullptr);
        MR::setBckFrame(this, 3.0f);
        MR::startSound(this, "SE_OJ_LEAVES_SWING", -1, -1);
        MR::setBckRate(this, 1.0f);
    }

    if (MR::isBckStopped(this)) {
        setNerve(&NrvPlantMember::HostTypeNrvWait::sInstance);
    }
}

void PlantMember::exeShakeStrong() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Shake", nullptr);
        MR::setBckFrame(this, 3.0f);
        MR::startSound(this, "SE_OJ_LEAVES_SWING", -1, -1);
        MR::setBckRate(this, 1.5f);
    }

    if (MR::isBckStopped(this)) {
        setNerve(&NrvPlantMember::HostTypeNrvWait::sInstance);
    }
}

bool PlantMember::generateItem(PlantGroup* pGroup) {
    if (mHasItem == true) {
        TVec3f itemDirection;
        itemDirection.negate(mGravity);
        itemDirection.scale(10.0f);

        if (pGroup->mIsStarPiece) {
            MR::startSound(pGroup, "SE_OJ_STAR_PIECE_BURST", -1, -1);
            MR::appearStarPiece(pGroup, mPosition, 1, 10.0f, 40.0f, false);
        } else {
            MR::appearCoinPop(pGroup, mPosition, 1);
        }

        mHasItem = false;
        return true;
    }

    return false;
}

void PlantMember::initPosture() {
    MR::calcGravityVector(this, &mGravity, nullptr, 0);

    TVec3f front;
    front.set(1, 0, 0);

    if (MR::isSameDirection(mGravity, front, 0.01f)) {
        front.set(0, 1, 0);
    }

    TPos3f baseMtx;
    MR::calcMtxFromGravityAndZAxis(&baseMtx, this, mGravity, front);

    TPos3f rotateMtx;
    rotateMtx.setRotateInlineZeroTrans(mGravity, MR::getRandom(-PI, PI));
    baseMtx.concat(rotateMtx, baseMtx);

    TVec3f rotation;

    if ((baseMtx[2][0] - 1.0f) >= -0.0000038146973f) {
        rotation.x = JMath::sAtanTable.atan2_(-baseMtx[0][1], baseMtx[1][1]);
        rotation.y = -HALF_PI;
        rotation.z = 0.0f;
    } else if ((baseMtx[2][0] + 1.0f) <= 0.0000038146973f) {
        rotation.x = JMath::sAtanTable.atan2_(baseMtx[0][1], baseMtx[1][1]);
        rotation.y = HALF_PI;
        rotation.z = 0.0f;
    } else {
        rotation.x = JMath::sAtanTable.atan2_(baseMtx[2][1], baseMtx[2][2]);
        rotation.z = JMath::sAtanTable.atan2_(baseMtx[1][0], baseMtx[0][0]);
        rotation.y = JGeometry::TUtil<f32>::asin(-baseMtx[2][0]);
    }

    mRotation.x = _180_PI * rotation.x;
    mRotation.y = _180_PI * rotation.y;
    mRotation.z = _180_PI * rotation.z;
    calcAnim();
}

bool PlantMember::tryShake(HitSensor* pSensor) {
    if (MR::isDead(this)) {
        return false;
    }

    if ((isNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance) || isNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance)
         || isNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance))
        && MR::isLessStep(this, 20)) {
        return false;
    }

    TVec3f sensorOffset(pSensor->mPosition);
    sensorOffset -= mPosition;
    MR::vecKillElement(sensorOffset, mGravity, &sensorOffset);
    f32 distance = PSVECMag(&sensorOffset);

    if (distance < 100.0f) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance);
    } else if (distance < 300.0f) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance);
    } else if (distance < 500.0f) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance);
    }

    return true;
}

bool PlantMember::tryPush(const TVec3f& rPosition, f32 radius, s32 pushType) {
    if (MR::isDead(this)) {
        return false;
    }

    if ((isNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance) || isNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance)
         || isNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance))
        && MR::isLessStep(this, 20)) {
        return false;
    }

    TVec3f offset(rPosition);
    offset -= mPosition;

    f32 hitRadius = radius + 50.0f * mScale.x;
    hitRadius *= hitRadius;

    if (offset.squared() >= hitRadius) {
        return false;
    }

    mPushType = pushType;

    if (isNerve(&NrvPlantMember::HostTypeNrvWait::sInstance) || isNerve(&NrvPlantMember::HostTypeNrvHint::sInstance)) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance);
    } else if (isNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance)) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance);
    } else if (isNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance)) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance);
    } else if (isNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance)) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance);
    }

    return true;
}

void PlantMember::animControl(PlantGroup* pGroup) {
    if (isNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance)) {
        if (mPushType == PLANT_MEMBER_PUSH_EMIT) {
            generateItem(pGroup);
        }
    } else if (isNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance) || isNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance)) {
        if (mPushType - 1 <= 1) {
            generateItem(pGroup);
        }
    }
}

PlantGroup::~PlantGroup() {}

PlantMember::~PlantMember() {}

namespace NrvPlantMember {
    INIT_NERVE(HostTypeNrvWait);
    INIT_NERVE(HostTypeNrvHint);
    INIT_NERVE(HostTypeNrvShakeWeak);
    INIT_NERVE(HostTypeNrvShakeMiddle);
    INIT_NERVE(HostTypeNrvShakeStrong);

    void HostTypeNrvShakeStrong::execute(Spine* pSpine) const {
        PlantMember* member = reinterpret_cast<PlantMember*>(pSpine->mExecutor);
        member->exeShakeStrong();
    }

    void HostTypeNrvShakeMiddle::execute(Spine* pSpine) const {
        PlantMember* member = reinterpret_cast<PlantMember*>(pSpine->mExecutor);
        member->exeShakeMiddle();
    }

    void HostTypeNrvShakeWeak::execute(Spine* pSpine) const {
        PlantMember* member = reinterpret_cast<PlantMember*>(pSpine->mExecutor);
        member->exeShakeWeak();
    }

    void HostTypeNrvHint::execute(Spine* pSpine) const {
        PlantMember* member = reinterpret_cast<PlantMember*>(pSpine->mExecutor);
        member->exeHint();
    }

    void HostTypeNrvWait::execute(Spine* pSpine) const {
        PlantMember* member = reinterpret_cast<PlantMember*>(pSpine->mExecutor);

        if (MR::isFirstStep(member)) {
            MR::startBck(member, "Wait", nullptr);
        }
    }
};
