#include "Game/Player/MarioHang.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include <cstring>
#include <revolution/mtx.h>

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

bool Mario::isHanging() const {
    return getCurrentStatus() == MarioStatus_Hang;
}

bool Mario::fixHangDir(const TVec3f& rPosition, TVec3f* pDirection) {
    TVec3f direction(rPosition - mPosition);
    MR::vecKillElement(direction, *getGravityVec(), &direction);
    MR::normalizeOrZero(&direction);

    Triangle triangle;
    TVec3f line(direction * 150.0f);
    if (MR::getFirstPolyOnLineToMap(nullptr, &triangle, mPosition, line)) {
        *pDirection = -*triangle.getNormal(0);
        mHang->recordWallPolygon(&triangle);
        return true;
    }

    if (pDirection->dot(direction) < 0.0f) {
        *pDirection = direction;
    }
    return false;
}

bool Mario::isEnableBackHang() {
    if (!_474->isValid()) {
        return false;
    }

    bool useCurrentPosition = false;
    if (_430 == 6) {
        if (isRising()) {
            return false;
        }
        useCurrentPosition = true;
    } else if (!mMovementStates._30) {
        return false;
    }

    if (!mMovementStates._19 || mMovementStates._B) {
        return false;
    }
    if (checkWallCode("NotGrab", false)) {
        return false;
    }

    const char* wallCode = MR::getWallCodeString(_474);
    if (wallCode != nullptr && strcmp(wallCode, "NotGrab") == 0) {
        return false;
    }
    if (isRising()) {
        return false;
    }

    const MarioConstTable* table = mActor->mConst->getTable();
    if (mStickPos.z > table->mWallBackHangStickPower || mWalkSpeed > table->mWallBackHangWalkSpeed) {
        return false;
    }

    if (getPlayer()->getShadowNorm().dot(*getGravityVec()) < -0.707f) {
        TVec3f groundPosition;
        if (useCurrentPosition) {
            groundPosition = mPosition;
        } else {
            getLastGroundPos(&groundPosition);
        }

        TVec3f horizontal;
        if (MR::vecKillElement(mShadowPos - groundPosition, *getGravityVec(), &horizontal) < table->mWallHangGrHeight) {
            return false;
        }
    }

    if (useCurrentPosition) {
        setFrontVecKeepUp(-mFrontVec);
        return false;
    }

    if ((_4F4 - mGroundPos).dot(*getGravityVec()) < 0.0f) {
        return false;
    }

    TVec3f groundPosition;
    getLastGroundPos(&groundPosition);
    TVec3f horizontal;
    MR::vecKillElement(groundPosition - mPosition, getAirGravityVec(), &horizontal);
    return horizontal.length() < 84.0f;
}

bool Mario::isEnableSideHang() {
    if (_1C._1 || _1C._2) {
        return false;
    }
    if (!mMovementStates._30 || !mMovementStates._1A) {
        return false;
    }
    if (checkWallCode("NotGrab", false) || checkWallCode("NoAction", false)) {
        return false;
    }

    const char* wallCode = MR::getWallCodeString(_474);
    if (wallCode != nullptr && (strcmp(wallCode, "NotGrab") == 0 || strcmp(wallCode, "NoAction") == 0)) {
        return false;
    }
    if (isRising()) {
        return false;
    }

    const MarioConstTable* table = mActor->mConst->getTable();
    if (mStickPos.z > table->mWallBackHangStickPower || mWalkSpeed > table->mWallBackHangWalkSpeed) {
        return false;
    }

    if (getPlayer()->getShadowNorm().dot(*getGravityVec()) < -0.707f) {
        TVec3f groundPosition;
        getLastGroundPos(&groundPosition);
        TVec3f horizontal;
        if (MR::vecKillElement(mShadowPos - groundPosition, *getGravityVec(), &horizontal) < table->mWallHangGrHeight) {
            return false;
        }
    }

    if (!mMovementStates._33) {
        return false;
    }
    if ((_500 - mGroundPos).dot(*getGravityVec()) < -20.0f) {
        return false;
    }

    TVec3f groundPosition;
    getLastGroundPos(&groundPosition);
    TVec3f horizontal;
    MR::vecKillElement(groundPosition - mPosition, getAirGravityVec(), &horizontal);
    return horizontal.length() < 84.0f;
}

