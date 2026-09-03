#if !defined(TARGET_PC)
#define JMAAcosRadian JMAAcosRadianInline
#endif

#include "Game/Enemy/KarikariDirector.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Player/MarioWall.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include <cstring>
#include <revolution/mtx.h>

#if !defined(TARGET_PC)
#undef JMAAcosRadian
#endif

f32 JMAAcosRadian(f32);

bool Mario::isWalling() const {
    return getCurrentStatus() == MarioStatus_Wall;
}

void Mario::checkWallStick() {
    if (getPlayerMode() == 4) {
        checkBeeStick();
        return;
    }

    if (checkWallJumpCode() || !isEnableStickWall()) {
        return;
    }

    if (mActor->_334 != 0) {
        TVec3f horizontal;
        const f32 vertical = MR::vecKillElement(mJumpVec, mActor->_240, &horizontal);
        horizontal.setLength(-0.25f);

        TVec3f verticalVec(mActor->_240);
        verticalVec.scale(vertical);
        TVec3f jumpVec(verticalVec);
        jumpVec += horizontal;
        mJumpVec = jumpVec;
        return;
    }

    changeStatus(mWall);
    stopWalk();
    mMovementStates.jumping = false;
    mMovementStates._B = false;
    mMovementStates._6 = false;
    mMovementStates._17 = false;
    fixWallingPosition(true);
    resetTornado();
}

u8 Mario::checkStickWallSide() {
    if (isStickOn()) {
        TVec3f& padDir = getWorldPadDir();
        if (padDir.dot(getWallNorm()) < -0.5f) {
            return 1;
        }

        if (padDir.dot(getWallNorm()) > 0.5f && !mMovementStates._1D) {
            return 2;
        }
    }

    return 0;
}

s32 Mario::checkStickFrontBack() {
    if (isStickOn()) {
        TVec3f& padDir = getWorldPadDir();
        TVec3f front(-mFrontVec);
        if (padDir.dot(front) < -0.5f) {
            return 1;
        }

        front = -mFrontVec;
        if (padDir.dot(front) > 0.5f && !mMovementStates._1D) {
            return 2;
        }
    }

    return 0;
}

MarioWall::MarioWall(MarioActor* pActor) : MarioState(pActor, MarioStatus_Wall), _14(0) {
    _18 = 0;
    _1C = 0;
    _1D = 0;
    _1E = 0;
    _20 = 0.0f;
    _24.zero();
    _30.zero();
}

void MarioWall::initTriangleJump() {
    Mario* player = getPlayer();
    MR::vecKillElement(player->mJumpVec, getGravityVec(), &_30);
    MR::normalizeOrZero(&_30);
    _14 = 0;
}

bool MarioWall::isCancel() {
    if (getPlayer()->mMovementStates._1) {
        _1C = false;
    }

    if (_1C && getPlayer()->mMovementStates._8) {
        const MarioConstTable* table = mActor->mConst->getTable();
        if (_24.dot(getPlayer()->getWallNorm()) < table->mWallStickCancelAngle) {
            _1C = false;
        }
    }

    return _1C;
}

bool Mario::fixWallingPosition(bool immediate) {
    if (!fixWallingDist()) {
        return false;
    }

    fixWallingDir(immediate);
    return true;
}

void Mario::fixWallingDir(bool immediate) {
    bool allowBackWall = true;
    if (isStatusActive(MarioStatus_SideStep)) {
        allowBackWall = false;
    }

    if (mMovementStates._19 && allowBackWall) {
        if (immediate) {
            TVec3f front(-*mBackWallTriangle->getNormal(0));
            setFrontVecKeepUp(front);
        } else {
            TVec3f front(-*mBackWallTriangle->getNormal(0));
            setFrontVecKeepUp(front, 0.1f);
        }
        return;
    }

    if (mMovementStates._8) {
        if (immediate) {
            TVec3f front(-*mFrontWallTriangle->getNormal(0));
            setFrontVecKeepUp(front);
        } else {
            TVec3f front(-*mFrontWallTriangle->getNormal(0));
            setFrontVecKeepUp(front, 0.1f);
        }
    }
}

