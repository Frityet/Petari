#include "Game/Util/PlayerUtil.hpp"

#include "Game/Camera/CameraTargetArg.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/MapObj/StarPieceDirector.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/RushEndInfo.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include <cstring>
#include <revolution/mtx.h>

class EventSequencer {
public:
    void startEvent(const char*);
};

namespace {
    const char cHandSensor[] = "\x8E\xE3";

    struct RushFlagBits {
        u32 _0 : 4;
        u32 mDamageType : 4;
        u32 _8 : 24;
    };

    MarioActor* getMarioActor() {
        return static_cast< MarioActor* >(MarioAccess::getPlayerActor());
    }

    void resetPlayerConditionAndWait() {
        getMarioActor()->resetCondition();
        MarioAccess::setStateWait();
    }

    void setRushEndFlags(RushEndInfo* pInfo, u32 flags) {
        pInfo->mFlags |= flags;
    }

    void endBind(LiveActor* pActor, u32 type, const TVec3f& rVec, bool useVec, u32 timer, u32 flags) {
        RushEndInfo info(pActor, type, rVec, useVec, timer);
        setRushEndFlags(&info, flags);
        MarioAccess::endRush(&info);
    }

}  // namespace

namespace MR {
    bool isOnGroundPlayer() {
        return MarioAccess::isOnGround(0);
    }

    Triangle* getPlayerGroundingPolygon() {
        return MarioAccess::getGroundingPolygon(0);
    }

    void forceKillPlayerByAbyss() {
        MarioAccess::forceKill(0, 0);
    }

    void forceKillPlayerByWaterRace() {
        MarioAccess::forceKill(1, 0);
    }

    void forceKillPlayerByGroundRace() {
        MarioAccess::forceKill(5, 0);
    }

    void forceKillPlayerByGhostRace() {
        MarioAccess::forceKill(2, 0);
    }

    bool isPlayerDead() {
        return !getMarioActor()->isEnableNerveChange();
    }

    bool isPlayerRefuseTalk() {
        return getMarioActor()->isRefuseTalk();
    }

    bool isPlayerTeresaDisappear() {
        return MarioAccess::isTeresaDisappear();
    }

    bool isPlayerInAreaObj(const char* pName) {
        return MR::isInAreaObj(pName, getMarioActor()->mPosition);
    }

    TVec3f* getPlayerPos() {
        return &getMarioActor()->mPosition;
    }

    TVec3f* getPlayerCenterPos() {
        return &getMarioActor()->_2A0;
    }

    void getPlayerTakePos(TVec3f* pPos) {
        MarioAccess::getTakePos(pPos);
    }

    void setPlayerPos(const TVec3f& rPos) {
        MarioAccess::setTrans(rPos, 0);
    }

    void setPlayerUpperRotateY(f32 y) {
        getMarioActor()->setUpperRotateY(y);
    }

    TVec3f* getPlayerRotate() {
        return &getMarioActor()->mRotation;
    }

    TVec3f* getPlayerShadowRotate() {
        return &getMarioActor()->_A18;
    }

    TVec3f* getPlayerVelocity() {
        return MarioAccess::getVelocity();
    }

    TVec3f* getPlayerLastMove() {
        return MarioAccess::getLastMove();
    }

    void setPlayerJumpVec(const TVec3f& rVec) {
        MarioAccess::setJumpVec(rVec);
    }

    f32 getPlayerHitRadius() {
        return getMarioActor()->mPlayerMode == 6 ? 100.0f : 60.0f;
    }

    void setPlayerWalkingResist(f32 resist) {
        MarioAccess::setWalkingResist(resist);
    }

    TVec3f* getPlayerGravity() {
        return &getMarioActor()->getGravityVec();
    }

    void calcPlayerSpinPullVelocity(TVec3f* pVelocity, const TVec3f& rPos) {
        MarioAccess::calcSpinPullVelocity(pVelocity, rPos);
    }

    bool checkPlayerActionTrigger() {
        return getMarioActor()->_EED;
    }

    bool checkPlayerSwingTrigger() {
        return getMarioActor()->_1E1;
    }

    f32 calcDistanceToPlayer(const TVec3f& rPos) {
        return PSVECDistance(&getMarioActor()->mPosition, &rPos);
    }

    void getPlayerUpVec(TVec3f* pVec) {
        getMarioActor()->getUpVec(pVec);
    }

