#include "Game/Player/MarioAccess.hpp"

#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"

namespace {
    MarioActor* getMarioActor() {
        return MR::getMarioHolder()->getMarioActor();
    }

    Mario* getMario() {
        return getMarioActor()->mMario;
    }
};  // namespace

void MarioAccess::getTakePos(TVec3f* pPos) {
    MR::getPlayerTakePos(pPos);
}

bool MarioAccess::isOnActor(const LiveActor* pActor) {
    return MR::isActorOnPlayer(pActor);
}

bool MarioAccess::isOnGround(u32 mode) {
    return mode == 0 && MR::isOnGroundPlayer();
}

bool MarioAccess::isHipDropFalling() {
    return MR::isPlayerHipDropFalling();
}

bool MarioAccess::isHipDropLand() {
    return MR::isPlayerHipDropLand();
}

bool MarioAccess::isSwingAction() {
    return MR::isPlayerSwingAction();
}

bool MarioAccess::isInRush() {
    return MR::isPlayerInRush();
}

bool MarioAccess::isSquat() {
    return MR::isPlayerSquat();
}

bool MarioAccess::isParalyzing() {
    return MR::isPlayerParalyzing();
}

bool MarioAccess::isTeresaDisappear() {
    return MR::isPlayerTeresaDisappear();
}

bool MarioAccess::isFlying() {
    return MR::isPlayerFlying();
}

bool MarioAccess::isNeedBrakingCamera() {
    return MR::isPlayerNeedBrakingCamera();
}

CubeCameraArea* MarioAccess::getCameraCubeCode() {
    return getMario()->getCameraCubeCode();
}

bool MarioAccess::isSwimming() {
    return MR::isPlayerSwimming();
}

bool MarioAccess::isSkating() {
    return MR::isPlayerSkating();
}

Triangle* MarioAccess::getGroundingPolygon(u32 mode) {
    return mode == 0 ? MR::getPlayerGroundingPolygon() : nullptr;
}

Triangle* MarioAccess::getShadowingPolygon() {
    return getMario()->mMovementStates._2 ? getMario()->_45C : nullptr;
}

f32 MarioAccess::getShadowHeight() {
    return MR::getPlayerShadowHeight();
}

void MarioAccess::forceKill(u32 killType, u32) {
    getMarioActor()->forceKill(killType);
}

bool MarioAccess::isOnPress() {
    return MR::isPlayerOnPress();
}

bool MarioAccess::isDisableFpView() {
    return MR::isPlayerDisableFpView();
}

bool MarioAccess::isFpViewChangingFailure() {
    return MR::isFpViewChangingFailure();
}

void MarioAccess::stopFpView() {
    MR::stopPlayerFpView();
}

void MarioAccess::noticeDashChance() {
    MR::noticePlayerDashChance();
}

void MarioAccess::setWalkingResist(f32 resist) {
    MR::setPlayerWalkingResist(resist);
}

void MarioAccess::forceFly(const TVec3f& rPos, const TVec3f& rFront, s32 type) {
    getMario()->doPointWarp(rPos, rFront, type);
}

void MarioAccess::setJumpVec(const TVec3f& rVec) {
    MR::setPlayerJumpVec(rVec);
}

void MarioAccess::forceJump(const TVec3f& rVec, u32) {
    MR::forceJumpPlayer(rVec);
}

void MarioAccess::freeJump(const TVec3f& rVec, u32) {
    MR::jumpPlayer(rVec);
}

void MarioAccess::tornadoJump() {
    MR::tornadoJumpPlayer();
}

void MarioAccess::tornadoJumpMini() {
    MR::miniTornadoJumpPlayer();
}

void MarioAccess::becomeNormalJumpStatus() {
    MR::becomePlayerNormalJumpStatus();
}

void MarioAccess::setFrontVecKeepUp(const TVec3f& rVec, u16) {
    MR::setPlayerFrontVec(rVec, 0);
}

void MarioAccess::setFrontVecTarget(const TVec3f& rVec, u16 step) {
    MR::setPlayerFrontTargetVec(rVec, step);
}

void MarioAccess::getThrowVec(TVec3f* pVec) {
    MR::getPlayerThrowVec(pVec);
}

void MarioAccess::setTrans(const TVec3f& rPos, u16) {
    MR::setPlayerPos(rPos);
    MR::updateHitSensorsAll(getMarioActor());
}

void MarioAccess::endRush(const RushEndInfo* pInfo) {
    getMarioActor()->endRush(pInfo);
}

void MarioAccess::incLife(u32 amount) {
    MR::incPlayerLife(amount);
}

bool MarioAccess::isConfrontDeath() {
    return MR::isPlayerConfrontDeath();
}

void MarioAccess::addStarPiece() {
    MR::getStarPiecePlayer();
}

void MarioAccess::getStarPieceDirect() {
}

