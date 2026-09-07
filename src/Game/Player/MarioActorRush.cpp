#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/MapObj/CollectCounter.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioDamage.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/RushEndInfo.hpp"
#include "Game/Screen/GameSceneLayoutHolder.hpp"
#include "Game/Player/MarioModule.hpp"
#include "Game/Player/MarioParts.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Player/MarioWait.hpp"
#include "Game/Util.hpp"

void MarioActor::settingRush() {
    rushDropThrowMemoSensor();

    switch ((s32)_924->mType) {
    case ATYPE_WATER_PRESSURE_BULLET_BIND:
        mMario->touchWater();
        break;
    }

    _934 = 1;
    resetCondition();
    XanimePlayer* p = mMario->getAnimator()->mXanimePlayer;
    p->_7E = false;
}

bool MarioActor::isFixJumpRushSensor(const HitSensor* pSensor) const {
    switch (pSensor->mType) {
    case ATYPE_TRANSFERABLE_BINDER:
    case ATYPE_SPIN_DRIVER_BIND:
    case ATYPE_SPIN_CANNON_BIND:
    case ATYPE_G_CAPTURE_BIND:
    case ATYPE_SPRING_ATTACKER_BIND:
    case ATYPE_WATER_PRESSURE_BULLET_BIND:
    case ATYPE_MARIO_LAUNCHER_BIND:
        return true;

    default:
        return selectJumpRushSensor(pSensor->mHost->mName);
    }
}

bool MarioActor::isLandEffectRushSensor(const HitSensor* pSensor) const {
    switch (pSensor->mType) {
    case ATYPE_MARIO_LAUNCHER_BIND:
    case ATYPE_SPIN_CANNON_BIND:
        return true;
    default:
        return false;
    }
}

void MarioActor::resetCondition() {
    bool v2 = false;
    TVec3f v8;

    if (_EA5) {
        MR::extractMtxTrans(_EA8.mMtx, &mPosition);
        mMario->mPosition = mPosition;
    }

    if (mMario->mSwim->checkWaterCube(true)) {
        v2 = true;
    }

    mMario->stopAnimationUpper(nullptr, nullptr);

    if (mMario->mMovementStates._A) {
        mMario->cancelSquatMode();
    }

    XanimePlayer* p = mMario->getAnimator()->mXanimePlayer;
    p->_7E = true;
    mMario->forceStopTornado();
    updateTornado();

    mMario->mSwim->mColdWaterDamageInterval = 0;
    mMario->_1C._3 = 0;

    if (mMario->isStatusActive(MarioStatus_Swim) && v2) {
        if (!_468) {
            mMario->mSwim->resetJet();
        }
    } else if (mMario->isStatusActive(MarioStatus_Teresa) && mPlayerMode == 6) {
        if (mMario->isStatusActive(MarioStatus_Wait)) {
            mMario->closeStatus(mMario->mWait);
        }

        mMario->resetTeresaMode();
    } else {
        mMario->closeStatus(nullptr);
    }

    // reset mMovementstates and _10
    // exept for mMovementStates._1F
    bool was1F = mMario->mMovementStates._1F;
    mMario->mMovementStates_LOW_WORD = 0;
    mMario->mMovementStates_HIGH_WORD = 0;
    mMario->_10_LOW_WORD = 0;
    mMario->_10_HIGH_WORD = 0;
    mMario->mMovementStates._1F = was1F;

    resetPadSwing();

    if (mMario->isStatusActive(MarioStatus_Swim) && v2) {
        if (_468) {
            mMario->changeAnimation("水泳ジェット", "水泳ジェット");
        } else {
            mMario->changeAnimation(nullptr, "水泳基本");
        }
    } else {
        mMario->changeAnimation(nullptr, "基本");

        if (_468) {
            mMarioAnim->waterToGround();
        }
    }

    mMario->setFrontVecKeepUp(mMario->mFrontVec);
    updateGravityVec(true, true);

    if (!MR::isNearZero(_24C)) {
        mMario->setHeadVec(-_24C);
    }

    mMario->_1FC = mMario->mHeadVec;
    mMario->_13C.zero();
    mMario->_148.zero();
    mMario->_154.zero();
    mMario->mVerticalSpeed = 0.0f;
    mMario->_474->mIdx = -1;
    stopEffect("いい汗");
    stopEffect("ホッパー尻落ルイージ");
    stopEffect("ホッパー尻落");
    stopEffect("尻落ルイージ");
    stopEffect("尻落");

    _9C4->kill();
    MR::offCalcAnim(_9C4);

    for (u32 i = 0; i < 11; i++) {
        mMario->_984[i] = nullptr;
    }

    if (_EA5) {
        mMario->changeAnimationInterpoleFrame(0);
        mBlendMtxTimer = 0;
    }
}

