#include "resource/TextEncoding.hpp"
#include "Game/Boss/KoopaParts.hpp"
#include "Game/Boss/Koopa.hpp"
#include "Game/Boss/KoopaFireShort.hpp"
#include "Game/Boss/KoopaFireStairs.hpp"
#include "Game/Boss/KoopaFunction.hpp"
#include "Game/Boss/KoopaPlanetShadow.hpp"
#include "Game/Boss/KoopaRockBreak.hpp"
#include "Game/Boss/KoopaShockWave.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/ActorJointCtrl.hpp"
#include "Game/LiveActor/LiveActorGroup.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/Map/KoopaBattleMapPlanet.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace {
    static const s32 sFireStairsNum = 16;
    static const s32 sFireShortNum = 32;
    static const s32 sShockWaveNum = 8;
};  // namespace

KoopaParts::KoopaParts(Koopa* pKoopa, const JMapInfoIter& rIter)
    : mKoopa(pKoopa), mPlanetRadius(1300.0f), mThornBig(), mThornSmall(), mArmorBreak(), mThornBreak(), mPlanetLv1(), mPlanetShadow(), mShockWave(),
      mFireShort(), mFireStairs(), mKoopaSwitchKeeper(), mKoopaViewSwitchKeeper(), mKoopaPowerUpSwitch(), mRock(), mRockBreak(), mRollBall(),
      mPlanetLv2(), mPlanetLv3(), mHoleSunPlanetOutside(), mHoleSunPlanetOutsideBloom(), mHoleSunPlanetInside(), mHoleSunPlanetInsideBloom(),
      mPeach(), mKoopaJr(), mKoopaJrShip(), mMeteor1(), mMeteor2(), mMeteor3(), mActorCameraInfo() {
    mActorCameraInfo = new ActorCameraInfo(rIter);
    MR::declareCameraRegisterVec(mKoopa, 0, &mKoopa->mPosition);
}

namespace {
    PartsModel* createKoopaBodyParts(LiveActor* pActor, const char* pName, const char* pModelName, const char* pJointName) {
        PartsModel* pPartsModel = new PartsModel(pActor, pName, pModelName, nullptr, MR::DrawBufferType_Enemy, false);

        pPartsModel->loadFixedPosition(pJointName);
        pPartsModel->initWithoutIter();

        MR::initLightCtrl(pPartsModel);
        MR::registerDemoSimpleCastAll(pPartsModel);

        return pPartsModel;
    };

    KoopaBattleMapPlanet* createKoopaBattleMapPlanet(const char* pName1, const char* pName2, const char* pResetPositionName, bool a1, bool a2,
                                                     bool a3) {
        KoopaBattleMapPlanet* pBattleMapPlanet = new KoopaBattleMapPlanet(pName1, pName2, a1, a2, a3);
        MR::resetPosition(pBattleMapPlanet, pResetPositionName);
        pBattleMapPlanet->initWithoutIter();
        return pBattleMapPlanet;
    };
};  // namespace

KoopaFireStairs* KoopaParts::emitFireStairsToPos(const KoopaBattleMapStair* pBattleMapStair, const TVec3f& rPosition, bool useFront) {
    LiveActor* actor = mFireStairs->getDeadActor();

    if (actor == nullptr) {
        return nullptr;
    }

    KoopaFireStairs* pFireStairs = static_cast< KoopaFireStairs* >(actor);
    pFireStairs->mPosition.set(rPosition);

    if (useFront) {
        pFireStairs->setInfo(pBattleMapStair, KoopaFunction::getKoopaFrontPtr(mKoopa));
    } else {
        pFireStairs->setInfo(pBattleMapStair, nullptr);
    }

    pFireStairs->appear();

    return pFireStairs;
}

void KoopaParts::killFireStairsAll() {
    for (int idx = 0; idx < ::sFireStairsNum; idx++) {
        LiveActor* pActor = mFireStairs->getActor(idx);

        if (!MR::isDead(pActor)) {
            pActor->makeActorDead();
        }
    }
}

void KoopaParts::emitFireShort(bool isFast, bool isCurve) {
    KoopaFireShort* pFireShort = static_cast< KoopaFireShort* >(mFireShort->getDeadActor());

    if (pFireShort == nullptr) {
        return;
    }

    if (isCurve) {
        pFireShort->emitCurve();
    } else if (isFast) {
        pFireShort->emitFast();
    } else {
        pFireShort->emitNormal();
    }
}

