#include "Game/Player/MarioFoo.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/MapObj/DashRing.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/Color.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/DirectDrawUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include <JSystem/JUtility/JUTTexture.hpp>
#include <revolution/gx.h>
#include <revolution/mtx.h>

namespace MR {
    ResTIMG* getTexture(ResourceHolder*, const char*);
    void calcSpherePos(TVec3f*, const AreaObj*);
    f32 getSphereRadius(const AreaObj*);
    void rotAxisVecRad(const TVec3f&, const TVec3f&, TVec3f*, f32);
};  // namespace MR

namespace NrvMarioActor {
    INIT_NERVE(MarioActorNrvWait);
    INIT_NERVE(MarioActorNrvGameOver);
    INIT_NERVE(MarioActorNrvGameOverAbyss);
    INIT_NERVE(MarioActorNrvGameOverAbyss2);
    INIT_NERVE(MarioActorNrvGameOverFire);
    INIT_NERVE(MarioActorNrvGameOverBlackHole);
    INIT_NERVE(MarioActorNrvGameOverNonStop);
    INIT_NERVE(MarioActorNrvGameOverSink);
    INIT_NERVE(MarioActorNrvTimeWait);
    INIT_NERVE(MarioActorNrvNoRush);
};  // namespace NrvMarioActor

namespace {
    f32 cTurnMotionSpeed = 5.0f;
    f32 cWidth = 70.0f;
    f32 cLimitAngleSink = JGeometry::TUtil< f32 >::PI() / 1.0001f;
    f32 cNeutralAngleWait = JGeometry::TUtil< f32 >::PI() * 0.5f - JGeometry::TUtil< f32 >::PI() / 6.0f;
    f32 cLimitAngleWait = JGeometry::TUtil< f32 >::PI() / 1.0001f;
    f32 cUpperAngleWait = JGeometry::TUtil< f32 >::PI() / 100.0f;
};  // namespace

void Mario::tryStartFoo() {
    AreaObj* pArea = MR::getAreaObj("CelestrialSphere", getTrans());
    if (pArea != nullptr || mFoo->_48 == nullptr) {
        changeStatus(mFoo);
        MarioActor* pActor = mActor;
        MarioFoo* pFoo = mFoo;
        pFoo->_28 = pActor->mConst->getTable()->mSwimFrontMaxSpeed;
        pFoo->_AC = 20;
    }
}

MarioFoo::MarioFoo(MarioActor* pActor) : MarioState(pActor, MarioStatus_Foo) {
    _11 = false;
    _12 = false;
    _14 = 0;
    _18 = 0;
    _1A = 0;
    _1C = 0;
    _1E = 0;
    _20 = 0.0f;
    _24 = 0.0f;
    _28 = 0.0f;
    _2C = 0.0f;
    _30.zero();
    _3C.zero();
    _48 = nullptr;
    _4C = 0;
    _4E = 0;
    _50 = 0;
    _54 = 0.0f;
    _58 = false;
    _59 = 0;
    _5A = 0;
    _5C = 0.0f;
    _60 = 0;
    _64.zero();
    _70 = 0.0f;
    _74 = 0.0f;
    PSMTXIdentity(_78);
    _A8 = 0;
    _AA = 0;
    _6B0 = 0;
    _6B4 = 0;
    _6B8 = nullptr;
    _AC = 0;
    _AE = false;

    for (u32 i = 0; i < 64; i++) {
        _B0[i].zero();
        _3B0[i].zero();
    }
}

void MarioFoo::init() {
    _6B8 = new JUTTexture(MR::getTexture(MR::getModelResourceHolder(mActor), "FooLine.bti"), 0);
}

bool MarioFoo::start() {
    _20 = 0.0f;
    _24 = 0.0f;
    _28 = 0.0f;
    _4C = 30;
    _A8 = 0;
    _11 = false;
    _6B0 = 0;
    _6B4 = 0;
    _4E = 0;
    _50 = 0;
    _54 = 0.0f;
    _1C = 0;
    _1E = 0;
    _60 = 0;
    _64.zero();
    _70 = 0.0f;
    _74 = 0.0f;
    _14 = 0;
    _2C = JGeometry::TUtil< f32 >::PI() / 6.0f;
    _30 = getFrontVec();
    MR::normalize(&_30);
    stopAnimationUpper(nullptr, nullptr);
    getPlayer()->mMovementStates._1 = false;
    getPlayer()->mMovementStates._A = false;
    _18 = 0;

    if (checkLvlA()) {
        _12 = true;
    }

    changeAnimation("フーファイター飛行開始", "フーファイター飛行");
    startPadVib(2);
    _48 = MR::getAreaObj("CelestrialSphere", getTrans());
    _AC = 0;
    _AE = false;
    return true;
}