void MarioHang::recordWallPolygon(const Triangle* pTriangle) {
    Triangle* wallTriangle = _20;
    wallTriangle->mParts = pTriangle->mParts;
    wallTriangle->mIdx = pTriangle->mIdx;
    wallTriangle->mSensor = pTriangle->mSensor;
    wallTriangle->mNormals[0] = pTriangle->mNormals[0];
    wallTriangle->mNormals[1] = pTriangle->mNormals[1];
    wallTriangle->mNormals[2] = pTriangle->mNormals[2];
    wallTriangle->mNormals[3] = pTriangle->mNormals[3];
    wallTriangle->mPos[0] = pTriangle->mPos[0];
    wallTriangle->mPos[1] = pTriangle->mPos[1];
    wallTriangle->mPos[2] = pTriangle->mPos[2];
    mWallSensor = pTriangle->mSensor;

    TPos3f inverse;
    MR::makeMtxWithoutScale(&inverse, *_20->getBaseInvMtx());
    PSMTXMultVecSR(inverse.toMtxPtr(), _20->getNormal(0), &_28);
    _34 = *_20->getNormal(0);
}

void MarioHang::recordHangNorm(const TVec3f& rNormal) {
    _34 = rNormal;
    TPos3f inverse;
    MR::makeMtxWithoutScale(&inverse, *_20->getBaseInvMtx());
    PSMTXMultVecSR(inverse.toMtxPtr(), &rNormal, &_28);
}

void MarioHang::forceDrop() {
    _1C = true;
}

MarioHang::MarioHang(MarioActor* pActor) : MarioState(pActor, MarioStatus_Hang) {
    _12 = 0;
    _14 = 0;
    _16 = 0;
    mHangTimer = 0;
    _1A = false;
    _1B = false;
    _1C = false;
    _1D = false;
    _1E = false;
    _28.zero();
    _34.zero();
    mWallSensor = nullptr;
    _20 = new Triangle();
    _24 = new Triangle();
}

bool MarioHang::close() {
    stopAnimation("崖つかまり開始", static_cast< const char* >(nullptr));
    stopAnimation("崖つかまり中", static_cast< const char* >(nullptr));
    stopAnimation("崖つかまり終了", static_cast< const char* >(nullptr));
    stopAnimation("崖つかまり終了坂", static_cast< const char* >(nullptr));

    if (getPlayer()->mMovementStates._1) {
        changeAnimation(static_cast< const char* >(nullptr), "基本");
    } else {
        changeAnimation(static_cast< const char* >(nullptr), "落下");
    }

    getPlayer()->setWallCancel();
    mHangTimer = 120;
    if (_1A) {
        getPlayer()->tryHangSlipUp();
    } else {
        getPlayer()->_10._F = true;
    }
    mActor->_F44 = true;
    return true;
}

bool MarioHang::notice() {
    if (getNoticedStatus() == MarioStateMsg_Notice) {
        addTrans(getPlayer()->mFrontVec * -150.0f, "Module");
        mActor->setBlendMtxTimer(16);
        stopAnimation("崖つかまり中", "基本");
    }
    return false;
}

bool MarioHang::postureCtrl(MtxPtr pMtx) {
    TVec3f check(getPlayer()->mHeadVec);
    check += _34;
    if (MR::isNearZero(check, 0.001f)) {
        return false;
    }

    if (_12 == 0) {
        if (MR::isSameDirection(getPlayer()->_368, _34, 0.01f)) {
            return false;
        }
        MR::makeMtxFrontUp(reinterpret_cast< TPos3f* >(pMtx), -_34, getPlayer()->_368);
        return true;
    }

    f32 blend = static_cast< f32 >(_14) / 30.0f;
    if (blend < 0.0f) {
        blend = 0.0f;
    } else if (blend > 1.0f) {
        blend = 1.0f;
    }

    TVec3f up;
    MR::vecBlendSphere(getPlayer()->_368, getPlayer()->mHeadVec, &up, blend);
    if (MR::isSameDirection(up, _34, 0.01f)) {
        return false;
    }
    MR::makeMtxFrontUp(reinterpret_cast< TPos3f* >(pMtx), -_34, up);
    return true;
}