void KoopaParts::emitFireLongTime() {
    KoopaFireShort* pFireShort = static_cast< KoopaFireShort* >(mFireShort->getDeadActor());

    if (pFireShort == nullptr) {
        return;
    }

    pFireShort->emitLongTime();
}

void KoopaParts::emitShockWave() {
    LiveActor* pActor = mShockWave->getDeadActor();

    if (pActor == nullptr) {
        return;
    }

    pActor->appear();
}

TVec3f& KoopaParts::getPlanetPos() const {
    if (KoopaFunction::isKoopaVs1(mKoopa) || KoopaFunction::isKoopaVs2(mKoopa)) {
        return mPlanetLv1->mPosition;
    }

    if (KoopaFunction::isKoopaLv1(mKoopa)) {
        return mPlanetLv1->mPosition;
    }

    if (KoopaFunction::isKoopaLv2(mKoopa)) {
        return mPlanetLv2->mPosition;
    }

    return mPlanetLv3->mPosition;
}

f32 KoopaParts::getPlanetRadius() const {
    return mPlanetRadius;
}

void KoopaParts::appearHoleSunPlanetInside() {
    mHoleSunPlanetInside->appear();
}

void KoopaParts::appearHoleSunPlanetOutside() {
    mHoleSunPlanetOutside->appear();
}

void KoopaParts::killHoleSunPlanetOutside() {
    mHoleSunPlanetOutside->kill();
}

void KoopaParts::createPlanetShadow() {
    mPlanetShadow = new KoopaPlanetShadow(mKoopa);
    mPlanetShadow->initWithoutIter();
    mPlanetShadow->kill();
}

void KoopaParts::initVs1() {
    createFireStairs(false);

    mPlanetLv1 = ::createKoopaBattleMapPlanet(CP932("クッパ惑星"), "KoopaBattleMapPlanet", CP932("惑星中心"), false, false, false);

    createPlanetShadow();
    createCommonParts();
}

void KoopaParts::initVs2() {
    mPlanetLv1 = ::createKoopaBattleMapPlanet(CP932("クッパ惑星"), "KoopaBattleMapPlanetVs2", CP932("惑星中心"), true, false, false);

    createCommonParts();
}

namespace {
    ModelObjNpc* createDemoNpc(const char* pName, const char* pModelArcName) {
        ModelObjNpc* pModelObjNpc = new ModelObjNpc(pName, pModelArcName, nullptr);
        pModelObjNpc->initWithoutIter();
        MR::resetPosition(pModelObjNpc, CP932("デモ中心"));

        pModelObjNpc->mLodCtrl->invalidateClipping();
        pModelObjNpc->mLodCtrl->invalidate();

        pModelObjNpc->mJointCtrl->endFaceCtrl(-1);

        MR::initLightCtrl(pModelObjNpc);

        pModelObjNpc->kill();

        return pModelObjNpc;
    };

    ModelObj* createDemoEnemy(const char* pName1, const char* pName2) {
        ModelObj* pModelObj = MR::createModelObjEnemy(pName1, pName2, nullptr);
        pModelObj->initWithoutIter();
        MR::resetPosition(pModelObj, CP932("デモ中心"));
        pModelObj->kill();
        return pModelObj;
    };
};  // namespace