bool MarioFoo::update() {
    _14++;

    if (checkTrgZ()) {
        playSound("声尻ドロップ", -1);
        playSound("フーブレーキ", -1);
        _11 = true;
        _59 = 3;
    }

    if (!checkLvlA()) {
        _12 = false;
    }

    if (_11) {
        return false;
    }

    _3C = -getGravityVec();
    jet();

    if (getPlayer()->_1C._9) {
        if (_6B0 != 0) {
            _6B0--;
        }
    } else {
        _B0[_6B4] = getTrans();

        TVec3f handLeft;
        TVec3f handRight;
        mActor->getRealPos("HandL", &handLeft);
        mActor->getRealPos("HandR", &handRight);

        TVec3f handDelta = handRight - handLeft;
        TVec3f handDirection;
        handDirection = handDelta;
        MR::normalizeOrZero(&handDirection);
        _3B0[_6B4] = handDirection;

        if (_6B0 < 64) {
            _6B0++;
        }

        _6B4 = (_6B4 + 1) & 0x3F;
    }

    const f32 speedStopRatio = 1.0f
        - MR::clamp(_28 / mActor->mConst->getTable()->mSwimFrontMaxSpeed, 0.0f, 1.0f);

    _20 = _20 * mActor->mConst->getTable()->mSwimRotXIne
        + getStickY() * (1.0f - mActor->mConst->getTable()->mSwimRotXIne);
    _24 = _24 * mActor->mConst->getTable()->mSwimRotZIne
        + getStickX() * (1.0f - mActor->mConst->getTable()->mSwimRotZIne);

    if (_1C == 0 && _18 != 0) {
        _2C += speedStopRatio * (_20 * mActor->mConst->getTable()->mSwimRotSpeedX);
    }

    if (!checkLvlA() && !checkLvlZ() && _18 == 0 && _28 < cTurnMotionSpeed) {
        if (_2C > JGeometry::TUtil< f32 >::PI() * 0.5f) {
        } else if (getStickY() > 0.0f) {
            if (JGeometry::TUtil< f32 >::PI() / 6.0f
                    + getStickY() * (JGeometry::TUtil< f32 >::PI() / 6.0f)
                > _2C) {
            }
        }

        if (_60 == 0) {
            _1C++;
        }

        if (_1C >= 120) {
            _1C = 120;
        }
    } else {
        _1C = 0;
    }

    f32 targetAngle;
    if (MR::isNearZero(getStickY(), 0.1f)) {
        f32 waitRatio = static_cast< f32 >(_1C) / 120.0f;
        if (waitRatio > 1.0f) {
            waitRatio = 1.0f;
        }

        targetAngle = _2C + waitRatio * (cNeutralAngleWait - _2C);
    } else if (getStickY() > 0.0f) {
        f32 lowerRatio = 0.0f;
        f32 waitRatio = static_cast< f32 >(_1C) / 120.0f;

        if (_28 < 2.0f) {
            lowerRatio = 1.0f;
        } else if (_28 <= 10.0f) {
            lowerRatio = 1.0f - (_28 - 2.0f) * 0.125f;
        }

        if (waitRatio < lowerRatio) {
            waitRatio = lowerRatio;
        }

        waitRatio = MR::clamp(waitRatio, 0.0f, 1.0f);
        f32 limitAngle = waitRatio * cLimitAngleWait + (1.0f - waitRatio) * cLimitAngleSink;
        if (getPlayer()->mVerticalSpeed < 100.0f) {
            limitAngle = cNeutralAngleWait;
        }

        targetAngle = cNeutralAngleWait + (limitAngle - cNeutralAngleWait) * getStickY();
    } else if (getStickY() < 0.0f) {
        targetAngle = cNeutralAngleWait + (cUpperAngleWait - cNeutralAngleWait) * -getStickY();
    }

    f32 angleRatio = 0.05f;
    if (_28 > 5.0f) {
        angleRatio = 0.05f - 0.05f * (5.0f / _28);
        if (angleRatio < 0.0f) {
            angleRatio = 0.0f;
        }
    }

    angleRatio *= mActor->mConst->getTable()->mSwimXJetRotRatio;
    if (getStickP() == 0.0f) {
        targetAngle = cLimitAngleSink;
        angleRatio *= 0.5f;
    }

    if (getPlayer()->_1C._9) {
        targetAngle = JGeometry::TUtil< f32 >::PI() / 3.0f;
        if (getStickY() > 0.1f) {
            const f32 ratio = 1.1f * (getStickY() - 0.1f);
            targetAngle = cLimitAngleSink * ratio + (JGeometry::TUtil< f32 >::PI() / 3.0f) * (1.0f - ratio);
        } else if (getStickY() < -0.1f) {
            const f32 ratio = 1.1f * (-getStickY() - 0.1f);
            targetAngle = cUpperAngleWait * ratio + (JGeometry::TUtil< f32 >::PI() / 3.0f) * (1.0f - ratio);
        }

        angleRatio = 0.01f;
    }

    _2C = _2C * (1.0f - angleRatio) + targetAngle * angleRatio;

    bool stopWaitAnimation = true;
    if (_1C != 0 && _60 == 0 && getStickY() > 0.0f) {
        const f32 realAngle = JGeometry::TUtil< f32 >::PI() / mActor->mConst->getTable()->mSwimTiltReal;
        const f32 targetRealAngle = JGeometry::TUtil< f32 >::PI() / 6.0f
            + getStickY() * (realAngle - JGeometry::TUtil< f32 >::PI() / 6.0f);

        if (_2C < targetRealAngle) {
            _2C = _2C * mActor->mConst->getTable()->mSwimTiltSpd
                + targetRealAngle * (1.0f - mActor->mConst->getTable()->mSwimTiltSpd);
        } else {
            _2C = _2C * (1.0f - angleRatio) + targetRealAngle * angleRatio;
        }

        stopWaitAnimation = false;
    }

    if (stopWaitAnimation) {
        stopAnimation("水泳ターン下", static_cast< const char* >(nullptr));
    }

    _2C = MR::clamp(_2C, cUpperAngleWait, cLimitAngleSink);

    const f32 yawRatio = speedStopRatio + mActor->mConst->getTable()->mSwimRotSpeedZStop;
    TVec3f rotationAxis = -_3C;
    MR::rotAxisVecRad(_30, rotationAxis, &_30,
        yawRatio * (_24 * mActor->mConst->getTable()->mSwimRotSpeedZ));
    MR::vecKillElement(_30, _3C, &_30);
    MR::normalize(&_30);

    TVec3f side;
    side.cross(_3C, _30);
    MR::normalize(&side);
    getPlayer()->setSideVec(side);

    TVec3f pitchedFront;
    MR::rotAxisVecRad(_30, side, &pitchedFront, _2C);
    TVec3f previousHead(getPlayer()->mHeadVec);
    TVec3f currentFront(getFrontVec());
    TVec3f blendedFront;
    MR::vecBlendSphere(currentFront, pitchedFront, &blendedFront, 0.1f);
    getPlayer()->setFrontVecKeepSide(blendedFront);

    spin();

    TVec3f forward;
    forward = getPlayer()->_1FC;
    TVec3f velocity(forward);
    velocity.scale(_28);
    const f32 gravityElement = MR::vecKillElement(velocity, _3C, &velocity);

    f32 timerRatio;
    if (_1A > 25) {
        timerRatio = static_cast< f32 >(50 - _1A) / 25.0f;
    } else {
        timerRatio = static_cast< f32 >(_1A) / 25.0f;
    }

    const f32 verticalRatio = timerRatio
        + (1.0f - timerRatio) * mActor->mConst->getTable()->mSwimSpdYratio;
    TVec3f gravityVelocity(_3C);
    gravityVelocity.scale(gravityElement);
    TVec3f verticalVelocity(gravityVelocity);
    verticalVelocity.scale(verticalRatio);
    velocity += verticalVelocity;
    addVelocity(velocity);

    if (_60 != 0) {
        _60--;
        addVelocity(_64);
        if (_60 < 120) {
            _64.x *= 0.98f;
            _64.y *= 0.98f;
            _64.z *= 0.98f;
        }
    }

    if (_18 != 0) {
        _18--;
    }

    if (_1A != 0) {
        _1A--;
    }

    updateTilt();

    if (_48 != nullptr) {
        TVec3f spherePosition;
        MR::calcSpherePos(&spherePosition, _48);
        const f32 sphereRadius = MR::getSphereRadius(_48);
        TVec3f offset = getTrans() - spherePosition;

        if (_1E == 0 && offset.length() > sphereRadius) {
            _1E = 30;
            hitWall(-offset, nullptr);
        } else if (_1E != 0) {
            _1E--;
        }
    }

    return true;
}

