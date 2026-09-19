#include "Game/MapObj/PlantGroup.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util.hpp"

namespace NrvPlantMember {
    NERVE_DECL(HostTypeNrvWait, PlantMember, exeWait);
    NERVE_DECL(HostTypeNrvHint, PlantMember, exeHint);
    NERVE_DECL(HostTypeNrvShakeWeak, PlantMember, exeShakeWeak);
    NERVE_DECL(HostTypeNrvShakeMiddle, PlantMember, exeShakeMiddle);
    NERVE_DECL(HostTypeNrvShakeStrong, PlantMember, exeShakeStrong);

    HostTypeNrvWait HostTypeNrvWait::sInstance;
    HostTypeNrvHint HostTypeNrvHint::sInstance;
    HostTypeNrvShakeWeak HostTypeNrvShakeWeak::sInstance;
    HostTypeNrvShakeMiddle HostTypeNrvShakeMiddle::sInstance;
    HostTypeNrvShakeStrong HostTypeNrvShakeStrong::sInstance;
};  // namespace NrvPlantMember

PlantGroup::PlantGroup(const char* pName)
    : LiveActor(pName), mMembers(nullptr), mMemberCount(7), mPlantType(0), mIsStarPiece(false), mHintPosition(0.0f, 0.0f, 0.0f),
      mHintRotation(0.0f, 0.0f, 0.0f), mHintTimer(300), mHintMember(0) {
}