bool Mario::fixWallingTop() {
    TVec3f side;
    side.cross(getWallNorm(), getAirGravityVec());
    MR::normalizeOrZero(&side);
    if (MR::isNearZero(side, 0.001f)) {
        return false;
    }

    _75C.cross(getWallNorm(), side);
    MR::normalizeOrZero(&_75C);
    if (MR::isNearZero(_75C, 0.001f)) {
        return false;
    }

    getWallNorm().dot(getAirGravityVec());
    getPlayer()->forceSetHeadVecKeepSide(_75C);
    return true;
}

bool Mario::checkWallFloorCode(u16 code) const {
    if ((mMovementStates._8 || mMovementStates._32) && _964[0] == code) {
        return true;
    }
    if (mMovementStates._19 && _964[1] == code) {
        return true;
    }
    if (mMovementStates._1A && _964[2] == code) {
        return true;
    }
    return false;
}

bool Mario::checkWallCode(const char* pCode, bool ignoreSide) const {
    if (mMovementStates._19) {
        const char* wallCode = MR::getWallCodeString(mBackWallTriangle);
        if (wallCode != nullptr && strcmp(pCode, wallCode) == 0) {
            return true;
        }
    }

    if (mMovementStates._8 || mMovementStates._32) {
        const char* wallCode = MR::getWallCodeString(mFrontWallTriangle);
        if (wallCode != nullptr && strcmp(pCode, wallCode) == 0) {
            return true;
        }
    }

    if (ignoreSide) {
        return false;
    }

    if (mMovementStates._1A) {
        const char* wallCode = MR::getWallCodeString(mSideWallTriangle);
        if (wallCode != nullptr && strcmp(pCode, wallCode) == 0) {
            return true;
        }
    }
    return false;
}

bool Mario::checkWallCodeNorm(u16 code, TVec3f* pNorm, bool ignoreSide) const {
    if (mMovementStates._19 && MR::getWallCodeIndex(mBackWallTriangle) == code) {
        if (pNorm != nullptr) {
            *pNorm = *mBackWallTriangle->getNormal(0);
        }
        return true;
    }

    if ((mMovementStates._8 || mMovementStates._32) && MR::getWallCodeIndex(mFrontWallTriangle) == code) {
        if (pNorm != nullptr) {
            *pNorm = *mFrontWallTriangle->getNormal(0);
        }
        return true;
    }

    if (ignoreSide) {
        return false;
    }

    if (mMovementStates._1A && MR::getWallCodeIndex(mSideWallTriangle) == code) {
        if (pNorm != nullptr) {
            *pNorm = *mSideWallTriangle->getNormal(0);
        }
        return true;
    }
    return false;
}

void Mario::setWallCancel() {
    mWall->_1C = true;
    mWall->_24 = getWallNorm();
}

void Mario::keepDistFrontWall() {
    if (!mMovementStates._8) {
        return;
    }

    TVec3f offset(_4E8);
    offset -= mGroundPos;
    TVec3f horizontal;
    MR::vecKillElement(offset, *getGravityVec(), &horizontal);
    if (horizontal.length() < 80.0f) {
        const f32 correction = 80.0f - horizontal.length();
        TVec3f pushVec(*mFrontWallTriangle->getNormal(0));
        pushVec.scale(correction);
        push(pushVec);
    }
}