bool MarioFoo::notice() {
    if (getNoticedStatus() == MarioStatus_FpView) {
        return true;
    }

    return getNoticedStatus() == MarioStatus_Swim;
}

bool MarioFoo::close() {
    stopEffect("フーマリオブレーキ左");
    stopEffect("フーマリオブレーキ右");
    stopEffect("フーマリオグロー左");
    stopEffect("フーマリオグロー右");
    playEffect("フーマリオ解除左");
    playEffect("フーマリオ解除右");
    stopAnimationUpper(nullptr, nullptr);
    setYangleOffset(0.0f);
    setJointGlobalMtx(getAnimator()->getUpperJointID(), nullptr);

    switch (_59) {
    case 0:
        changeAnimation("飛び込み失敗回転着地", static_cast< const char* >(nullptr));
        break;
    case 1:
        changeAnimation("フーファイター着地", static_cast< const char* >(nullptr));
        break;
    case 2:
        stopAnimation(nullptr, static_cast< const char* >(nullptr));
        break;
    case 3:
        changeAnimation("フーファイター解除", static_cast< const char* >(nullptr));
        TVec3f zeroVelocity(0.0f, 0.0f, 0.0f);
        getPlayer()->mJumpVec = zeroVelocity;
        getPlayer()->_10._21 = true;
        break;
    }

    if (getPlayer()->mMovementStates.jumping) {
        changeAnimation(nullptr, "落下");
    } else {
        changeAnimation(nullptr, "基本");
    }

    getPlayer()->_4B0 = getPlayer()->mPosition;
    getPlayer()->forceSetHeadVecKeepSide(-getGravityVec());
    changeAnimationInterpoleFrame(6);
    mActor->setBlendMtxTimer(4);
    getPlayer()->unlockGroundCheck(this);
    mActor->_F44 = true;
    mActor->resetWaterLife();
    _54 = 0.0f;
    getPlayer()->_10._7 = true;
    return true;
}