void PlantGroup::init(const JMapInfoIter& rIter) {
    MR::connectToScene(this, MR::MovementType_MapObj, -1, -1, -1);
    MR::initDefaultPos(this, rIter);

    const char* objectName = nullptr;
    MR::getObjectName(&objectName, rIter);
    if (MR::isEqualString(objectName, "FlowerGroup")) {
        mPlantType = 1;
    } else if (MR::isEqualString(objectName, "FlowerBlueGroup")) {
        mPlantType = 2;
    } else {
        mPlantType = 0;
    }

    s32 memberCount = -1;
    MR::getJMapInfoArg0NoInit(rIter, &memberCount);
    if (memberCount > 0) {
        mMemberCount = memberCount;
    } else if (memberCount == -1) {
        mMemberCount = 7;
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
    MR::setEffectHostSRT(this, "HintShakeLeaf", &mHintPosition, &mHintRotation, nullptr);
    MR::initStarPointerTarget(this, 0.0f, TVec3f(0.0f, 0.0f, 0.0f));
    MR::useStageSwitchSleep(this, rIter);
    MR::useStageSwitchReadAppear(this, rIter);
    if (MR::isValidSwitchAppear(this)) {
        makeActorDead();
        MR::syncStageSwitchAppear(this);
    } else {
        makeActorAppeared();
    }
    MR::tryRegisterDemoCast(this, rIter);
    mHintTimer = MR::getRandom(static_cast< s32 >(3), static_cast< s32 >(10)) * 10;
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

void PlantGroup::initMember(s32 itemCount, const JMapInfoIter& rIter) {
    mMembers = new PlantMember*[mMemberCount];
    for (s32 i = 0; i < mMemberCount; i++) {
        if (mPlantType == 1) {
            mMembers[i] = new PlantMember("花", "Flower", false);
        } else if (mPlantType == 2) {
            mMembers[i] = new PlantMember("青い花", "FlowerBlue", false);
        } else {
            mMembers[i] = new PlantMember("草", "CutBush", true);
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
        const s32 index = MR::getRandom(0, i + 1);
        const bool hasItem = mMembers[i]->mHasItem;
        mMembers[i]->mHasItem = mMembers[index]->mHasItem;
        mMembers[index]->mHasItem = hasItem;
    }

    initHitSensor(1);
    MR::addHitSensorMapObj(this, "境界球", 16, 100.0f, TVec3f(gZeroVec));
}

s32 PlantGroup::placeOnCollisionFormCircle(TVec3f* pCenter, const TVec3f& rGravity, const TVec3f& rAxisX, const TVec3f& rAxisY) {
    pCenter->set(0, 0, 0);
    s32 ring = 0;
    f32 angle = 0.0f;
    f32 angleStep = TWO_PI;
    f32 radius = 160.0f * ring;
    s32 ringCount = 0;
    s32 placedCount = 0;
    for (s32 i = 0; i < mMemberCount; i++) {
        TVec3f offset(rAxisX);
        offset.scale(JMACosRadian(angle));
        offset += rAxisY * JMASinRadian(angle);
        offset.scale(radius);

        TVec3f position(mPosition);
        position += offset;
        TVec3f above(rGravity);
        above.scale(100.0f);
        position -= above;
        TVec3f direction(rGravity);
        direction.scale(1000.0f);
        if (MR::getFirstPolyOnLineToMap(&mMembers[i]->mPosition, nullptr, position, direction)) {
            *pCenter += mMembers[i]->mPosition;
            placedCount++;
        } else {
            mMembers[i]->kill();
        }
        mMembers[i]->initPosture();
        angle += angleStep;
        if (angle >= TWO_PI) {
            ringCount += 6;
            ring++;
            angle = 0.0f;
            angleStep = TWO_PI / ringCount;
            radius = 160.0f * ring;
        }
    }
    pCenter->scale(1.0f / placedCount);
    return placedCount;
}

f32 PlantGroup::calcBoundingSphereRadius(const TVec3f& rCenter) {
    TVec3f minimum(rCenter);
    TVec3f maximum(rCenter);
    for (s32 i = 0; i < mMemberCount; i++) {
        if (MR::isDead(mMembers[i])) {
            continue;
        }
        const TVec3f& position = mMembers[i]->mPosition;
        if (position.x < minimum.x) {
            minimum.x = position.x;
        } else if (maximum.x < position.x) {
            maximum.x = position.x;
        }
        if (position.y < minimum.y) {
            minimum.y = position.y;
        } else if (maximum.y < position.y) {
            maximum.y = position.y;
        }
        if (position.z < minimum.z) {
            minimum.z = position.z;
        } else if (maximum.z < position.z) {
            maximum.z = position.z;
        }
        MR::isNoCalcAnim(mMembers[i]);
        mMembers[i]->calcAnim();
    }
    mPosition.set((maximum + minimum) * 0.5f);
    TVec3f extent(maximum - minimum);
    if (extent.x < extent.y) {
        if (extent.y < extent.z) {
            return extent.z * 0.5f;
        }
        return extent.y * 0.5f;
    }
    if (extent.x < extent.z) {
        return extent.z * 0.5f;
    }
    return extent.x * 0.5f;
}

void PlantGroup::initAfterPlacement() {
    TVec3f gravity;
    TVec3f axisX;
    axisX.set(1, 0, 0);
    TVec3f axisY;
    axisY.set(0, 1, 0);
    MR::calcGravityVector(this, mPosition, &gravity, nullptr, 0);
    MR::makeAxisCrossPlane(&axisX, &axisY, gravity);
    TVec3f center;
    placeOnCollisionFormCircle(&center, gravity, axisX, axisY);
    f32 radius = calcBoundingSphereRadius(center);
    f32 scale = mScale.y;
    HitSensor* sensor = getSensor("境界球");
    f32 sensorRadius = 160.0f + radius * scale;
    sensor->mRadius = sensorRadius;
    MR::setStarPointerTargetRadius3d(this, sensorRadius);
    MR::setClippingTypeSphere(this, 160.0f + radius * scale);
}

void PlantGroup::control() {
    bool pointing = MR::isStarPointerPointing2POnPressButton(this, nullptr, false, false);
    for (s32 i = 0; i < mMemberCount; i++) {
        mMembers[i]->animControl(this);
        mMembers[i]->movement();
        if (pointing) {
            TVec2f velocity(*MR::getStarPointerScreenVelocity(1));
            if (velocity.x * velocity.x + velocity.y * velocity.y > 36.0f) {
                TVec3f position(*MR::getStarPointerWorldPosUsingDepth(1));
                if (mMembers[i]->tryPush(position, 100.0f, 2) == true) {
                    MR::tryRumblePadVeryWeak(this, 1);
                    break;
                }
            }
        }
    }
    emitHintEffect();
}

void PlantGroup::emitHintEffect() {
    if (--mHintTimer > 0) {
        return;
    }
    mHintTimer = 300;
    s32 index = (mHintMember + mMemberCount) % mMemberCount;
    do {
        if (mMembers[index]->mHasItem && mMembers[index]->tryEmitHint()) {
            mHintMember = (index + mMemberCount + 1) % mMemberCount;
            mHintPosition.set(mMembers[index]->mPosition);
            mHintRotation.set(mMembers[index]->mRotation);
            MR::emitEffect(this, "HintShakeLeaf");
            break;
        }
        index = (index + mMemberCount + 1) % mMemberCount;
    } while (index != mHintMember);
}

bool PlantGroup::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (msg == ACTMES_TORNADO_ATTACK || msg == ACTMES_TORNADO_STORM_RANGE || msg == ACTMES_SPIN_STORM_RANGE) {
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
    TVec3f velocity(pReceiver->mHost->mVelocity);
    MR::vecKillElement(velocity, pReceiver->mHost->mGravity, &velocity);
    if (velocity.squared() < 2.0f) {
        return;
    }
    s32 pushType = 0;
    if (MR::isSensorPlayer(pReceiver) || MR::isSensorNpc(pReceiver)) {
        pushType = 1;
    } else if (MR::sendArbitraryMsg(ACTMES_PLANT_GROUP_EMIT_ITEM, pReceiver, pSender)) {
        pushType = 1;
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

void PlantMember::init(const JMapInfoIter& rIter) {
    _9C = false;
    _90 = 1.0f;
    mPushType = 3;
    mHasItem = false;
    MR::onCalcAnim(this);
    initNerve(&NrvPlantMember::HostTypeNrvWait::sInstance);
    appear();
}

bool PlantMember::tryEmitHint() {
    if (isNerve(&NrvPlantMember::HostTypeNrvWait::sInstance)) {
        setNerve(&NrvPlantMember::HostTypeNrvHint::sInstance);
        return true;
    }
    return false;
}

void PlantMember::exeWait() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Wait", nullptr);
    }
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
        TVec3f velocity(-mGravity);
        velocity.scale(30.0f);
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
    TVec3f axis;
    axis.set(1, 0, 0);
    if (MR::isSameDirection(mGravity, axis, 0.01f)) {
        axis.set(0, 1, 0);
    }
    TPos3f posture;
    MR::calcMtxFromGravityAndZAxis(&posture, this, mGravity, axis);
    const f32 angle = MR::getRandom(-PI, PI);
    TPos3f rotation;
    rotation.makeRotate(mGravity, angle);
    posture.concat(rotation, posture);
    TVec3f euler;
    posture.getEuler(euler);
    mRotation.x = _180_PI * euler.x;
    mRotation.y = _180_PI * euler.y;
    mRotation.z = _180_PI * euler.z;
    calcAnim();
}

bool PlantMember::tryShake(HitSensor* pSensor) {
    if (MR::isDead(this)) {
        return false;
    }
    if ((isNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance) || isNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance) ||
         isNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance)) &&
        MR::isLessStep(this, 20)) {
        return false;
    }
    TVec3f distance(pSensor->mPosition);
    distance -= mPosition;
    MR::vecKillElement(distance, mGravity, &distance);
    const f32 length = PSVECMag(&distance);
    if (length < 100.0f) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance);
    } else if (length < 300.0f) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance);
    } else if (length < 500.0f) {
        setNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance);
    }
    return true;
}

bool PlantMember::tryPush(const TVec3f& rPosition, f32 radius, s32 pushType) {
    if (MR::isDead(this)) {
        return false;
    }
    if ((isNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance) || isNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance) ||
         isNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance)) &&
        MR::isLessStep(this, 20)) {
        return false;
    }
    TVec3f distance(rPosition);
    distance -= mPosition;
    const f32 range = radius + 50.0f * mScale.y;
    if (distance.squared() < range * range) {
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
    return false;
}

void PlantMember::animControl(PlantGroup* pGroup) {
    if (isNerve(&NrvPlantMember::HostTypeNrvShakeWeak::sInstance)) {
        if (mPushType == 1) {
            generateItem(pGroup);
        }
    } else if (isNerve(&NrvPlantMember::HostTypeNrvShakeMiddle::sInstance) || isNerve(&NrvPlantMember::HostTypeNrvShakeStrong::sInstance)) {
        if (mPushType == 1 || mPushType == 2) {
            generateItem(pGroup);
        }
    }
}

PlantGroup::~PlantGroup() {
}

PlantMember::~PlantMember() {
}