bool Mario::isEnableStickWall() {
    switch (getPlayerMode()) {
    case 6:
        return false;
    case 7:
        if (isStatusActive(MarioStatus_Foo)) {
            return false;
        }
        break;
    }

    if (mWall->isCancel()) {
        return false;
    }

    if (!mMovementStates.jumping || mMovementStates._1) {
        return false;
    }
    if (mMovementStates._B || mMovementStates._F || mDrawStates._1E) {
        return false;
    }
    if (!mMovementStates._8 && !mMovementStates._19) {
        return false;
    }

    const MarioConstTable* table = mActor->mConst->getTable();
    if (mMovementStates._2 && mVerticalSpeed < table->mWallStickGrHeight) {
        return false;
    }

    if (mMovementStates._15 && _4E0 < table->mWallStickFrHeight && !getWallPolygon()->mSensor->isType(0x57)) {
        return false;
    }

    if (isAnimationRun("空中ひねり")) {
        if (mActor->_945 < 25) {
            return false;
        }
    } else if (mActor->isPunching() && mActor->_945 < 15) {
        return false;
    }

    if (mMovementStates._15 && mMovementStates._2 && mMovementStates._39) {
        TVec3f step(_4A4);
        step -= mShadowPos;
        TVec3f up(-*getGravityVec());
        if (step.dot(up) < table->mWallStickStepHeight) {
            return false;
        }
    }

    if (mMovementStates._19) {
        TVec3f jumpHorizontal;
        MR::vecKillElement(mJumpVec, mActor->_240, &jumpHorizontal);
        if (mBackWallTriangle->getNormal(0)->dot(jumpHorizontal) > 0.0f) {
            return false;
        }
    }

    if (isInhibitWall()) {
        return false;
    }
    if (mMovementStates._8 && calcPolygonAngleD(mFrontWallTriangle) < 80.0f) {
        return false;
    }
    if (mMovementStates._19 && calcPolygonAngleD(mBackWallTriangle) < 80.0f) {
        return false;
    }

    TVec3f wallSide;
    wallSide.cross(getWallNorm(), getAirGravityVec());
    if (MR::normalizeOrZero(&wallSide)) {
        return false;
    }

    if (!checkWallCode("NotWallSlip", true)) {
        mMovementStates._28 = true;
    }
    if (checkWallCode("NoAction", true)) {
        return false;
    }

    if ((getPlayerMode() != 3 || !getWallPolygon()->mSensor->isType(0x57)) && isRising()) {
        return false;
    }

    if (mMovementStates._19) {
        if (!isAnimationRun("壁ジャンプ") && checkStickWallSide() != 1) {
            return false;
        }
    } else if (!isAnimationRun("壁ジャンプ") && !mMovementStates._9 && checkStickWallSide() != 1) {
        return false;
    }

    if (mMovementStates._30) {
        return false;
    }

    if (mActor->_468 != 0) {
        if (!checkTrgA()) {
            return false;
        }
        mWall->initTriangleJump();
        mWall->startJump();
        return false;
    }

    TVec3f start(mActor->_2AC);
    f32 distance = 120.0f;
    if (mMovementStates._19) {
        distance = -120.0f;
    }
    TVec3f ray(mFrontVec);
    ray.scale(distance);
    if (!MR::isExistMapCollision(start, ray)) {
        return false;
    }
    return MR::getKarikariClingNum() == 0;
}

bool MarioWall::start() {
    _18 = 0;
    _20 = 0.0f;
    changeAnimation("壁くっつき", static_cast< const char* >(nullptr));
    startPadVib(static_cast< u32 >(0));
    getPlayer()->mMovementStates._28 = false;
    getPlayer()->_20_HIGH_WORD &= ~0x800000;
    _1D = false;
    initTriangleJump();
    _1E = false;

    if (getPlayerMode() == 3 && getPlayer()->getWallPolygon()->mSensor->isType(0x57)) {
        TVec3f wallOffset(getPlayer()->getWallNorm());
        wallOffset.scale(30.0f);
        TVec3f icePos(getPlayer()->getWallPos());
        icePos -= wallOffset;
        mActor->createIceWall(icePos, getPlayer()->getWallNorm());

        TVec3f effectOffset(getPlayer()->getWallNorm());
        effectOffset.scale(15.0f);
        TVec3f effectPos(getPlayer()->getWallPos());
        effectPos -= effectOffset;
        playEffectRT("氷壁ジャンプ", getPlayer()->getWallNorm(), effectPos);
        playSound("スケート着地", -1);
        _1E = true;
    }
    return true;
}