const TVec3f& MarioFoo::getGravityVec() const {
    return MarioModule::getGravityVec();
}

void MarioFoo::jet() {
    f32 ringAcc = calcRingAcc();
    f32 targetSpeed;
    if (ringAcc != 1.0f) {
        targetSpeed = ringAcc * mActor->mConst->getTable()->mSwimFrontMaxSpeed;
    } else {
        targetSpeed = mActor->mConst->getTable()->mSwimFrontJetSpeed;
    }

    f32 acceleration = 1.02f;
    f32 deceleration = 0.98f;
    if (checkLvlA() && !_12) {
        targetSpeed = 0.0f;
        deceleration = 0.9f;
        _AC++;

        if (!isAnimationRun("フーファイタースピン")) {
            changeAnimation("フーファイター静止", static_cast< const char* >(nullptr));
        }

        playEffect("フーマリオブレーキ左");
        playEffect("フーマリオブレーキ右");
        stopEffect("フーマリオグロー左");
        stopEffect("フーマリオグロー右");
        if (!_AE) {
            playSound("フーブレーキ", -1);
        }

        _AE = true;
        getPlayer()->_1C._9 = true;
    } else if (_14 < 8) {
        _AE = true;
        _AC++;
        _28 = 0.0f;
    } else if (_AC != 0) {
        if (_AC < 8) {
            _AC = 0;
            _AE = false;
        } else {
            if (_AE) {
                playEffect("共通ひこうきブースト");
                playSound("フー加速", -1);
                _AE = false;

                if (isAnimationRun("フーファイター静止")) {
                    changeAnimation("フーファイター飛行再開", static_cast< const char* >(nullptr));
                }
            }

            if (_AC > 20) {
                _AC = 20;
            }

            acceleration = 1.2f;
            targetSpeed *= 2.0f;
            _AC--;
        }

        stopAnimation("フーファイター静止", static_cast< const char* >(nullptr));
        playEffect("フーマリオグロー左");
        playEffect("フーマリオグロー右");
        stopEffect("フーマリオブレーキ左");
        stopEffect("フーマリオブレーキ右");
    }

    f32 soundLevel = 50.0f * (1.0f + getStickY());
    if (!_AE) {
        playSound("フー飛行中", static_cast< s32 >(soundLevel));
    }
    playSound("フー滞空中", static_cast< s32 >(soundLevel));

    if (_28 < targetSpeed) {
        if (_28 < 1.0f) {
            _28 = 1.0f;
        }

        _28 *= acceleration;
        if (_4E != 0) {
            _28 *= 1.5f;
        }
    } else if (_28 > targetSpeed) {
        _28 *= deceleration;
    } else {
        _28 = targetSpeed;
    }
}