MtxPtr MarioAccess::getJointMtx(const char* pName) {
    return getMarioActor()->getGlobalJointMtx(pName);
}

TVec3f* MarioAccess::getVelocity() {
    return MR::getPlayerVelocity();
}

TVec3f* MarioAccess::getLastMove() {
    return MR::getPlayerLastMove();
}

void MarioAccess::hide() {
    MR::hidePlayer();
}

void MarioAccess::show() {
    MR::showPlayer();
}

HitSensor* MarioAccess::getTakingSensor() {
    HitSensor* sensor = getMarioActor()->_428[0];
    return sensor != nullptr ? sensor : getMarioActor()->_424;
}

void MarioAccess::dropTakingActor() {
    MR::tryPlayerDropTakingActor();
}

void MarioAccess::killTakingActor() {
    MR::tryPlayerKillTakingActor();
}

f32 MarioAccess::getAnimationFrameMax() {
    return MR::getBckFrameMaxPlayer();
}

void MarioAccess::changeAnimationJ(const char* pName) {
    MR::startBckPlayerJ(pName);
}

void MarioAccess::changeAnimationE(const char* pName, s32 interpole) {
    MR::startBckPlayer(pName, interpole);
}

void MarioAccess::changeAnimationE(const char* pName, const char* pInterpoleName) {
    MR::startBckPlayer(pName, pInterpoleName);
}

void MarioAccess::changeAnimationE(const char* pName, const BckCtrlData& rCtrl) {
    MR::startBckPlayer(pName, rCtrl);
}

void MarioAccess::keepCurrentAnimation() {
    MR::becomeContinuousBckPlayer();
}

void MarioAccess::progressAnimation() {
    MR::progressPlayerBckOnPause();
}

const char* MarioAccess::getCurrentBckName() {
    return MR::getPlayerCurrentBckName();
}

void MarioAccess::setAnimationBlendWeight(const f32* weights) {
    MR::setBckBlendWeight(weights[0], weights[1], weights[2], weights[3]);
}

void MarioAccess::setSpot(f32 spot, u32 color) {
    MR::setPlayerSpot(spot, color);
}

void MarioAccess::offControl() {
    MR::offPlayerControl();
}

bool MarioAccess::isOffControl() {
    return MR::isOffPlayerControl();
}

void MarioAccess::onControl(bool requestMovement) {
    MR::onPlayerControl(requestMovement);
}

void MarioAccess::setStateWait() {
    MR::setPlayerStateWait();
}

void MarioAccess::startTalk(const LiveActor* pActor) {
    MR::startPlayerTalk(pActor);
}

void MarioAccess::endTalk() {
    MR::endPlayerTalk();
}

void MarioAccess::readyRemoteDemo() {
    MR::readyPlayerDemo();
}

void MarioAccess::onFollowDemo() {
    MR::onFollowDemoEffect();
}

void MarioAccess::setBaseMtx(MtxPtr mtx) {
    MR::setPlayerBaseMtx(mtx);
}

MtxPtr MarioAccess::getBaseMtx() {
    return MR::getPlayerBaseMtx();
}

void MarioAccess::calcSpinPullVelocity(TVec3f* pVelocity, const TVec3f& rPos) {
    MR::calcPlayerSpinPullVelocity(pVelocity, rPos);
}

void MarioAccess::tryCoinPull() {
    MR::tryPlayerCoinPull();
}

void MarioAccess::addVelocity(const TVec3f& rVelocity) {
    MR::pushPlayer(rVelocity);
}

void MarioAccess::addVelocityFromArea(const TVec3f& rVelocity) {
    MR::pushPlayerFromArea(rVelocity);
}

bool MarioAccess::isOnWaterSurface() {
    return MR::isPlayerOnWaterSurface();
}

void MarioAccess::calcWorldPadDir(TVec3f* pDir, f32 x, f32 y) {
    MR::calcPlayerWorldPadDir(pDir, x, y);
}

void MarioAccess::preventRush() {
    MR::preventPlayerRush();
}

LiveActor* MarioAccess::getPlayerActor() {
    return MR::getPlayerDemoActor();
}

void MarioAccess::validateSensor() {
    MR::validatePlayerSensor();
}

void MarioAccess::incOxygen(u32 amount) {
    MR::incPlayerOxygen(amount);
}

void MarioAccess::scatterStarPiece(u32 amount) {
    MR::scatterStarPiecePlayer(amount);
}

void MarioAccess::startDownWipe() {
    MR::startPlayerDownWipe();
}

void MarioAccess::readyDemo() {
    MR::readyPlayerDemo();
}

void MarioAccess::endRemoteDemo(const RushEndInfo* pInfo) {
    endRush(pInfo);
    MR::onPlayerControl(true);
}

bool MarioAccess::isInWaterMode() {
    return MR::isPlayerInWaterMode();
}

void MarioAccess::changeItemStatus(s32 status) {
    MR::changePlayerItemStatus(status);
}