bool MarioWall::update() {
    bool cancel = false;
    if (mActor->_334 != 0 || MR::getKarikariClingNum() != 0) {
        return false;
    }

    if ((isStatusActiveID(MarioStatus_Rabbit) || static_cast< u32 >(mActor->_37C - getPlayer()->_558) < 6) && startJump()) {
        return false;
    }

    if (getPlayer()->mDrawStates._6) {
        if (_1D) {
            TVec3f horizontal;
            if (MR::vecKillElement(mActor->getLastMove(), getGravityVec(), &horizontal) < 1.0f) {
                cancel = true;
            }
        }
        getPlayer()->mMovementStates._1 = true;
        _1D = true;
    } else {
        _1D = false;
    }

    _14++;
    if (getPlayer()->mMovementStates._1 && !cancel) {
        if (getPlayer()->mVerticalSpeed < 80.0f) {
            if (!isOnSlipGround()) {
                getPlayer()->setFrontVecKeepUp(getPlayer()->getWallNorm());
                changeAnimation("着地", static_cast< const char* >(nullptr));
                changeAnimationInterpoleFrame(1);
                mActor->setBlendMtxTimer(4);
            }
            cancel = true;
        } else {
            getPlayer()->mMovementStates._1 = false;
        }
    }

    if (_14 >= static_cast< u32 >(mActor->mConst->getTable()->mWallStickTime
        + mActor->mConst->getTable()->mWallReleaseTime)) {
        cancel = true;
    }
    if (_14 >= 3 && !getPlayer()->mMovementStates._8 && !getPlayer()->mMovementStates._32) {
        cancel = true;
    }

    if (cancel) {
        if (!getPlayer()->mMovementStates._1) {
            _1C = true;
            TVec3f wallNorm(getPlayer()->getWallNorm());
            _24 = wallNorm;
            TVec3f front(-wallNorm);
            getPlayer()->setFrontVecKeepUp(front);
        }

        TVec3f pushVec(getPlayer()->getWallNorm());
        if (isOnSlipGround()) {
            pushVec.scale(10.0f);
        } else {
            pushVec.scale(30.0f);
        }
        addVelocityAfter(pushVec);
        return false;
    }

    if (checkTrgA() && startJump()) {
        if (_1E) {
            playSound("スケートジャンプ", -1);
        }
        return false;
    }

    if (mActor->isRequestRush()) {
        getPlayer()->mMovementStates._2B = false;
        changeAnimation("空中ひねり", static_cast< const char* >(nullptr));
        getPlayer()->tryWallPunch();
        getPlayer()->setWallCancel();
        return false;
    }

    if (!getPlayer()->fixWallingPosition(false)) {
        getPlayer()->setWallCancel();
        return false;
    }

    f32 damping = 0.9f;
    f32 dropSpeed = mActor->mConst->getTable()->mWallDropSpeedNormal;
    u8 side = getPlayer()->checkStickWallSide();
    if (side == 1) {
        side = 0;
    }

    switch (side) {
    case 1:
        damping = 0.7f;
        if (_14 > static_cast< u32 >(mActor->mConst->getTable()->mWallStickTime)) {
            _14 = mActor->mConst->getTable()->mWallStickTime;
        }
        _18 = 0;
        dropSpeed = mActor->mConst->getTable()->mWallDropSpeedStop;
        changeAnimation("壁くっつき", static_cast< const char* >(nullptr));
        stopEffect("共通壁手擦り");
        break;
    case 2:
        if (_1E) {
            const u32 slideStart = mActor->mConst->getTable()->mWallStickTimeIce - 15;
            if (_14 < slideStart) {
                _14 = slideStart;
            }
        } else if (_14 < 165) {
            _14 = 165;
        }
        if (_18 == 0) {
            _18 = 1;
        }
        stopEffect("共通壁手擦り");
        break;
    case 0:
        if (_14 < static_cast< u32 >(mActor->mConst->getTable()->mWallStickTime)) {
            stopEffect("共通壁手擦り");
        } else {
            if (!isAnimationRun("壁くっつき")) {
                changeAnimation("壁すべり", static_cast< const char* >(nullptr));
            }
            playSound("スリップ", -1);
            playEffect("共通壁手擦り");
        }
        break;
    }

    if (_18 != 0) {
        _18++;
    }

    if (_14 < static_cast< u32 >(mActor->mConst->getTable()->mWallStickTime)) {
        damping = 1.0f;
    }

    if (_1E) {
        if (_14 > static_cast< u32 >(mActor->mConst->getTable()->mWallStickTimeIce)) {
            _1C = true;
            _24 = getPlayer()->getWallNorm();
            return false;
        }
        return true;
    }

    _20 = _20 * damping + dropSpeed * (1.0f - damping);
    addVelocity(getPlayer()->_75C, -_20);

    if (side <= 1) {
        f32 ratio;
        if (_14 < static_cast< u32 >(mActor->mConst->getTable()->mWallStickTime)) {
            ratio = 0.0f;
        } else {
            ratio = 1.0f - static_cast< f32 >(_14 - mActor->mConst->getTable()->mWallStickTime)
                * mActor->mConst->getTable()->mWallSideMoveRatio;
        }

        if (ratio < 0.0f) {
            ratio = 0.0f;
        } else if (ratio > 1.0f) {
            ratio = 1.0f;
        }
        getPlayer()->moveWallSlide(ratio);
    }
    return true;
}