    void getPlayerFrontVec(TVec3f* pVec) {
        getMarioActor()->getFrontVec(pVec);
    }

    void getPlayerSideVec(TVec3f* pVec) {
        getMarioActor()->getSideVec(pVec);
    }

    void getPlayerThrowVec(TVec3f* pVec) {
        MarioAccess::getThrowVec(pVec);
    }

    void getPlayerGroundPos(TVec3f* pVec) {
        getMarioActor()->getGroundPos(pVec);
    }

    TVec3f* getPlayerGroundNormal() {
        return const_cast< TVec3f* >(MarioAccess::getGroundingPolygon(0)->getFaceNormal());
    }

    void setPlayerFrontTargetVec(const TVec3f& rVec, s32 step) {
        MarioAccess::setFrontVecTarget(rVec, static_cast< u16 >(step));
    }

    void setPlayerFrontVec(const TVec3f& rVec, s32 step) {
        MarioAccess::setFrontVecKeepUp(rVec, static_cast< u16 >(step));
    }

    void setPlayerSwingInhibitTimer(u16 timer) {
        getMarioActor()->_EF6 = timer;
    }

    void setPlayerSwingPermission(bool permission) {
        getMarioActor()->_EEB = permission;
    }

    void setPlayerStateWait() {
        MarioAccess::setStateWait();
    }

    void startBckPlayer(const char* pName, const char* pInterpoleName) {
        MarioAccess::changeAnimationE(pName, pInterpoleName != nullptr ? pInterpoleName : pName);
    }

    void startBckPlayer(const char* pName, const BckCtrlData& rCtrl) {
        MarioAccess::changeAnimationE(pName, rCtrl);
    }

    void startBckPlayer(const char* pName, s32 interpole) {
        MarioAccess::changeAnimationE(pName, interpole);
    }

    bool isBckStoppedPlayer() {
        return MR::isBckStopped(getMarioActor());
    }

    bool isBckOneTimeAndStoppedPlayer() {
        return MR::isBckOneTimeAndStopped(getMarioActor());
    }

    f32 getBckFrameMaxPlayer() {
        return MarioAccess::getAnimationFrameMax();
    }

    s16 getBckFrameMaxPlayer(const char* pName) {
        return MR::getBckFrameMax(getMarioActor(), pName);
    }

    void startBckPlayerJ(const char* pName) {
        MarioAccess::changeAnimationJ(pName);
    }

    void becomeContinuousBckPlayer() {
        MarioAccess::keepCurrentAnimation();
    }

    void progressPlayerBckOnPause() {
        MarioAccess::progressAnimation();
    }

    const char* getPlayerCurrentBckName() {
        return MarioAccess::getCurrentBckName();
    }

    void setBckBlendWeight(f32 w1, f32 w2) {
        f32 weights[2] = { w1, w2 };
        MarioAccess::setAnimationBlendWeight(weights);
    }

    void setBckBlendWeight(f32 w1, f32 w2, f32 w3) {
        f32 weights[3] = { w1, w2, w3 };
        MarioAccess::setAnimationBlendWeight(weights);
    }

    void setBckBlendWeight(f32 w1, f32 w2, f32 w3, f32 w4) {
        f32 weights[4] = { w1, w2, w3, w4 };
        MarioAccess::setAnimationBlendWeight(weights);
    }

    void setBckRatePlayer(f32 rate) {
        MR::setBckRate(getMarioActor(), rate);
    }

    XanimeResourceTable* getPlayerXanimeResource() {
        return getMarioActor()->getResourceTable();
    }

    void jumpPlayer(const TVec3f& rVec) {
        MarioAccess::freeJump(rVec, 0);
    }

    void forceJumpPlayer(const TVec3f& rVec) {
        MarioAccess::forceJump(rVec, 0);
    }

    void forceFlyPlayer(const TVec3f& rPos, const TVec3f& rFront, s32 type) {
        MarioAccess::forceFly(rPos, rFront, type);
    }

    void tornadoJumpPlayer() {
        MarioAccess::tornadoJump();
    }

    void miniTornadoJumpPlayer() {
        MarioAccess::tornadoJumpMini();
    }

    void becomePlayerNormalJumpStatus() {
        MarioAccess::becomeNormalJumpStatus();
    }

