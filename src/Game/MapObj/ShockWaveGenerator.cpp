#include "Game/MapObj/ShockWaveGenerator.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include <JSystem/JMath/JMath.hpp>

namespace NrvShockWaveGenerator {
    NEW_NERVE(ShockWaveGeneratorNrvWait, ShockWaveGenerator, Wait);
    NEW_NERVE(ShockWaveGeneratorNrvDemoEcho, ShockWaveGenerator, DemoEcho);
    NEW_NERVE(ShockWaveGeneratorNrvGenerate, ShockWaveGenerator, Generate);
};  // namespace NrvShockWaveGenerator

namespace {
    const char* cDemoCameraName = "注目カメラ";
};  // namespace

ShockWaveGenerator::ShockWaveGenerator(const char* pName) : LiveActor(pName) {
    mCameraInfo = nullptr;
}

void ShockWaveGenerator::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    initModelManagerWithAnm("ShockWaveGenerator", nullptr, false);
    MR::connectToSceneIndirectMapObj(this);
    initHitSensor(3);
    MR::addBodyMessageSensorMapObj(this);
    MR::addHitSensorMapObjSimple(this, "spin", 8, 400.0f, TVec3f(0.0f, 200.0f, 0.0f));
    MR::addHitSensorEnemyAttack(this, "shock", 16, 1000.0f, TVec3f(0.0f, 0.0f, 0.0f));
    MR::initCollisionParts(this, "ShockWaveGenerator", getSensor("body"), nullptr);

    if (!MR::initActorCamera(this, rIter, &mCameraInfo)) {
        mCameraInfo = nullptr;
    }

    initEffectKeeper(0, nullptr, false);
    initSound(2, false);
    MR::setClippingTypeSphereContainsModelBoundingBox(this, 100.0f);
    MR::useStageSwitchSleep(this, rIter);
    initNerve(&NrvShockWaveGenerator::ShockWaveGeneratorNrvWait::sInstance);
    makeActorAppeared();
}

void ShockWaveGenerator::exeWait() {
    if (MR::isFirstStep(this)) {
        MR::startBrk(this, "SpinHit");
        MR::startBtk(this, "SpinHit");
        MR::setBrkFrameAndStop(this, 0.0f);
        MR::setBtkFrameAndStop(this, 0.0f);
        MR::validateClipping(this);
    }
}

void ShockWaveGenerator::exeDemoEcho() {
    if (MR::isFirstStep(this)) {
        MR::startActorCameraTargetSelf(this, mCameraInfo, -1);
    }

    MR::tryRumblePadVeryWeak(this, 0);

    if (MR::isStep(this, 1)) {
        MR::endDemo(this, cDemoCameraName);
        setNerve(&NrvShockWaveGenerator::ShockWaveGeneratorNrvGenerate::sInstance);
    }
}

void ShockWaveGenerator::exeGenerate() {
    if (MR::isFirstStep(this)) {
        MR::startAllAnim(this, "SpinHit");
        sendMsgShockWaveToNearEnemy();
        MR::emitEffect(this, "ShockWave");
        MR::shakeCameraWeak();
        MR::tryRumblePadStrong(this, 0);
    }

    if (MR::isStep(this, 50)) {
        if (mCameraInfo != nullptr) {
            MR::endActorCamera(this, mCameraInfo, false, -1);
        }

        setNerve(&NrvShockWaveGenerator::ShockWaveGeneratorNrvWait::sInstance);
    }
}

bool ShockWaveGenerator::receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    bool isBusy = false;

    if (isNerve(&NrvShockWaveGenerator::ShockWaveGeneratorNrvGenerate::sInstance)
        || isNerve(&NrvShockWaveGenerator::ShockWaveGeneratorNrvDemoEcho::sInstance)) {
        isBusy = true;
    }

    if (isBusy) {
        return false;
    }

    if (MR::isDemoActive()) {
        return false;
    }

    if (pReceiver == getSensor("body") && MR::isMsgStarPieceAttack(msg)) {
        startShockWave();
        return true;
    }

    if (pReceiver == getSensor("spin") && MR::isMsgPlayerSpinAttack(msg) && isHitCylinder(pSender, pReceiver)) {
        startShockWave();
        return true;
    }

    return false;
}

void ShockWaveGenerator::startShockWave() {
    MR::invalidateClipping(this);
    MR::startSound(this, "SE_OJ_SHOCK_WAVE_GENERATE", -1, -1);

    if (mCameraInfo != nullptr) {
        bool shouldStartDemo = false;

        if (mCameraInfo != nullptr && MR::isNearPlayerAnyTime(this, 2000.0f)) {
            shouldStartDemo = true;
        }

        if (shouldStartDemo) {
            if (MR::tryStartDemoWithoutCinemaFrame(this, cDemoCameraName)) {
                setNerve(&NrvShockWaveGenerator::ShockWaveGeneratorNrvDemoEcho::sInstance);
            }
        }
        else {
            setNerve(&NrvShockWaveGenerator::ShockWaveGeneratorNrvGenerate::sInstance);
        }
    }
    else {
        MR::stopSceneForDefaultHit(1);
        setNerve(&NrvShockWaveGenerator::ShockWaveGeneratorNrvGenerate::sInstance);
    }
}

void ShockWaveGenerator::sendMsgShockWaveToNearEnemy() {
    HitSensor* shockSensor = getSensor("shock");

    for (s32 i = 0; i < shockSensor->mSensorCount; i++) {
        HitSensor* sensor = shockSensor->mSensors[i];

        if (MR::isSensorEnemy(sensor)) {
            MR::sendMsgToEnemyAttackShockWave(sensor, shockSensor);
        }
    }
}

bool ShockWaveGenerator::isHitCylinder(HitSensor* pSender, HitSensor* pReceiver) const {
    TVec3f sensorDiff;
    TVec3f up;
    TVec3f projected;

    sensorDiff.subInline(pSender->mPosition, pReceiver->mPosition);
    MR::calcUpVec(&up, this);
    JMAVECScaleAdd(&up, &sensorDiff, &projected, -up.dot(sensorDiff));

    f32 radius = pSender->mRadius;
    return PSVECMag(&projected) <= radius + 140.0f;
}

ShockWaveGenerator::~ShockWaveGenerator() {}