bool MarioWall::close() {
    stopAnimation("壁くっつき", static_cast< const char* >(nullptr));
    stopAnimation("壁すべり", static_cast< const char* >(nullptr));
    if (getPlayer()->mMovementStates._1) {
        changeAnimation(static_cast< const char* >(nullptr), "基本");
    }
    stopEffect("共通壁手擦り");
    getPlayer()->resetTornado();
    getPlayer()->mMovementStates._38 = false;
    return true;
}

bool MarioWall::startJump() {
    if (getPlayer()->isInhibitWall()) {
        return false;
    }

    TVec3f jump(getPlayer()->getWallNorm());
    _24 = jump;
    if (!getPlayer()->mDrawStates._3 && _14 < 15 && !MR::isNearZero(_30, 0.001f)) {
        TVec3f previous(-_30);
        const f32 dot = jump.dot(previous);
        if (dot < mActor->mConst->getTable()->mWallTriJumpMargin && dot > 0.0f) {
            TVec3f axis;
            axis.cross(previous, jump);
            Mtx rotation;
            PSMTXRotAxisRad(rotation, &axis, JMAAcosRadian(dot));
            PSMTXMultVec(rotation, &jump, &jump);
        }
    }

    jump.scale(mActor->mConst->getTable()->mWallJumpPowerXZ);
    const f32 verticalSpeed = -mActor->mConst->getTable()->mWallJumpPowerY;
    TVec3f vertical(getGravityVec());
    vertical.scale(verticalSpeed);
    jump += vertical;
    getPlayer()->tryWallJump(jump, true);
    playEffect("共通壁ジャンプ");
    _1C = true;
    getPlayer()->mMovementStates._2B = false;
    return true;
}

bool MarioWall::startBackJump(u32 type) {
    if (getPlayer()->isInhibitWall()) {
        return false;
    }

    TVec3f jump(getPlayer()->getWallNorm());
    if (getPlayer()->mMovementStates.jumping && jump.dot(getPlayer()->mJumpVec) > 0.0f) {
        return false;
    }

    _24 = jump;
    jump.scale(mActor->mConst->getTable()->mWallBackJumpPowerXZ);
    TVec3f vertical(getGravityVec());
    vertical.scale(-mActor->mConst->getTable()->mWallBackJumpPowerY);
    jump += vertical;
    getPlayer()->tryWallJump(jump, false);

    switch (type) {
    case 0:
        playEffectRTZ("結界ヒット", _24, getPlayer()->getWallPos());
        playSound("結界ヒット", -1);
        getPlayer()->_402 = 0;
        getPlayer()->_428 = 60;
        break;
    case 1:
        playEffectRTZ("水壁ヒット", _24, getPlayer()->getWallPos());
        playSound("水弾かれ", -1);
        break;
    case 2:
        playSound("トランポリンジャンプ大", -1);
        break;
    }

    startPadVib(2);
    _1C = true;
    getPlayer()->mMovementStates._2B = true;
    return true;
}