    bool isOnPlayer(const HitSensor* pSensor) {
        Triangle* triangle = MarioAccess::getGroundingPolygon(0);
        if (triangle != nullptr) {
            return triangle->mSensor == pSensor;
        }

        return false;
    }

    bool isActorOnPlayer(const LiveActor* pActor) {
        return MarioAccess::isOnActor(pActor);
    }

    bool isOnPlayerShadow(const LiveActor* pActor) {
        Triangle* triangle = MarioAccess::getShadowingPolygon();
        if (triangle != nullptr) {
            return triangle->mSensor->mHost == pActor;
        }

        return false;
    }

    f32 getPlayerShadowHeight() {
        return MarioAccess::getShadowHeight();
    }

    void setPlayerPos(const char* pName) {
        TPos3f mtx;
        mtx.identity();
        MR::findNamePos(pName, mtx.toMtxPtr());
        MarioAccess::setBaseMtx(mtx.toMtxPtr());

        TVec3f pos;
        MR::extractMtxTrans(mtx.toMtxPtr(), &pos);
        MarioAccess::setTrans(pos, 0);
    }

    void setPlayerPosAndWait(const TVec3f& rPos) {
        MarioAccess::setTrans(rPos, 0);
        resetPlayerConditionAndWait();
    }

    void setPlayerPosAndWait(const char* pName) {
        MR::setPlayerPos(pName);
        getMarioActor()->resetCondition();
        MarioAccess::setStateWait();
    }

    void setPlayerLinkPosAndWait(const NameObj* pObj, const char* pName) {
        TPos3f mtx;
        mtx.identity();
        MR::findLinkNamePos(pObj, pName, mtx.toMtxPtr());
        MarioAccess::setBaseMtx(mtx.toMtxPtr());
        resetPlayerConditionAndWait();
    }

    void setPlayerPosOnGround(const char* pName) {
        TPos3f mtx;
        mtx.identity();
        MR::findNamePosOnGround(pName, mtx.toMtxPtr());
        MarioAccess::setBaseMtx(mtx.toMtxPtr());
    }

    void setPlayerPosOnGroundAndWait(const char* pName) {
        TPos3f mtx;
        mtx.identity();
        MR::findNamePosOnGround(pName, mtx.toMtxPtr());
        MarioAccess::setBaseMtx(mtx.toMtxPtr());
        resetPlayerConditionAndWait();
    }

    bool isPlayerHipDropFalling() {
        return MarioAccess::isHipDropFalling();
    }

    bool isPlayerHipDropLand() {
        return MarioAccess::isHipDropLand();
    }

    void incPlayerLife(u32 amount) {
        MarioAccess::incLife(amount);
    }

    void incPlayerOxygen(u32 amount) {
        MarioAccess::incOxygen(amount);
    }

    bool isPlayerConfrontDeath() {
        return MarioAccess::isConfrontDeath();
    }

    void getStarPiecePlayer() {
        MarioAccess::addStarPiece();
    }

    void notifyDirectGetStarPiecePlayer() {
        MarioAccess::getStarPieceDirect();
    }

    void scatterStarPiecePlayer(u32 amount) {
        MarioAccess::scatterStarPiece(amount);
    }

    bool isPlayerSwingAction() {
        return MarioAccess::isSwingAction();
    }

    bool isPlayerPointedBy2POnTriggerButton() {
        return MR::isStarPointerPointing2POnTriggerButton(getMarioActor(), cHandSensor, true, false);
    }

    bool isPlayerSquat() {
        return MarioAccess::isSquat();
    }

    bool isPlayerInRush() {
        return MarioAccess::isInRush();
    }

    bool isPlayerNeedBrakingCamera() {
        return MarioAccess::isNeedBrakingCamera();
    }

    bool isPlayerFlying() {
        return MarioAccess::isFlying();
    }

    bool isPlayerElementMode(s32 mode) {
        return getMarioActor()->mPlayerMode == mode;
    }

    bool isPlayerElementModeTornado() {
        return getMarioActor()->mPlayerMode == 9;
    }

    bool isPlayerElementModeInvincible() {
        return getMarioActor()->mPlayerMode == 1;
    }

    bool isPlayerElementModeBee() {
        return getMarioActor()->mPlayerMode == 4;
    }

    bool isPlayerElementModeHopper() {
        return getMarioActor()->mPlayerMode == 5;
    }