void MarioFoo::updateTilt() {
    f32 roll = getStickX() * JGeometry::TUtil< f32 >::PI() / 5.0f;
    f32 blendRatio = 0.1f;
    f32 pitch = 0.0f;

    if (_1C == 0) {
        pitch = getStickY() * JGeometry::TUtil< f32 >::PI() / 12.0f;
        if (getStickY() > 0.0f && _2C <= 2.0943952f) {
            pitch = getStickY() * JGeometry::TUtil< f32 >::PI() * 0.125f;
        }
    } else if (getStickY() < 0.0f) {
        pitch = getStickY() * JGeometry::TUtil< f32 >::PI() / mActor->mConst->getTable()->mSwimTiltZup;
    } else if (getStickY() > 0.0f) {
        pitch = getStickY() * JGeometry::TUtil< f32 >::PI() / mActor->mConst->getTable()->mSwimTiltZdown;
    } else if (_1C != 0) {
        f32 ratio = static_cast< f32 >(_1C) / 120.0f;
        if (ratio > 1.0f) {
            ratio = 1.0f;
        }
        pitch -= ratio * 0.31415927f;
    } else {
        pitch = 0.0f;
    }

    _70 = blendRatio * roll + (1.0f - blendRatio) * _70;
    _74 = 0.1f * pitch + 0.9f * _74;

    Mtx rollMtx;
    Mtx pitchMtx;
    PSMTXRotRad(rollMtx, 'X', _70);
    PSMTXRotRad(pitchMtx, 'Z', _74);
    PSMTXConcat(rollMtx, pitchMtx, _78);
    setJointGlobalMtx(getAnimator()->getUpperJointID(), _78);
}

void MarioFoo::hitWall(const TVec3f& rNormal, HitSensor* pSensor) {
    if (sendPunch(pSensor, false) == 1) {
        return;
    }

    _11 = true;
    if (MR::diffAngleAbs(rNormal, -getGravityVec()) <= 0.7853982f) {
        if (MR::diffAngleAbs(rNormal, -getPlayer()->_1FC) <= 0.7853982f) {
            _59 = 0;
        } else {
            _59 = 1;
        }
    } else {
        _59 = 2;
    }

    playSound("フーブレーキ", -1);
}

f32 MarioFoo::getStickY() const {
    return MarioModule::getStickY();
}

void MarioFoo::spin() {
    if (_60 == 0) {
        if (mActor->isRequestSpin() && _4C == 0) {
            _4C = 20;
            if (!isAnimationRun("フーファイタースピン") && !checkLvlA()) {
                playSound("声スピン", -1);
                playSound("スピンジャンプ", -1);
            }

            changeAnimation("フーファイタースピン", static_cast< const char* >(nullptr));
            if (mActor->_944 == 0) {
                mActor->_945 = 0;
                mActor->_974 = 0;
            }

            mActor->_944 = 30;
            mActor->_946 = 60;
        }

        if (_4C != 0) {
            _4C--;
        }
    }
}