void KoopaParts::initVs3() {
    createFireStairs(true);

    mPlanetLv1 = ::createKoopaBattleMapPlanet(CP932("クッパ惑星Ｌｖ１"), "KoopaBattleMapPlanetVs3Lv1", CP932("惑星中心"), false, false, true);
    mPlanetLv2 = ::createKoopaBattleMapPlanet(CP932("クッパ惑星Ｌｖ２"), "KoopaBattleMapPlanetVs3Lv2", CP932("惑星Ｌｖ２"), false, false, true);
    mPlanetLv3 = ::createKoopaBattleMapPlanet(CP932("クッパ惑星Ｌｖ３"), "KoopaBattleMapPlanetVs3Lv3", CP932("惑星Ｌｖ３"), false, true, false);

    MR::startBrk(mPlanetLv1, "Wait");
    MR::startBrk(mPlanetLv2, "Wait");

    createPlanetShadow();

    mHoleSunPlanetOutside = MR::createModelObjMapObj(CP932("穴あき太陽（外側）"), "KoopaVS3HoleSunPlanet", nullptr);
    MR::setClippingTypeSphere(mHoleSunPlanetOutside, 40000.0f);
    MR::setClippingFarMax(mHoleSunPlanetOutside);
    MR::resetPosition(mHoleSunPlanetOutside, CP932("惑星Ｌｖ３"));
    MR::tryStartAllAnim(mHoleSunPlanetOutside, "KoopaVS3HoleSunPlanet");

    mHoleSunPlanetOutsideBloom = MR::createBloomModel(mHoleSunPlanetOutside, nullptr);

    mHoleSunPlanetInside = MR::createModelObjMapObj(CP932("穴あき太陽（内側）"), "KoopaVS3HoleSunInsidePlanet", nullptr);
    MR::invalidateClipping(mHoleSunPlanetInside);
    MR::resetPosition(mHoleSunPlanetInside, CP932("惑星Ｌｖ３"));
    MR::tryStartAllAnim(mHoleSunPlanetInside, "KoopaVS3HoleSunInsidePlanet");

    mHoleSunPlanetInsideBloom = MR::createBloomModel(mHoleSunPlanetInside, nullptr);
    mHoleSunPlanetInside->kill();

    MR::registerDemoSimpleCastAll(mHoleSunPlanetOutside);
    MR::registerDemoSimpleCastAll(mHoleSunPlanetOutsideBloom);
    MR::registerDemoSimpleCastAll(mHoleSunPlanetInside);
    MR::registerDemoSimpleCastAll(mHoleSunPlanetInsideBloom);

    mPeach = ::createDemoNpc(CP932("ピーチ"), "Peach");
    mKoopaJr = ::createDemoNpc(CP932("クッパＪｒ"), "KoopaJr");
    mKoopaJrShip = ::createDemoNpc(CP932("クッパＪｒ戦艦"), "KoopaJrShip");

    mMeteor1 = ::createDemoEnemy(CP932("デモ砲弾１"), "MeteorStrike");
    mMeteor2 = ::createDemoEnemy(CP932("デモ砲弾２"), "MeteorStrike");
    mMeteor3 = ::createDemoEnemy(CP932("デモ砲弾３"), "MeteorStrike");

    createCommonParts();
}

void KoopaParts::createRock() {
    if (mRock != nullptr) {
        return;
    }

    mRock = ::createKoopaBodyParts(mKoopa, CP932("クッパ岩"), "KoopaRock", "RockFixPos");
    mRock->kill();

    mRockBreak = new KoopaRockBreak(mKoopa);
    mRockBreak->initWithoutIter();
}

void KoopaParts::createRollBall() {
    if (mRollBall != nullptr) {
        return;
    }

    mRollBall = ::createKoopaBodyParts(mKoopa, CP932("回転攻撃ボール"), "KoopaRollBall", "RollBallFixPos");
    mRollBall->kill();
}

void KoopaParts::createCommonParts() {
    mThornBig = ::createKoopaBodyParts(mKoopa, CP932("尻尾のトゲ（大）"), "KoopaThorn", "TailThornBigFixPos");
    mThornSmall = ::createKoopaBodyParts(mKoopa, CP932("尻尾のトゲ（小）"), "KoopaThorn", "TailThornSmallFixPos");
    mArmorBreak = ::createKoopaBodyParts(mKoopa, CP932("壊れ甲羅"), "KoopaArmorBreak", "ArmorBreakFixPos");
    mArmorBreak->kill();

    mThornBreak = ::createKoopaBodyParts(mKoopa, CP932("トゲ破片"), "KoopaThornBreak", "ThornBreakFixPos");
    mThornBreak->kill();

    mFireShort = new LiveActorGroup(CP932("ショート炎"), ::sFireShortNum);
    mFireShort->initWithoutIter();

    for (int idx = 0; idx < ::sFireShortNum; idx++) {
        KoopaFireShort* pFireShort = new KoopaFireShort(mKoopa);
        pFireShort->initWithoutIter();
        mFireShort->registerActor(pFireShort);
    }

    mShockWave = new LiveActorGroup(CP932("衝撃波（球状）"), ::sShockWaveNum);
    mShockWave->initWithoutIter();

    for (int idx = 0; idx < ::sShockWaveNum; idx++) {
        KoopaShockWave* pShockWave = new KoopaShockWave(mKoopa);
        pShockWave->initWithoutIter();
        mShockWave->registerActor(pShockWave);
    }
}

void KoopaParts::createFireStairs(bool a1) {
    mFireStairs = new LiveActorGroup(CP932("炎（階段用）保持"), ::sFireStairsNum);
    mFireStairs->initWithoutIter();

    for (int idx = 0; idx < ::sFireStairsNum; idx++) {
        KoopaFireStairs* pFireStairs = new KoopaFireStairs(CP932("炎（階段用）"), a1);
        pFireStairs->initWithoutIter();
        mFireStairs->registerActor(pFireStairs);
    }
}