    bool isPlayerElementModeTeresa() {
        return getMarioActor()->mPlayerMode == 6;
    }

    bool isPlayerElementModeIce() {
        return getMarioActor()->mPlayerMode == 3;
    }

    bool isPlayerElementModeNormal() {
        return getMarioActor()->mPlayerMode == 0;
    }

    bool isPlayerSkating() {
        return MarioAccess::isSkating();
    }

    void changePlayerItemStatus(s32 status) {
        MarioAccess::changeItemStatus(status);
    }

    void curePlayerElementMode() {
        MarioAccess::changeItemStatus(8);
    }

    bool isPlayerParalyzing() {
        return MarioAccess::isParalyzing();
    }

    bool isPlayerDamaging() {
        return getMarioActor()->isDamaging();
    }

    bool isPlayerStaggering() {
        return getMarioActor()->isStaggering();
    }

    bool isPlayerSwimming() {
        return MarioAccess::isSwimming();
    }

    bool isPlayerSleeping() {
        return getMarioActor()->isSleeping();
    }

    bool isPlayerJumpRising() {
        return getMarioActor()->isJumpRising();
    }

    void validatePlayerSensor() {
        MarioAccess::validateSensor();
    }

    bool isPlayerInBind() {
        return MarioAccess::isInRush();
    }

    void endBindAndPlayerWait(LiveActor* pActor) {
        TVec3f vec(0.0f, 0.0f, 0.0f);
        RushEndInfo info(pActor, 0, vec, false, 0);
        MarioAccess::endRush(&info);
    }

    void endBindAndPlayerJump(LiveActor* pActor, const TVec3f& rVec, u32 timer) {
        endBind(pActor, 2, rVec, true, timer, 0);
    }

    void endBindAndPlayerForceJump(LiveActor* pActor, const TVec3f& rVec, u32 timer) {
        endBind(pActor, 2, rVec, true, timer, 0x40000000);
    }

    void endBindAndPlayerWeakGravityJump(LiveActor* pActor, const TVec3f& rVec) {
        endBind(pActor, 3, rVec, true, 0, 0);
    }

    void endBindAndPlayerForceWeakGravityJump(LiveActor* pActor, const TVec3f& rVec) {
        endBind(pActor, 3, rVec, true, 0, 0x40000000);
    }

    void endBindAndPlayerForceWeakGravityJumpInputOff(LiveActor* pActor, const TVec3f& rVec) {
        endBind(pActor, 3, rVec, true, 0, 0xC0000000);
    }

    void endBindAndPlayerWeakGravityLimitJump(LiveActor* pActor, const TVec3f& rVec) {
        endBind(pActor, 3, rVec, true, 0, 0x00800000);
    }

    void endBindAndSpinDriverJump(LiveActor* pActor, const TVec3f& rVec) {
        endBind(pActor, 3, rVec, true, 0, 0xC0000000);
    }

    void endBindAndPlayerDamage(LiveActor* pActor, const TVec3f& rVec) {
        RushEndInfo info(pActor, 3, rVec, true, 0);
        u32 damageType = 1;
        info.mFlags |= 0xC0000000;
        reinterpret_cast< RushFlagBits* >(&info.mFlags)->mDamageType = damageType;
        MarioAccess::endRush(&info);
    }

    void endBindAndPlayerFlip(LiveActor* pActor, const TVec3f& rVec) {
        RushEndInfo info(pActor, 3, rVec, true, 0);
        u32 damageType = 6;
        info.mFlags |= 0xC0000000;
        reinterpret_cast< RushFlagBits* >(&info.mFlags)->mDamageType = damageType;
        MarioAccess::endRush(&info);
    }

    void endBindAndPlayerJumpWithRollLanding(LiveActor* pActor, const TVec3f& rVec, u32 timer) {
        endBind(pActor, 3, rVec, true, timer, 0x00400000);
    }

    void endBindAndPlayerDamageMsg(LiveActor* pActor, u32 msg) {
        MR::endBindAndPlayerDamageMsg(pActor, msg, TVec3f(0.0f, 0.0f, 0.0f));
    }