void MarioActor::beginRush() {
    _924 = _7E4[0];
    mMarioAnim->clearAllJointTransform();
    MR::invalidateHitSensors(this);
    stopEffect("共通壁手擦り");
    stopEffect("スピンライト");
    bool isSpinCatch = selectSpinCatchInRush(_924->mHost->mName);
    if (mPlayerMode == 4 && selectHideFlyMeter(_924)) {
        MR::getGameSceneLayoutHolder()->changeLifeMeterModeGround();
    }
    if (isFixJumpRushSensor(_924) || isSpinCatch) {
        settingRush();
        getSensor("eye")->validate();
        getSensor("body")->validate();
    } else {
        getSensor("body")->validate();
        if (_924->isType(ATYPE_POWER_STAR_BIND)) {
            MR::forceDeleteEffectAll(this);
            _1B8->kill();
        }
        if (selectLandEffect(_924)) {
            playEffect("特殊着地");
        }
        switch (_924->mType) {
        case ATYPE_POWER_STAR_BIND:
            setPlayerMode(0, false);
            resetFog();
            break;
        }
        settingRush();
        if (!_924->isType(ATYPE_POWER_STAR_BIND)) {
            setBlendMtxTimer(mConst->getTable()->mRushInBlendTimer);
        }
    }
}

