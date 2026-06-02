#include "Game/Player/MarioAccess.hpp"

#include "Game/Animation/XanimePlayer.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/MapObj/CollectCounter.hpp"
#include "Game/MapObj/StarPiece.hpp"
#include "Game/MapObj/StarPieceDirector.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioFpView.hpp"
#include "Game/Player/MarioHang.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Player/MarioParts.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Player/RushEndInfo.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/FixedPosition.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include <revolution/mtx.h>

namespace {
    const char cRecoverOxygenEffect[] = "\x8E\x5F\x91\x66\x89\xF1\x95\x9C";
    const char cStarPieceBurstSound[] = "SE_OJ_STAR_PIECE_BURST";
    const char cDownWipeAnim[] = "\x95\x58\x8C\x8B";
    const char cBodySensor[] = "body";
    const char cDummySensor[] = "dummy";
    const char cWaterSurfaceAnim[] = "\x90\x85\x89\x6A\x83\x57\x83\x46\x83\x62\x83\x67";
    const char cInvalidMorphItemSound[] = "SE_OJ_MORPH_ITEM_INVALID";
    const char cEyeSensor[] = "eye";

    MarioActor* getMarioActor() {
        return MR::getMarioHolder()->getMarioActor();
    }

    Mario* getMario() {
        return getMarioActor()->mMario;
    }

}  // namespace

void MarioAccess::getTakePos(TVec3f* pPos) {
    if (getMarioActor()->_494 != nullptr) {
        getMarioActor()->_494->calc();
        FixedPosition* pos = getMarioActor()->_494;
        pPos->set< f32 >(pos->mMtx.mMtx[0][3], pos->mMtx.mMtx[1][3], pos->mMtx.mMtx[2][3]);
    }
}

bool MarioAccess::isOnActor(const LiveActor* pActor) {
    if (getMario()->_1C._13) {
        Triangle* triangle = getMario()->mGroundPolygon;
        if (!triangle->isValid()) {
            return false;
        }

        return triangle->mSensor->mHost == pActor;
    }

    if (getMario()->_1C._14 || (getMario()->isSwimming() && getMario()->mMovementStates._2)) {
        Triangle* triangle = getMario()->_45C;
        if (!triangle->isValid()) {
            return false;
        }

        return triangle->mSensor->mHost == pActor;
    }

    return false;
}

bool MarioAccess::isOnGround(u32 mode) {
    if (mode != 0) {
        return false;
    }

    if (getMarioActor()->_934) {
        return MR::isOnGround(getMarioActor()->_924->mHost);
    }

    return getMario()->mMovementStates._1;
}