    void endBindAndPlayerDamageMsg(LiveActor* pActor, u32 msg, const TVec3f& rVec) {
        switch (msg) {
        case 0x57:
        case 0x58:
        case 0x59:
            MR::endBindAndPlayerFireDamage(pActor);
            break;
        case 0x5C:
            MR::endBindAndPlayerAcidDamage(pActor);
            break;
        case 0x5D:
            MR::endBindAndPlayerFreezeDamage(pActor);
            break;
        case 0x5A:
        case 0x5B:
            MR::endBindAndPlayerElectricDamage(pActor);
            break;
        case 0x56:
            MR::endBindAndPlayerDamage(pActor, rVec);
            break;
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x51:
        case 0x52:
            endBind(pActor, 3, rVec, true, 0, 0x00800000);
            break;
        case 0x50:
            MR::endBindAndPlayerFlip(pActor, rVec);
            break;
        default:
            MR::endBindAndPlayerDamage(pActor, rVec);
            break;
        }
    }

    void endBindAndPlayerAcidDamage(LiveActor* pActor) {
        RushEndInfo info(pActor, 3, TVec3f(0.0f, 0.0f, 0.0f), true, 0);
        u32 damageType = 4;
        info.mFlags |= 0xC0000000;
        reinterpret_cast< RushFlagBits* >(&info.mFlags)->mDamageType = damageType;
        MarioAccess::endRush(&info);
    }

    void endBindAndPlayerFreezeDamage(LiveActor* pActor) {
        RushEndInfo info(pActor, 3, TVec3f(0.0f, 0.0f, 0.0f), true, 0);
        u32 damageType = 3;
        info.mFlags |= 0xC0000000;
        reinterpret_cast< RushFlagBits* >(&info.mFlags)->mDamageType = damageType;
        MarioAccess::endRush(&info);
    }

    void endBindAndPlayerFireDamage(LiveActor* pActor) {
        RushEndInfo info(pActor, 3, TVec3f(0.0f, 0.0f, 0.0f), true, 0);
        u32 damageType = 2;
        info.mFlags |= 0xC0000000;
        reinterpret_cast< RushFlagBits* >(&info.mFlags)->mDamageType = damageType;
        MarioAccess::endRush(&info);
    }

    void endBindAndPlayerElectricDamage(LiveActor* pActor) {
        RushEndInfo info(pActor, 3, TVec3f(0.0f, 0.0f, 0.0f), true, 0);
        u32 damageType = 5;
        info.mFlags |= 0xC0000000;
        reinterpret_cast< RushFlagBits* >(&info.mFlags)->mDamageType = damageType;
        MarioAccess::endRush(&info);
    }

    LiveActor* getCurrentRushActor() {
        if (!MarioAccess::isInRush()) {
            return nullptr;
        }

        return getMarioActor()->_924->mHost;
    }

    HitSensor* getCurrentRushSensor() {
        if (!MarioAccess::isInRush()) {
            return nullptr;
        }

        return getMarioActor()->_924;
    }

    void tryPlayerCoinPull() {
        MarioAccess::tryCoinPull();
    }

    void tryPlayerPullActor(HitSensor* pSensor) {
        getMarioActor()->tryTornadoPull(pSensor);
    }

    void tryPlayerDropTakingActor() {
        MarioAccess::dropTakingActor();
    }

    void tryPlayerKillTakingActor() {
        MarioAccess::killTakingActor();
    }

    bool isPlayerTakingActor(const char* pName) {
        if (MarioAccess::getTakingSensor() == nullptr) {
            return false;
        }

        bool result;
        if (::strcmp(MarioAccess::getTakingSensor()->mHost->mName, pName) == 0) {
            result = true;
        }
        else {
            result = false;
        }

        return result;
    }

    bool isPlayerCarryAny() {
        return getMarioActor()->_468 != 0;
    }

    void startSoundPlayer(const char* pName, s32 param) {
        MR::startSound(getMarioActor(), pName, param, -1);
    }

    void startLevelSoundPlayer(const char* pName, s32 param) {
        MR::startLevelSound(getMarioActor(), pName, param, -1, -1);
    }

    void stopSoundPlayer(const char* pName, u32 param) {
        MR::stopSound(getMarioActor(), pName, param);
    }

    void startSoundPlayerJ(const char* pName) {
        getMarioActor()->playSound(pName, -1);
    }

    void showPlayer() {
        MarioAccess::show();
    }

    void hidePlayer() {
        MarioAccess::hide();
    }

    void showPlayerJoint(const char* pName) {
        MR::showJoint(getMarioActor(), pName);
    }

    void hidePlayerJoint(const char* pName) {
        MR::hideJoint(getMarioActor(), pName);
    }