bool MarioFoo::passRing(const HitSensor* pSensor) {
    const TVec3f& ringPosition = pSensor->mPosition;

    if (_4E == 0) {
        _50 = mActor->mConst->getTable()->mSwimRingDashChargeTime;
        changeAnimation("リングダッシュ準備", static_cast< const char* >(nullptr));
    }

    if (_50 != 0) {
        getPlayer()->push((ringPosition - getTrans()) * 0.25f);
    }

    DashRing* pRing = static_cast< DashRing* >(pSensor->mHost);
    _5A = pRing->_AC;
    _5C = pRing->_B0;
    _4E = pRing->_A8;
    return true;
}

f32 MarioFoo::calcRingAcc() {
    if (_4E != 0) {
        if (_50 != 0) {
            _50--;
            if (_50 == 1) {
                MarioActor* pActor = mActor;
                const MarioConstTable* pTable = pActor->mConst->getTable();
                pActor->_1AA = pTable->mStarPieceFogTime;
                pActor->_1AC = 1.0f;
                pActor->_1B0.set(0xFF, 0xFF, 0xFF, 0);
                pActor->_1B5 = false;
            }

            if (_50 == 0) {
                startPadVib(3);
                changeAnimation("リングダッシュ", static_cast< const char* >(nullptr));
            }

            return 1.0f;
        }

        _54 += 0.5f;
        if (_54 > 30.0f) {
            _54 = 30.0f;
        }

        f32 result = _5C;
        if (_4E < _5A) {
            f32 ratio = static_cast< f32 >(_4E) / static_cast< f32 >(_5A);
            result = (1.0f - ratio) + _5C * ratio;
        }

        _4E--;
        return result;
    }

    if (_54 > 0.0f) {
        _54 -= 0.5f;
    } else {
        _54 = 0.0f;
    }

    return 1.0f;
}