bool MarioAccess::isHipDropFalling() {
    if (getMario()->mMovementStates.jumping) {
        if (getMario()->mMovementStates._B) {
            if (!getMario()->mMovementStates._1) {
                if (!getMarioActor()->isJumpRising()) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool MarioAccess::isHipDropLand() {
    return getMario()->mDrawStates._14;
}

bool MarioAccess::isSwingAction() {
    if (getMarioActor()->_934) {
        return false;
    }

    if (getMario()->mMovementStates._F) {
        return true;
    }

    return getMarioActor()->isPunching();
}

bool MarioAccess::isInRush() {
    bool result = true;
    if (!getMarioActor()->_934) {
        if (!getMario()->isStatusActive(0x13)) {
            result = false;
        }
    }

    return result;
}

bool MarioAccess::isSquat() {
    return getMario()->mMovementStates._A && getMario()->mMovementStates._1;
}

bool MarioAccess::isParalyzing() {
    return getMario()->isStatusActive(0xB);
}

bool MarioAccess::isTeresaDisappear() {
    if (getMarioActor()->mPlayerMode != 6) {
        return false;
    }

    return getMario()->_418 != 0;
}

bool MarioAccess::isFlying() {
    if (getMario()->isStatusActive(0x18)) {
        return true;
    }

    return getMario()->_10._21;
}

bool MarioAccess::isNeedBrakingCamera() {
    return getMario()->_1C._9;
}

CubeCameraArea* MarioAccess::getCameraCubeCode() {
    return getMario()->getCameraCubeCode();
}

bool MarioAccess::isSwimming() {
    return getMario()->isSwimming();
}

bool MarioAccess::isSkating() {
    return getMario()->isStatusActive(0x1F);
}

Triangle* MarioAccess::getGroundingPolygon(u32 mode) {
    if (getMario()->isSwimming()) {
        if (getMario()->mMovementStates._2) {
            return getMario()->_45C;
        }

        return nullptr;
    }

    if (!MarioAccess::isOnGround(0)) {
        return nullptr;
    }

    if (getMarioActor()->_934) {
        return &getMarioActor()->_924->mHost->mBinder->mGroundInfo.mParentTriangle;
    }

    return getMario()->mGroundPolygon;
}

Triangle* MarioAccess::getShadowingPolygon() {
    return getMario()->mMovementStates._2 ? getMario()->_45C : nullptr;
}

f32 MarioAccess::getShadowHeight() {
    return getMario()->mVerticalSpeed;
}

void MarioAccess::forceKill(u32 killType, u32) {
    getMarioActor()->forceKill(killType);
}

bool MarioAccess::isOnPress() {
    return getMarioActor()->_390 != 0;
}

bool MarioAccess::isDisableFpView() {
    return getMario()->isDisableFpViewMode();
}

bool MarioAccess::isFpViewChangingFailure() {
    return getMario()->_898;
}

void MarioAccess::stopFpView() {
    if (getMario()->isStatusActive(0x12)) {
        getMario()->mFpView->forceClose();
        getMario()->closeStatus(nullptr);
    }
}

void MarioAccess::noticeDashChance() {
    getMario()->_436 = 5;
}

void MarioAccess::setWalkingResist(f32 resist) {
    getMario()->_2D0 = resist;
}

void MarioAccess::forceFly(const TVec3f& rPos, const TVec3f& rFront, s32 type) {
    getMario()->doPointWarp(rPos, rFront, type);
}

void MarioAccess::setJumpVec(const TVec3f& rVec) {
    getMario()->mJumpVec = rVec;
}

void MarioAccess::forceJump(const TVec3f& rVec, u32) {
    getMario()->mMovementStates_HIGH_WORD |= 0x40000000;
    getMario()->tryForceJumpDelay(rVec);
}

void MarioAccess::freeJump(const TVec3f& rVec, u32) {
    getMario()->mMovementStates_HIGH_WORD |= 0x40000000;
    getMario()->tryFreeJumpDelay(rVec);
}

void MarioAccess::tornadoJump() {
    MarioConstTable* table = getMarioActor()->mConst->getTable();
    getMario()->_544 = table->mTornadoTimeAir;
    getMario()->tryTornadoJump();

    MarioActor* actor = getMarioActor();
    getMario()->startTornadoCentering(reinterpret_cast< HitSensor* >(actor->_928));
}

void MarioAccess::tornadoJumpMini() {
    MarioConstTable* table = getMarioActor()->mConst->getTable();
    getMario()->_544 = table->mTornadoTimeAir;
    getMario()->tryTornadoJump();

    const TVec3f* gravity = getMario()->getGravityVec();
    TVec3f booster = gravity->negateInline();
    f32 boostPower = getMarioActor()->mConst->getTable()->mTornadoBoostPower;
    booster.mult(boostPower);

    MarioConstTable* timerTable = getMarioActor()->mConst->getTable();
    MarioConstTable* attenTable = getMarioActor()->mConst->getTable();
    TVec3f* boosterPtr = &booster;
    getMario()->setRocketBooster(*boosterPtr, attenTable->mTornadoBoostAttnMini, timerTable->mTornadoBoostTimerMini);

    MarioActor* actor = getMarioActor();
    getMario()->startTornadoCentering(reinterpret_cast< HitSensor* >(actor->_928));
}

void MarioAccess::becomeNormalJumpStatus() {
    getMario()->_430 = 0;
}

void MarioAccess::setFrontVecKeepUp(const TVec3f& rVec, u16) {
    if (!getMario()->isStatusActive(5)) {
        getMario()->setFrontVecKeepUp(rVec);
        getMarioActor()->_2DC = getMario()->mFrontVec;
        getMarioActor()->_2D0 = getMario()->mHeadVec;
        getMarioActor()->_2E8 = getMario()->mSideVec;
    }
}

void MarioAccess::setFrontVecTarget(const TVec3f& rVec, u16 step) {
    MarioActor* actor = getMarioActor();
    actor->_3C4 = rVec;
    actor->_3D0 = step;
}

void MarioAccess::getThrowVec(TVec3f* pVec) {
    if (getMario()->isStatusActive(0x18)) {
        *pVec = getMario()->mHeadVec;
    }
    else if (getMario()->isSwimming()) {
        getMarioActor()->getThrowVec(pVec);
    }
    else {
        pVec->set(getMarioActor()->_F3C_vec[getMarioActor()->_F40]);
    }
}

void MarioAccess::setTrans(const TVec3f& rPos, u16) {
    getMarioActor()->mPosition.set(rPos);
    getMario()->mPosition = rPos;

    if ((getMario()->mMovementStates_HIGH_WORD & 0x00000100) != 0) {
        getMario()->_688 = rPos;
    }

    getMarioActor()->_1C0 = true;
    getMarioActor()->_2F4 = rPos;
    MR::updateHitSensorsAll(getMarioActor());
}

void MarioAccess::endRush(const RushEndInfo* pInfo) {
    getMarioActor()->endRush(pInfo);
}

void MarioAccess::incLife(u32 amount) {
    if (getMario()->isSwimming()) {
        for (u32 i = 0; i < amount; i++) {
            getMario()->mSwim->incLife();
        }
    }
    else {
        getMarioActor()->incLife(amount);
    }

    if (getMarioActor()->mPlayerMode == 4) {
        for (u32 i = 0; i < (getMarioActor()->mConst->getTable()->mAirWalkTime >> 3); i++) {
            getMario()->incAirWalkTimer();
        }
    }
}

bool MarioAccess::isConfrontDeath() {
    if (!getMarioActor()->mSuperKinokoCollected) {
        if (getMarioActor()->mHealth == 0) {
            return true;
        }

        if (getMarioActor()->mHealth == 1 && getMario()->isDamaging() && !getMario()->isSwimming()) {
            return true;
        }
    }

    if (getMario()->_735 && getMario()->isCurrentFloorSink()) {
        return true;
    }

    return !getMarioActor()->isEnableNerveChange();
}

void MarioAccess::addStarPiece() {
    MarioConstTable* fogTable = getMarioActor()->mConst->getTable();
    f32 fogLevel = fogTable->mStarPieceFogLevel;
    MarioConstTable* timeTable = getMarioActor()->mConst->getTable();
    u8 fogTime = timeTable->mStarPieceFogTime;

    MarioActor* actor = getMarioActor();
    actor->_1AA = fogTime;
    actor->_1AC = fogLevel;
    actor->_1B0.set(0xFF, 0xFF, 0xFF, 0);
    actor->_1B5 = false;
}

void MarioAccess::getStarPieceDirect() {
}

MtxPtr MarioAccess::getJointMtx(const char* pName) {
    return getMarioActor()->getGlobalJointMtx(pName);
}

TVec3f* MarioAccess::getVelocity() {
    if (getMarioActor()->_934) {
        return const_cast< TVec3f* >(&getMarioActor()->getLastMove());
    }

    return &getMario()->mVelocity;
}

TVec3f* MarioAccess::getLastMove() {
    return const_cast< TVec3f* >(&getMarioActor()->getLastMove());
}

void MarioAccess::hide() {
    getMarioActor()->_482 = true;
    MR::forceDeleteEffectAll(getMarioActor());
    getMarioActor()->updateHand();
    getMarioActor()->updateFace();
}

void MarioAccess::show() {
    getMarioActor()->_482 = false;
}

HitSensor* MarioAccess::getTakingSensor() {
    if (getMarioActor()->_428[0] != nullptr) {
        return getMarioActor()->_428[0];
    }

    if (getMarioActor()->_424 != nullptr) {
        return getMarioActor()->_424;
    }

    return nullptr;
}

void MarioAccess::dropTakingActor() {
    getMarioActor()->rushDropThrowMemoSensor();
}

void MarioAccess::killTakingActor() {
    getMarioActor()->damageDropThrowMemoSensor();
}

f32 MarioAccess::getAnimationFrameMax() {
    return getMarioActor()->mMarioAnim->mXanimePlayer->_20->mEnd;
}

void MarioAccess::changeAnimationJ(const char* pName) {
    getMarioActor()->changeAnimationNonStop(pName);
}

void MarioAccess::changeAnimationE(const char* pName, s32 interpole) {
    getMarioActor();
    if (getMarioActor()->_B91) {
        return;
    }

    if (getMario()->isPlayerModeTeresa()) {
        getMarioActor()->changeTeresaAnimation(pName, interpole);
        return;
    }

    if (getMarioActor()->_468 == 0) {
        getMario()->stopAnimationUpperForce();
    }

    MR::startBck(getMarioActor(), pName, nullptr);
    getMarioActor();
    if (interpole >= 0) {
        getMarioActor()->mMarioAnim->mXanimePlayer->changeInterpoleFrame(interpole);
    }

    getMarioActor()->setBlink(pName);
    getMarioActor()->mMarioAnim->closeCallback();
    getMarioActor()->mMarioAnim->entryCallback(pName);
}

void MarioAccess::changeAnimationE(const char* pName, const char* pInterpoleName) {
    getMarioActor();
    if (getMarioActor()->_B91) {
        return;
    }

    if (getMarioActor()->_468 == 0) {
        getMario()->stopAnimationUpperForce();
    }

    MR::startBck(getMarioActor(), pName, pInterpoleName);
    getMarioActor()->setBlink(pName);
    getMarioActor()->mMarioAnim->closeCallback();
    getMarioActor()->mMarioAnim->entryCallback(pName);
}

void MarioAccess::changeAnimationE(const char* pName, const BckCtrlData& rCtrl) {
    getMarioActor();
    if (getMarioActor()->_B91) {
        return;
    }

    if (getMarioActor()->_468 == 0) {
        getMario()->stopAnimationUpperForce();
    }

    MR::startBck(getMarioActor(), pName, nullptr);
    MR::reflectBckCtrlData(getMarioActor(), rCtrl);
    getMarioActor()->setBlink(pName);
    getMarioActor()->mMarioAnim->closeCallback();
    getMarioActor()->mMarioAnim->entryCallback(pName);
}

void MarioAccess::keepCurrentAnimation() {
    XanimeFrameCtrl* frameCtrl = getMarioActor()->mMarioAnim->mXanimePlayer->_20;
    if (frameCtrl->mAttribute == 0) {
        getMarioActor()->mMarioAnim->mXanimePlayer->_20->mAttribute = 1;
    }
}

void MarioAccess::progressAnimation() {
    getMarioActor()->mMarioAnim->update();
    getMarioActor()->calcAnimInMovement();

    XanimePlayer* player = getMarioActor()->mMarioAnim->mXanimePlayer;
    player->updateBeforeMovement();
    player->updateAfterMovement();
}

const char* MarioAccess::getCurrentBckName() {
    return getMario()->getCurrentBckName();
}

void MarioAccess::setAnimationBlendWeight(const f32* weights) {
    getMarioActor()->mMarioAnim->forceSetBlendWeight(weights);
}

void MarioAccess::setSpot(f32 spot, u32 color) {
}

void MarioAccess::offControl() {
    getMarioActor()->_3C0 = true;
}

bool MarioAccess::isOffControl() {
    return getMarioActor()->_3C0;
}

void MarioAccess::onControl(bool requestMovement) {
    getMarioActor()->_3C0 = false;
    if (requestMovement) {
        getMarioActor()->resetCondition();
    }
}

void MarioAccess::setStateWait() {
    getMario()->stopJump();
    getMario()->stopWalk();
}

void MarioAccess::startTalk(const LiveActor* pActor) {
    getMario()->startTalk(pActor);
    getMarioActor()->stopSpinTicoEffect(false);
}

void MarioAccess::endTalk() {
    getMario()->endTalk();
}

void MarioAccess::readyRemoteDemo() {
    if (getMarioActor()->_EA4) {
        return;
    }

    if (getMario()->isStatusActive(0x13) || getMarioActor()->mPlayerMode == 5) {
        getMarioActor()->_3C0 = true;
        getMario()->_10_HIGH_WORD |= 0x80000000;
        return;
    }

    getMario()->_10_HIGH_WORD &= 0x7FFFFFFF;

    if (getMarioActor()->_934) {
        getMarioActor()->_924->receiveMessage(0x93, getMarioActor()->getSensor(cBodySensor));
        if (getMarioActor()->_934) {
            RushEndInfo info(nullptr, 4, TVec3f(0.0f, 0.0f, 0.0f), false, 0);
            getMarioActor()->endRush(&info);
        }
    }

    if (getMario()->isStatusActive(0x12)) {
        getMario()->closeStatus(nullptr);
    }

    getMarioActor()->flushCoinPull();
    getMarioActor()->calcAndSetBaseMtx();
    getMarioActor()->_3C0 = true;
    getMarioActor()->_EA4 = true;
    MR::invalidateHitSensors(getMarioActor());
    PSMTXCopy(getMarioActor()->getBaseMtx(), getMarioActor()->_EA8);
    getMarioActor()->_B90 = true;
    getMario()->stopWalk();
    MR::deleteEffectAll(getMarioActor());
    getMarioActor()->_1B8->kill();
    getMarioActor()->_1E4 = 0.0f;
    getMarioActor()->_ED8 = getMarioActor()->mPosition;
    getMarioActor()->_EE4 = reinterpret_cast< u32 >(getMarioActor()->mMarioAnim->mXanimePlayer->getCurrentAnimationName());
    getMarioActor()->_EA6 = false;
}

void MarioAccess::onFollowDemo() {
    if (getMarioActor()->mPlayerMode == 6) {
        MR::requestMovementOn(getMarioActor()->_9A4);
    }
}

void MarioAccess::setBaseMtx(MtxPtr mtx) {
    getMarioActor()->forceSetBaseMtx(mtx);
}

MtxPtr MarioAccess::getBaseMtx() {
    if (getMarioActor()->_EA5) {
        return getMarioActor()->_EA8;
    }

    return getMarioActor()->getBaseMtx();
}

bool MarioAccess::calcSpinPullVelocity(TVec3f* pVelocity, const TVec3f& rPos) {
    getMarioActor()->tryPullTrans(pVelocity, rPos);
    return true;
}

void MarioAccess::tryCoinPull() {
    HitSensor* sensor = getMarioActor()->getSensor(cEyeSensor);
    bool isValid = false;
    if (sensor->mValidByHost && sensor->mValidBySystem) {
        isValid = true;
    }

    if (!isValid) {
        getMarioActor()->getSensor(cEyeSensor)->validate();
    }

    getMarioActor()->_6D0 = 1;
}

void MarioAccess::addVelocity(const TVec3f& rVelocity) {
    if (getMario()->isSwimming() && getMario()->mSwim->mJetTimer != 0) {
        return;
    }

    if (getMarioActor()->_EA4 || getMarioActor()->_934) {
        return;
    }

    getMario()->push(rVelocity);
    if (getMario()->isStatusActive(5)) {
        getMario()->mHang->forceDrop();
    }
}

void MarioAccess::addVelocityFromArea(const TVec3f& rVelocity) {
    if (getMario()->isSwimming() && getMario()->mSwim->mJetTimer != 0) {
        return;
    }

    if (getMarioActor()->_EA4 || getMarioActor()->_934) {
        return;
    }

    getMario()->push(rVelocity);
}

bool MarioAccess::isOnWaterSurface() {
    bool result;
    if (getMario()->isStatusActive(6)) {
        MarioSwim* swim = getMario()->mSwim;
        result = false;
        if (swim->mIsOnSurface || swim->mIsSwimmingAtSurface) {
            result = true;
        }
    }
    else {
        result = getMarioActor()->isAnimationRun(cWaterSurfaceAnim);
    }

    return result;
}

void MarioAccess::calcWorldPadDir(TVec3f* pDir, f32 x, f32 y) {
    getMario()->calcWorldPadDir(pDir, x, y, false);
}

void MarioAccess::preventRush() {
    getMarioActor()->setNerve(&NrvMarioActor::MarioActorNrvNoRush::sInstance);
}

LiveActor* MarioAccess::getPlayerActor() {
    return getMarioActor();
}

void MarioAccess::validateSensor() {
    getMarioActor()->getSensor(cEyeSensor)->validate();
}

void MarioAccess::incOxygen(u32 amount) {
    if (getMario()->isSwimming()) {
        for (u32 i = 0; i < amount; i++) {
            getMario()->mSwim->incOxygen();
        }

        getMarioActor()->playEffect(cRecoverOxygenEffect);
    }
}

void MarioAccess::scatterStarPiece(u32 amount) {
    if (MR::getStarPieceNum() > 0) {
        MR::startSound(getMarioActor(), cStarPieceBurstSound, -1, -1);
    }

    for (u32 i = 0; i < amount; i++) {
        if (MR::getStarPieceNum() == 0) {
            return;
        }

        StarPiece* piece = MR::getDeadStarPiece();
        piece->launch(getMarioActor()->_2A0, 24.0f, 24.0f, false, false);
        MR::addStarPiece(-1);
    }
}

void MarioAccess::startDownWipe() {
    if (!getMarioActor()->isNerve(&NrvMarioActor::MarioActorNrvGameOverSink::sInstance)
        && !getMarioActor()->isAnimationRun(cDownWipeAnim)
        && getMarioActor()->_390 == 0) {
        getMarioActor()->_A61 = true;
    }
}

void MarioAccess::readyDemo() {
    if (getMarioActor()->_934) {
        if (!getMarioActor()->_924->receiveMessage(0x93, getMarioActor()->getSensor(cBodySensor))) {
            getMarioActor()->_924->receiveMessage(0x94, getMarioActor()->getSensor(cBodySensor));
        }

        if (getMarioActor()->_934) {
            RushEndInfo info(nullptr, 4, TVec3f(0.0f, 0.0f, 0.0f), false, 0);
            getMarioActor()->endRush(&info);
        }
    }

    if (getMario()->isStatusActive(0x12)) {
        getMario()->closeStatus(nullptr);
    }
}

void MarioAccess::endRemoteDemo(const RushEndInfo* pInfo) {
    if (getMario()->isStatusActive(0x13) || getMario()->isStatusActive(0x22)
        || (getMario()->_10_HIGH_WORD & 0x80000000) != 0 || !getMarioActor()->_EA4) {
        getMarioActor()->_3C0 = false;
        return;
    }

    getMarioActor()->_B90 = false;
    MarioAccess::onControl(true);
    getMarioActor()->_EA4 = false;
    getMarioActor()->stopAnimation(nullptr);

    if (getMarioActor()->_468 != 0) {
        HitSensor* sensor = nullptr;
        if (getMarioActor()->_468 != 0) {
            sensor = getMarioActor()->_428[0];
        }

        getMarioActor()->mMarioAnim->updateTakingAnimation(sensor);
    }

    MR::validateHitSensors(getMarioActor());
    getMarioActor()->getSensor(cDummySensor)->invalidate();
    getMarioActor()->_EA6 = false;
}

bool MarioAccess::isInWaterMode() {
    if (getMario()->isStatusActive(6)) {
        return true;
    }

    return getMarioActor()->isAnimationRun(cWaterSurfaceAnim);
}

void MarioAccess::changeItemStatus(s32 status) {
    switch (status) {
    case 0:
    case 8:
        getMarioActor()->setPlayerMode(0, true);
        break;
    case 1:
        getMarioActor()->setPlayerMode(5, true);
        break;
    case 2:
        getMarioActor()->setPlayerMode(4, true);
        break;
    case 3:
        getMarioActor()->setPlayerMode(6, true);
        break;
    case 4:
        getMarioActor()->setPlayerMode(3, true);
        break;
    case 5:
        getMarioActor()->setPlayerMode(2, true);
        break;
    case 6:
        getMarioActor()->setPlayerMode(7, true);
        break;
    case 7:
        getMarioActor()->setPlayerMode(1, true);
        break;
    case 9:
        getMario()->mMovementStates_LOW_WORD |= 0x00010000;
        getMario()->_544 = 3;
        break;
    case 10:
        if (getMarioActor()->isEnableNerveChange()) {
            if (getMarioActor()->mMaxHealth > 3) {
                MR::startSound(getMarioActor(), cInvalidMorphItemSound, -1, -1);
                getMarioActor()->changeMaxLife(6);
            }
            else {
                getMarioActor()->mSuperKinokoCollected = true;
            }
        }
        break;
    }
}