void Mario::checkHang() {
    if (mHang->mHangTimer != 0) {
        return;
    }

    if ((mMovementStates.jumping && isRising()) || (mMovementStates.jumping && _3BC > 20)) {
        mMovementStates._30 = false;
    }
    if (isCeiling()) {
        mMovementStates._31 = false;
    }
    if (!mMovementStates._31 || getPlayer()->mDrawStates.mIsUnderwater || isStatusActive(MarioStatus_Rabbit) || mActor->_468 != 0 ||
        getPlayerMode() == 4 || getPlayerMode() == 6) {
        return;
    }

    bool sideBaseStable = true;
    bool frontAndSideStable = true;
    bool frontAndBackStable = true;
    if (mMovementStates._1A && !MR::isSameMtx(*mSideWallTriangle->getBaseMtx(), *mSideWallTriangle->getPrevBaseMtx())) {
        sideBaseStable = false;
        frontAndSideStable = false;
    }
    if (mMovementStates._8 && !MR::isSameMtx(*mFrontWallTriangle->getBaseMtx(), *mFrontWallTriangle->getPrevBaseMtx())) {
        frontAndBackStable = false;
        frontAndSideStable = false;
    }
    if (mMovementStates._19 && !MR::isSameMtx(*mBackWallTriangle->getBaseMtx(), *mBackWallTriangle->getPrevBaseMtx())) {
        sideBaseStable = false;
        frontAndBackStable = false;
    }

    if (sideBaseStable && mMovementStates._19 && isEnableHang()) {
        TVec3f towardHang(_4A4 - mPosition);
        MR::vecKillElement(towardHang, getAirGravityVec(), &towardHang);
        MR::normalizeOrZero(&towardHang);
        if (getFrontWallNorm().dot(towardHang) >= -0.8f) {
            return;
        }

        TVec3f line(mFrontVec * 60.0f);
        TVec3f start(_4A4 - mFrontVec * 50.0f - getAirGravityVec() * 5.0f);
        if (!MR::isExistMapCollision(start, line)) {
            const bool enteredFromFall = !isAnimationRun("落下");
            changeStatus(mHang);
            mVelocity.zero();
            stopWalk();
            stopJump();
            setTrans(_4A4, static_cast< const char* >(nullptr));
            setFrontVecKeepUp(-getWallNorm());
            mHang->recordWallPolygon(mFrontWallTriangle);
            mHang->_1E = enteredFromFall;
        }
    } else if (frontAndSideStable && isEnableBackHang()) {
        TVec3f hangPosition;
        getLastGroundPos(&hangPosition);
        if ((mPosition - hangPosition).dot(getAirGravityVec()) >= 160.0f) {
            return;
        }

        TVec3f edgeDirection(-mFrontVec);
        u32 edge = getLastGroundEdgeIndex(mActor->_2A0, edgeDirection);
        edgeDirection = _4F4 - mActor->_2A0;
        MR::normalizeOrZero(&edgeDirection);
        const u32 positionEdge = getLastGroundEdgeIndex(mActor->_2A0, edgeDirection);
        edgeDirection = -*mBackWallTriangle->getNormal(0);
        const u32 wallEdge = getLastGroundEdgeIndex(mActor->_2A0, edgeDirection);
        if (edge != positionEdge || edge != wallEdge) {
            return;
        }

        TVec3f hangNormal(*getLastGroundEdgeNrm(edge));
        if (hangNormal.dot(getAirGravityVec()) >= 0.1f || hangNormal.dot(mHeadVec) >= 0.1f) {
            return;
        }
        edgeDirection = -*mBackWallTriangle->getNormal(0);
        if (MR::diffAngleAbsHorizontal(hangNormal, -edgeDirection, getAirGravityVec()) > 0.08726647f) {
            return;
        }

        f32 edgeDistance = 0.0f;
        if (edge < 2) {
            TVec3f offset(*_474->calcAndGetPos(edge == 0 ? 0 : 1) - hangPosition);
            TVec3f horizontal;
            edgeDistance = MR::vecKillElement(offset, hangNormal, &horizontal);
            hangPosition += hangNormal * edgeDistance;
        }
        if (edgeDistance > 100.0f) {
            return;
        }

        MR::vecKillElement(hangNormal, getAirGravityVec(), &hangNormal);
        MR::normalizeOrZero(&hangNormal);
        TVec3f line(edgeDirection * 60.0f);
        TVec3f start(hangPosition - edgeDirection * 50.0f - getAirGravityVec() * 5.0f);
        if (!MR::isExistMapCollision(start, line)) {
            changeStatus(mHang);
            mVelocity.zero();
            stopWalk();
            stopJump();
            TVec3f fixedDirection;
            bool foundWall = fixHangDir(hangPosition, &fixedDirection);
            setTrans(hangPosition, static_cast< const char* >(nullptr));
            setFrontVecKeepUp(hangNormal);
            mMovementStates._1D = true;
            if (!foundWall) {
                mHang->recordWallPolygon(mBackWallTriangle);
            }
            mHang->recordHangNorm(hangNormal);
            mHang->_1E = false;
        }
    } else if (frontAndBackStable && isEnableSideHang()) {
        TVec3f hangPosition;
        getLastGroundPos(&hangPosition);
        if ((mPosition - hangPosition).dot(getAirGravityVec()) >= 160.0f) {
            return;
        }

        TVec3f edgeDirection(_500 - mActor->_2A0);
        MR::normalizeOrZero(&edgeDirection);
        u32 edge = getLastGroundEdgeIndex(mActor->_2A0, edgeDirection);
        TVec3f side(mSideVec);
        edgeDirection = side.dot(edgeDirection) < 0.0f ? -side : side;
        const u32 positionEdge = getLastGroundEdgeIndex(mActor->_2A0, edgeDirection);
        edgeDirection = -*mSideWallTriangle->getNormal(0);
        const u32 wallEdge = getLastGroundEdgeIndex(mActor->_2A0, edgeDirection);
        if (edge != positionEdge || edge != wallEdge) {
            return;
        }

        TVec3f hangNormal(*getLastGroundEdgeNrm(edge));
        if (hangNormal.dot(getAirGravityVec()) >= 0.1f || hangNormal.dot(mHeadVec) >= 0.1f) {
            return;
        }
        edgeDirection = -*mSideWallTriangle->getNormal(0);
        if (MR::diffAngleAbsHorizontal(hangNormal, -edgeDirection, getAirGravityVec()) > 0.08726647f) {
            return;
        }

        MR::vecKillElement(hangNormal, getAirGravityVec(), &hangNormal);
        MR::normalizeOrZero(&hangNormal);
        TVec3f line(edgeDirection * 60.0f);
        TVec3f start(hangPosition - edgeDirection * 50.0f - getAirGravityVec() * 5.0f);
        if (!MR::isExistMapCollision(start, line)) {
            changeStatus(mHang);
            mVelocity.zero();
            stopWalk();
            stopJump();
            TVec3f fixedDirection;
            bool foundWall = fixHangDir(hangPosition, &fixedDirection);
            setTrans(hangPosition, static_cast< const char* >(nullptr));
            setFrontVecKeepUp(hangNormal);
            mMovementStates._1D = true;
            if (!foundWall) {
                mHang->recordWallPolygon(mSideWallTriangle);
            }
            mHang->recordHangNorm(hangNormal);
            mHang->_1E = false;
        }
    }

    if (isStatusActive(MarioStatus_Hang)) {
        _414 = 0;
        for (u32 i = 0; i < 10; i++) {
            updateGroundInfo();
            if (mMovementStates._1) {
                break;
            }
            addTrans(mFrontVec - getAirGravityVec(), static_cast< const char* >(nullptr));
        }
    }
}