void MarioActor::endRush(const RushEndInfo* pInfo) {
    bool isJump = false;
    if (_924 == nullptr) {
        return;
    }
    if (pInfo->_1C != nullptr && _924->mHost != pInfo->_1C) {
        return;
    }

    _934 = false;
    _935 = true;
    mMario->stopJump();
    s32 mode = pInfo->_4;
    mMario->mMovementStates._22 = false;
    if (mMario->isStatusActive(MarioStatus_Swim) && mode < 4 && mode >= 1) {
        mode = 4;
    }

    switch (mode) {
    case 2:
        isJump = true;
        mMario->initJumpParam();
        mMario->mMovementStates._21 = true;
        if (mPlayerMode == 4) {
            mMario->_774 = 60;
        }
        mMario->tryForceFreeJump(pInfo->_8);
        mMario->mMovementStates._9 = true;
        mMario->_10._D = true;
        if (pInfo->_20 & 0x40000000) {
            mMario->mMovementStates._2F = true;
        } else {
            mMario->mMovementStates._2F = false;
        }
        if (pInfo->_20 & 0x80000000) {
            mMario->mMovementStates._22 = true;
        }
        break;
    case 3:
        isJump = true;
        if (mPlayerMode == 4) {
            mMario->_774 = 60;
        }
        mMario->tryForcePowerJump(pInfo->_8, false);
        if (pInfo->_20 & 0x40000000) {
            mMario->mMovementStates._2F = true;
        } else {
            mMario->mMovementStates._2F = false;
        }
        if (pInfo->_20 & 0x00800000) {
            mMario->mMovementStates._20 = false;
        }
        if (pInfo->_20 & 0x80000000) {
            mMario->mMovementStates._22 = true;
        }
        switch ((pInfo->_20 >> 24) & 0xF) {
        case 1:
            mMario->mMovementStates._1B = true;
            mMario->mDamage->setVec(pInfo->_8);
            break;
        case 4:
            mMario->_10.jumping = true;
            break;
        case 2:
            mMario->_10.jumping = true;
            break;
        case 5:
            mMario->_10._14 = true;
            break;
        case 6:
            mMario->doFlipLarge(pInfo->_8);
            break;
        case 3:
            mMario->doFreeze();
            break;
        }
        mMario->_10._D = true;
        if (selectRebindTimer(_924)) {
            _92C = _37C;
        }
        break;
    case 1:
        mMario->_10._D = true;
        mMario->mMovementStates._21 = true;
        break;
    case 0:
        mMario->stopWalk();
    case 4:
        mMario->mMovementStates._B = false;
        mMario->mMovementStates.jumping = false;
        mMario->_3C0 = 0;
        break;
    case 5:
        takeSensor(_924);
        break;
    }

    mMario->_10._8 = (pInfo->_20 >> 22) & 1;
    if (_3D0 == 0) {
        TMtx34f mtx;
        PSMTXConcat(getBaseMtx(), _E3C.toMtxPtr(), mtx.toMtxPtr());
        TVec3f front;
        front.set<f32>(mtx.mMtx[0][2], mtx.mMtx[1][2], mtx.mMtx[2][2]);
        mMario->setFrontVecKeepUp(front);
    } else {
        TMtx34f mtx;
        PSMTXConcat(getBaseMtx(), _E3C.toMtxPtr(), mtx.toMtxPtr());
        TVec3f front;
        TVec3f side;
        TVec3f up;
        front.set<f32>(mtx.mMtx[0][2], mtx.mMtx[1][2], mtx.mMtx[2][2]);
        up.set<f32>(mtx.mMtx[0][1], mtx.mMtx[1][1], mtx.mMtx[2][1]);
        side.set<f32>(mtx.mMtx[0][0], mtx.mMtx[1][0], mtx.mMtx[2][0]);
        MR::normalize(&front);
        MR::normalize(&side);
        MR::normalize(&up);
        if (!MR::isNearZero(up)) {
            mMario->mHeadVec = up;
        }
        if (!MR::isNearZero(side)) {
            mMario->mSideVec = side;
        }
        if (!MR::isNearZero(front)) {
            mMario->setFrontVec(front);
        }
        if (up.dot(-*mMario->getGravityVec()) < 0.0f) {
            mMario->setFrontVecKeepSide(_3C4);
        } else if (_3D0 > 1) {
            mMario->setFrontVecKeepUp(_3C4, static_cast<u32>(_3D0));
        } else {
            mMario->setFrontVecKeepUp(_3C4);
        }
    }
    mMario->_1FC = mMario->mHeadVec;
    if (!pInfo->_14) {
        mMario->stopAnimation(nullptr, static_cast<const char*>(nullptr));
    }
    if (pInfo->_18 != 0) {
        setBlendMtxTimer(pInfo->_18);
    }
    MR::validateHitSensors(this);
    getSensor("dummy")->invalidate();
    mMario->getAnimator()->mXanimePlayer->_7E = true;
    if (_F1C < 15) {
        _F1C = 15;
    }
    if (_F1E < 15) {
        _F1E = 15;
    }
    _EF8 = 0;
    _F04 = 0;
    if (((pInfo->_20 >> 24) & 0xF) == 0 && isLandEffectRushSensor(_924) && (pInfo->_20 & 0x40000000)) {
        mMario->mMovementStates._3E = 1;
        mMario->mMovementStates.jumping = true;
        mMario->mMovementStates._1 = false;
    } else if (_924->isType(0x6B)) {
        if (!MR::isNearZero(_938)) {
            mMario->mMovementStates._3E = 1;
            mMario->mJumpVec = _938;
        }
    } else if (_924->isType(0x6D)) {
        if (pInfo->_4 != 0) {
            mMario->mMovementStates._3E = 2;
            mMario->mMovementStates.jumping = true;
            mMario->mMovementStates._1 = false;
        } else {
            changeAnimation("基本", nullptr);
        }
        updateGravityVec(true, true);
        _2C4 = _24C * -70.0f;
        _240 = _24C;
        mMario->setGravityVec(_240);
    }
    _928 = _924;
    _924 = nullptr;
    mMario->_10.turning = true;
    updateGravityVec(true, false);
    if (mPlayerMode == 5) {
        mMario->startRabbitMode();
    }
    if (mPlayerMode == 4) {
        MR::getGameSceneLayoutHolder()->changeLifeMeterModeBee();
    }
    if (isJump) {
        mLastMove = mMario->mVelocity;
    } else {
        mMario->mVelocity = mLastMove;
    }
    mMarioAnim->_15 = 0xFF;
    if (selectWaterInOutRush(_928) && !mMario->forceStartSwimAndShoot(pInfo->_8)) {
        mMario->forceExitSwim();
    }
    mMario->_350.zero();
    mMario->_35C.zero();
    if (_938.length() > 200.0f) {
        mMario->_1C._8 = true;
    }
}


bool MarioActor::takeSensor(HitSensor* pSensor) {
    if (_424) {
        return false;
    }

    if (_468) {
        return false;
    }

    changeAnimation("カブ抜き", nullptr);
    changeNullAnimation("PullOut", -1);
    _424 = pSensor;
    memorizeSensorThrow(pSensor);
    _480 = 1;
    _38C = 60;
    return true;
}