void MarioFoo::draw3D() const {
    if (getPlayer()->_1C._9) {
        TDDraw::setup(1, 1, 0);
        MR::ddSetVtxFormat(2);
        MR::ddLightingOff();
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_C1, GX_CC_TEXA, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
        _6B8->load(GX_TEXMAP0);

        Color8 color1(255, 32, 32, 128);
        GXSetTevColor(GX_TEVREG0, color1);
        Color8 color2(255, 64, 64, 255);
        GXSetTevColor(GX_TEVREG1, color2);

        TVec3f up(getPlayer()->_1FC);
        up.scale(50.0f);
        TVec3f center(mActor->_2A0);
        center += up;

        TVec3f negativeUp = -getPlayer()->_1FC;
        TVec3f side(getPlayer()->mSideVec);
        TVec3f normal;
        normal.cross(negativeUp, side);
        MR::normalizeOrZero(&normal);
        TVec3f back = -normal;
        const f32 fade = static_cast< f32 >(64 - _6B0) * 0.015625f;

        TVec3f leftOuter;
        leftOuter = center - side * 70.0f;
        TVec3f leftOuterBack;
        leftOuterBack = center - side * 70.0f + back * 2000.0f * fade;
        TVec3f leftInnerBack;
        leftInnerBack = center - side * 50.0f + back * 2000.0f * fade;
        TVec3f leftInner;
        leftInner = center - side * 50.0f;

        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        MR::ddSendVtxData(leftOuter, TVec2f(0.0f, 0.0f));
        MR::ddSendVtxData(leftOuterBack, TVec2f(0.5f, 0.0f));
        MR::ddSendVtxData(leftInnerBack, TVec2f(0.5f, 1.0f));
        MR::ddSendVtxData(leftInner, TVec2f(0.0f, 1.0f));

        TVec3f rightOuter;
        rightOuter = center + side * 70.0f;
        TVec3f rightOuterBack;
        rightOuterBack = center + side * 70.0f + back * 2000.0f * fade;
        TVec3f rightInnerBack;
        rightInnerBack = center + side * 50.0f + back * 2000.0f * fade;
        TVec3f rightInner;
        rightInner = center + side * 50.0f;

        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        MR::ddSendVtxData(rightOuter, TVec2f(0.0f, 0.0f));
        MR::ddSendVtxData(rightOuterBack, TVec2f(0.5f, 0.0f));
        MR::ddSendVtxData(rightInnerBack, TVec2f(0.5f, 1.0f));
        MR::ddSendVtxData(rightInner, TVec2f(0.0f, 1.0f));
    }

    Color8 color2(255, 64, 64, 255);
    GXSetTevColor(GX_TEVREG1, color2);

    if (_6B0 == 0) {
        return;
    }

    TDDraw::setup(1, 1, 0);
    MR::ddSetVtxFormat(2);
    MR::ddLightingOff();
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_C1, GX_CC_TEXA, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
    _6B8->load(GX_TEXMAP0);

    const u32 pointCount = _6B0;
    const u32 halfCount = pointCount >> 1;
    TVec3f previousLeftOuter;
    TVec3f previousLeftInner;
    TVec3f previousRightOuter;
    TVec3f previousRightInner;
    TVec3f previousPosition;
    TVec2f previousTexLeft;
    TVec2f previousTexRight;

    for (u32 i = 0; i < pointCount; i++) {
        s32 alpha;
        if (i > halfCount) {
            alpha = static_cast< s32 >(255.0f * (static_cast< f32 >(pointCount - i) / static_cast< f32 >(pointCount)));
        } else {
            alpha = static_cast< s32 >(255.0f * (static_cast< f32 >(i) / static_cast< f32 >(pointCount)));
        }

        Color8 color1(255, 0, 0, static_cast< u8 >(alpha));
        GXSetTevColor(GX_TEVREG0, color1);

        const u32 pointIndex = (_6B4 + 127 - i) & 0x3F;
        if (i == 0) {
            const TVec3f* pPosition = &_B0[pointIndex];
            u32 j = 1;
            for (; j < pointCount; j++) {
                TVec3f delta = _B0[(_6B4 + 127 - j) & 0x3F] - *pPosition;
                TVec3f cameraDirection = MR::getCamZdir();
                TVec3f projected;
                MR::vecKillElement(delta, cameraDirection, &projected);
                if (!MR::normalizeOrZero(&projected)) {
                    cameraDirection = MR::getCamZdir();
                    TVec3f offset;
                    offset.cross(projected, cameraDirection);
                    offset.scale(10.0f);

                    const TVec3f* pDirection = &_3B0[pointIndex];
                    previousLeftOuter = *pPosition + offset - *pDirection * cWidth;
                    previousLeftInner = *pPosition - offset - *pDirection * cWidth;
                    previousRightOuter = *pPosition + offset + *pDirection * cWidth;
                    previousRightInner = *pPosition - offset + *pDirection * cWidth;
                    break;
                }
            }

            if (j >= pointCount) {
                return;
            }

            previousPosition = *pPosition;
            previousTexLeft.set(0.0f, 0.0f);
            previousTexRight.set(1.0f, 0.0f);
            continue;
        }

        const TVec3f* pPosition = &_B0[pointIndex];
        const TVec3f* pDirection = &_3B0[pointIndex];
        TVec2f texLeft(0.0f, static_cast< f32 >(i + 1) / static_cast< f32 >(pointCount));
        TVec2f texRight(1.0f, texLeft.y);

        TVec3f delta = *pPosition - previousPosition;
        TVec3f offset(*pDirection);
        offset.setLength(10.0f);

        TVec3f leftOuter;
        leftOuter = *pPosition + offset - *pDirection * cWidth;
        TVec3f leftInner;
        leftInner = *pPosition - offset - *pDirection * cWidth;
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        MR::ddSendVtxData(previousLeftOuter, previousTexLeft);
        MR::ddSendVtxData(leftOuter, texLeft);
        MR::ddSendVtxData(leftInner, texRight);
        MR::ddSendVtxData(previousLeftInner, previousTexRight);

        TVec3f rightOuter;
        rightOuter = *pPosition + offset + *pDirection * cWidth;
        TVec3f rightInner;
        rightInner = *pPosition - offset + *pDirection * cWidth;
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        MR::ddSendVtxData(previousRightOuter, previousTexLeft);
        MR::ddSendVtxData(rightOuter, texLeft);
        MR::ddSendVtxData(rightInner, texRight);
        MR::ddSendVtxData(previousRightInner, previousTexRight);

        previousLeftOuter = leftOuter;
        previousTexLeft = texLeft;
        previousLeftInner = leftInner;
        previousTexRight = texRight;
        previousRightOuter = rightOuter;
        previousRightInner = rightInner;
        previousPosition = *pPosition;
    }
}

f32 MarioFoo::getBlurOffset() const {
    return _54;
}