    void setPlayerSpot(f32 spot, u32 color) {
        MarioAccess::setSpot(spot, color);
    }

    void startPlayerDownWipe() {
        MarioAccess::startDownWipe();
    }

    void setCameraTargetToPlayer(CameraTargetArg* pArg) {
        MarioActor* actor = getMarioActor();
        pArg->mMarioActor = actor;
        pArg->mTargetObj = nullptr;
        pArg->mTargetMtx = nullptr;
        pArg->mLiveActor = nullptr;
    }

    bool isPlayerDisableFpView() {
        return MarioAccess::isDisableFpView();
    }

    bool isFpViewChangingFailure() {
        return MarioAccess::isFpViewChangingFailure();
    }

    void stopPlayerFpView() {
        MarioAccess::stopFpView();
    }

    void setRasterScroll(s32 a1, s32 a2, s32 a3) {
        getMarioActor()->setRasterScroll(a1, a2, a3);
    }

    void noticePlayerDashChance() {
        MarioAccess::noticeDashChance();
    }

    void startPlayerTalk(const LiveActor* pActor) {
        MarioAccess::startTalk(pActor);
    }

    void endPlayerTalk() {
        MarioAccess::endTalk();
    }

    void preventPlayerRush() {
        MarioAccess::preventRush();
    }

    bool isExistMario() {
        return MR::isExistSceneObj(SceneObj_MarioHolder);
    }

    void startPlayerEvent(const char* pName) {
        NameObj* event = MR::getSceneObjHolder()->getObj(SceneObj_EventSequencer);
        reinterpret_cast< EventSequencer* >(event)->startEvent(pName);
        MR::requestMovementOn(event);
        MR::requestMovementOn(getMarioActor());
    }

    void offPlayerControl() {
        MarioAccess::offControl();
    }

    void onPlayerControl(bool resetCondition) {
        MarioAccess::onControl(resetCondition);
    }

    bool isOffPlayerControl() {
        return MarioAccess::isOffControl();
    }

    void unlockPlayerAnimation() {
        getMarioActor()->_B90 = false;
    }

    void resetPlayerStatus() {
        getMarioActor()->resetCondition();
    }

    void resetPlayerEffect() {
        MR::forceDeleteEffectAll(getMarioActor());
        MR::resetChasingStarPiece();
    }

    void setPlayerBaseMtx(MtxPtr mtx) {
        MarioAccess::setBaseMtx(mtx);
    }

    MtxPtr getPlayerBaseMtx() {
        return MarioAccess::getBaseMtx();
    }

    LiveActor* getPlayerDemoActor() {
        return MarioAccess::getPlayerActor();
    }

    void initPlayerAfterOpeningDemo() {
        getMarioActor()->initAfterOpeningDemo();
    }

    void readyPlayerDemo() {
        MarioAccess::readyDemo();
    }

    void onFollowDemoEffect() {
        MarioAccess::onFollowDemo();
    }

    void requestMovementOnPlayer() {
        MR::requestMovementOn(getMarioActor());
    }

    void calcPlayerJointMtx(TPos3f* pMtx, const char* pName) {
        pMtx->setInline(MarioAccess::getJointMtx(pName));
    }

    bool isPlayerOnPress() {
        return MarioAccess::isOnPress();
    }

    void pushPlayer(const TVec3f& rVec) {
        MarioAccess::addVelocity(rVec);
    }

    void pushPlayerFromArea(const TVec3f& rVec) {
        MarioAccess::addVelocityFromArea(rVec);
    }

    bool isPlayerInWaterMode() {
        return MarioAccess::isInWaterMode();
    }

    bool isPlayerOnWaterSurface() {
        return MarioAccess::isOnWaterSurface();
    }

    bool isPlayerHidden() {
        return getMarioActor()->mPlayerMode == 6;
    }

    void calcPlayerWorldPadDir(TVec3f* pDir, f32 x, f32 y) {
        MarioAccess::calcWorldPadDir(pDir, x, y);
    }

    JUTTexture* getFullScreenBlurTexture() {
        return getMarioActor()->_B7C;
    }

    u16 getPlayerMovementTimer() {
        return getMarioActor()->_378;
    }

    CubeCameraArea* getCameraCube() {
        return MarioAccess::getCameraCubeCode();
    }
}  // namespace MR