bool Mario::fixWallingDist() {
    if (mMovementStates._19 && mMovementStates._8) {
        return true;
    }

    if (mMovementStates._19) {
        TVec3f normal(*mBackWallTriangle->getNormal(0));
        normal.scale(79.0f);
        TVec3f target(_4F4);
        target += normal;
        TVec3f offset(target);
        offset -= mPosition;
        const f32 distance = MR::vecKillElement(offset, *mBackWallTriangle->getNormal(0), &target);
        TVec3f correction(*mBackWallTriangle->getNormal(0));
        correction.scale(distance);
        mPosition += correction;

        if (!fixWallingTop()) {
            return false;
        }

        TVec3f top(_75C);
        top.scale(80.0f);
        TVec3f wall(*mBackWallTriangle->getNormal(0));
        wall.scale(60.0f);
        TVec3f position(_4F4);
        position += wall;
        TVec3f fixed(position);
        fixed -= top;
        mPosition = fixed;
    } else if (mMovementStates._8) {
        if (!fixWallingTop()) {
            return false;
        }

        TVec3f ray(mFrontVec);
        ray.scale(100.0f);
        if (MR::isExistMapCollision(mPosition, ray) && mFrontVec.dot(*mFrontWallTriangle->getNormal(0)) < -0.999f) {
            TVec3f top(_75C);
            top.scale(80.0f);
            TVec3f wall(*mFrontWallTriangle->getNormal(0));
            wall.scale(60.0f);
            TVec3f position(_4E8);
            position += wall;
            TVec3f target(position);
            target -= top;
            TVec3f correction(target);
            correction -= mPosition;

            TVec3f horizontal;
            MR::vecKillElement(correction, *getGravityVec(), &horizontal);
            if (!MR::isNearZero(horizontal, 0.001f)) {
                TVec3f moved(horizontal);
                moved += mActor->_288;
                if (__fabsf(moved.x) < __fabsf(horizontal.x)) {
                    horizontal.x = moved.x;
                }
                if (__fabsf(moved.y) < __fabsf(horizontal.y)) {
                    horizontal.y = moved.y;
                }
                if (__fabsf(moved.z) < __fabsf(horizontal.z)) {
                    horizontal.z = moved.z;
                }
                addTrans(horizontal, "壁補正");
            }
        }
    }
    return true;
}

bool Mario::isInhibitWall() const {
    if (mDrawStates._2) {
        return true;
    }
    if (checkWallCode("NotWallJump", true)) {
        return true;
    }
    return checkWallCode("NoAction", true);
}

void Mario::tryWallPunch() {
    if (mMovementStates._2B || checkWallCodeNorm(8, nullptr, false)
        || (isStatusActive(MarioStatus_Skate) && checkWallFloorCode(33))) {
        return;
    }

    if (isSwimming()) {
        if (mMovementStates._8) {
            mSwim->hitWall(*mFrontWallTriangle->getNormal(0), mFrontWallTriangle->mSensor);
        }
        if (mMovementStates._19) {
            mSwim->hitWall(*mBackWallTriangle->getNormal(0), mBackWallTriangle->mSensor);
        }
        if (mMovementStates._1A) {
            mSwim->hitWall(*mSideWallTriangle->getNormal(0), mSideWallTriangle->mSensor);
        }
        return;
    }

    stopWalk();
    TVec3f jump(getWallNorm());
    jump.scale(mActor->mConst->getTable()->mWallSpinFlipGround);
    if (mMovementStates.jumping) {
        jump.scale(mActor->mConst->getTable()->mWallSpinFlipAirRatio);
    }
    TVec3f gravity(*getGravityVec());
    gravity.scale(mActor->mConst->getTable()->mWallSpinHopGround);
    jump -= gravity;
    tryForcePowerJump(jump, true);
    mMovementStates._2B = true;
    startPadVib(2);
    playSound("壁反射", -1);
    playSound("声スピンキャンセル", -1);
    playEffectTrans("壁ヒット", getWallPos());

    if (mMovementStates._8) {
        sendPunch(mFrontWallTriangle->mSensor, true);
    }
    if (mMovementStates._19) {
        sendPunch(mBackWallTriangle->mSensor, true);
    }
    if (mMovementStates._1A) {
        sendPunch(mSideWallTriangle->mSensor, true);
    }
    mActor->_EF6 = 30;
}

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

bool MarioWall::notice() {
    return false;
}