bool Mario::isEnableHang() {
    if (!mMovementStates._8 || !mMovementStates._15 || mMovementStates._B) {
        return false;
    }
    if (mDrawStates._1E || isStatusActive(MarioStatus_Recovery)) {
        return false;
    }
    if (checkWallCode("NotGrab", false) || checkWallCode("NoAction", false)) {
        return false;
    }

    const char* wallCode = MR::getWallCodeString(_47C);
    if (wallCode != nullptr && (strcmp(wallCode, "NotGrab") == 0 || strcmp(wallCode, "NoAction") == 0)) {
        return false;
    }

    const MarioConstTable* table = mActor->mConst->getTable();
    TVec3f negGravity(-getAirGravityVec());
    f32 normalDot = _45C->getNormal(0)->dot(negGravity);
    f32 heightLimit = table->mWallHangGrHeight;
    if (normalDot > 0.0f) {
        if (normalDot > 0.2f) {
            normalDot = 0.2f;
        }
        heightLimit -= 150.0f * normalDot;
    }

    if (getPlayer()->getShadowNorm().dot(*getGravityVec()) < -0.707f) {
        if ((_4A4 - mShadowPos).dot(-*getGravityVec()) < heightLimit) {
            return false;
        }
    }

    if ((_4A4 - mPosition).dot(-*getGravityVec()) > table->mWallHangMyHeight) {
        return false;
    }
    return !isRising();
}

bool MarioHang::start() {
    getPlayer()->cancelSquatMode();
    stopAnimationUpper(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
    changeAnimation("崖つかまり開始", "崖つかまり中");
    mActor->setBlendMtxTimer(mActor->mConst->getTable()->mHangBlendTime);
    _12 = 0;
    _14 = 0;
    _16 = 0;
    _1B = false;
    _1C = false;
    _1D = false;
    _1A = false;
    Mario* player = getPlayer();
    player->_74C = 0.0f;
    player->_750 = 0;
    player->_754 = 0;
    mActor->_F44 = false;
    return true;
}

bool MarioHang::update() {
    getPlayer()->mDrawStates._A = false;

    TPos3f base;
    base.identity();
    MR::makeMtxWithoutScale(&base, *_20->getBaseMtx());
    TVec3f wallNormal;
    PSMTXMultVecSR(base.toMtxPtr(), &_28, &wallNormal);
    getPlayer()->setFrontVecKeepUp(-wallNormal);
    _34 = wallNormal;
    _14++;

    if (_16 != 0) {
        _16--;
        if (_16 != 0) {
            return true;
        }
        _1A = true;
    }
    if (_1A) {
        return false;
    }

    if (!MR::isSameMtx(*_20->getBaseMtx(), *_20->getPrevBaseMtx())) {
        if (getPlayer()->mMovementStates._1A && _14 > 2 && mActor->getLastMove().dot(getPlayer()->getSideWallNorm()) < 0.0f) {
            _1C = true;
        }
        if (getPlayer()->_184.dot(getFrontVec()) > 0.95f && getPlayer()->_4E4 < 6.0f && !isAnimationRun("崖つかまり開始")) {
            _1C = true;
        }
    } else if (getPlayer()->_8D4 != nullptr) {
        addTrans(getPlayer()->mFrontVec * -150.0f, "Module");
        mActor->setBlendMtxTimer(16);
        stopAnimation("崖つかまり中", "基本");
        return false;
    }

    if ((getPlayer()->_8D4 != nullptr && mWallSensor != getPlayer()->_8D4) || _1C) {
        addTrans(getPlayer()->mFrontVec * -150.0f, "Module");
        mActor->setBlendMtxTimer(16);
        stopAnimation("崖つかまり中", "基本");
        return false;
    }

    if (_12 == 1) {
        if (calcAngleD(getPlayer()->_368) <= 45.0f) {
            changeAnimation("崖つかまり終了", "基本");
        } else {
            changeAnimationNonStop("崖つかまり終了坂");
            changeAnimation(static_cast< const char* >(nullptr), "基本");
        }
        playSound("声崖つかまり終了", -1);
        _12++;
        return true;
    }

    if (_12 == 0) {
        if (getPlayer()->mVerticalSpeed < 0.1f && getGravityVec().dot(getPlayer()->getShadowNorm()) < -0.707f) {
            addTrans(getPlayer()->mFrontVec * -150.0f, "Module");
            mActor->setBlendMtxTimer(16);
            stopAnimation("崖つかまり中", "基本");
            return false;
        }
        if (!getPlayer()->mMovementStates._1 || getPlayer()->calcDistToCeilHead() < 80.0f) {
            _1C = true;
        }
    } else if (_12 == 2) {
        if (!getPlayer()->mMovementStates._1) {
            addVelocity(getFrontVec() * -100.0f);
        }
        if (!isAnimationRun(static_cast< const char* >(nullptr)) || isAnimationTerminate(static_cast< const char* >(nullptr))) {
            getPlayer()->mMovementStates._1 = true;
            return false;
        }
        return true;
    }

    if (_1C) {
        addTrans(getPlayer()->mFrontVec * -150.0f, "Module");
        mActor->setBlendMtxTimer(16);
        stopAnimation("崖つかまり中", "基本");
        return false;
    }

    if (checkTrgA()) {
        tryClimb(true);
    }
    if (getPlayer()->mMovementStates._2) {
        return true;
    }

    u8 stickDirection = 0;
    if (_16 == 0) {
        stickDirection = getPlayer()->checkStickFrontBack();
        if (getStickP() < 0.2f) {
            stickDirection = 0;
        }
        if (_1D) {
            stickDirection = 2;
        }
    }

    if (stickDirection == 2) {
        if (isAnimationRun("崖つかまり開始") && getAnimator()->getFrame() < 15.0f) {
            _1D = true;
        } else {
            addTrans(getPlayer()->mFrontVec * -150.0f, "Module");
            mActor->setBlendMtxTimer(16);
            getPlayer()->_3C0 = 0;
            getPlayer()->mMovementStates._31 = false;
            getPlayer()->mMovementStates._1 = false;
            getPlayer()->tryDrop();
            getPlayer()->_3CA = 120;
            return false;
        }
    } else if (stickDirection == 1 && _14 > 18) {
        tryClimb(false);
    }

    if (_1B && _12 == 0 && _14 > 18) {
        _12 = 1;
        _14 = 0;
    }
    if (_12 == 0 && _14 == 2) {
        playSound("声崖つかまり", -1);
    }
    return true;
}

void MarioHang::tryClimb(bool allowSlipUp) {
    if (_12 != 0 || getPlayer()->calcDistToCeil(false) < 160.0f) {
        return;
    }

    Triangle triangle;
    TVec3f line(getFrontVec() * 60.0f);
    TVec3f start(getTrans() - getGravityVec() * 30.0f);
    if (MR::getFirstPolyOnLineToMap(nullptr, &triangle, start, line)) {
        return;
    }

    u32 attempt = 0;
    for (; attempt < 3; attempt++) {
        Triangle sideTriangle;
        TVec3f sideLine(getPlayer()->mSideVec * 55.0f);
        TVec3f sideOffset(getPlayer()->mSideVec * 15.0f);
        TVec3f gravityOffset(getGravityVec() * 30.0f);
        TVec3f leftStart(getTrans() - gravityOffset - sideOffset);
        bool leftHit = MR::getFirstPolyOnLineToMap(nullptr, &sideTriangle, leftStart, sideLine);

        TVec3f rightStart(getTrans() - gravityOffset + sideOffset);
        TVec3f rightLine(getPlayer()->mSideVec * -55.0f);
        bool rightHit = MR::getFirstPolyOnLineToMap(nullptr, &sideTriangle, rightStart, rightLine);
        if (leftHit && rightHit) {
            _1C = true;
            return;
        }
        if (leftHit) {
            addTrans(getPlayer()->mSideVec * -15.0f, "Module");
        } else if (rightHit) {
            addTrans(getPlayer()->mSideVec * 15.0f, "Module");
        } else {
            break;
        }
    }

    if (attempt == 3) {
        _1C = true;
        return;
    }

    if (!isAnimationRun("崖つかまり開始")) {
        _12 = 1;
        _14 = 0;
    } else {
        if (_1E && allowSlipUp && getAnimationFrame() < 30.0f) {
            _16 = 5;
            changeAnimationNonStop("つかまりスリップアップ準備");
        }
        _1B = true;
    }
}
