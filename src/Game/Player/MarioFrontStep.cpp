#include "Game/Player/MarioFrontStep.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

bool Mario::doFrontStep() {
    if (getCurrentStatus() == MarioStatus_FrontStep) {
        return false;
    }

    if (getCurrentStatus() == MarioStatus_SideStep) {
        return false;
    }

    if (checkWallCode("NotSideStep", false)) {
        return false;
    }

    if (checkWallCode("NoAction", false)) {
        return false;
    }

    if (mActor->_468 != 0) {
        return false;
    }

    if (_1C._5) {
        return false;
    }

    if (mMovementStates.jumping) {
        return false;
    }

    if (getPlayerMode() == 5) {
        return false;
    }

    if (_960 == 2) {
        return false;
    }

    if (calcAngleD(_368) >= 30.0f) {
        return false;
    }

    if (mAirGravityVec.dot(_374) >= 0.25f) {
        return false;
    }

    Triangle triangle;
    const TVec3f& checkStart = mActor->_2A0;
    TVec3f checkOffset = mFrontVec * 200.0f;
    if (!MR::getFirstPolyOnLineToMap(nullptr, &triangle, checkStart, checkOffset)) {
        return false;
    }

    setFrontVecKeepUp(-*MR::getNormal(&triangle));
    stopWalk();
    forceStopTornado();
    changeStatus(mFrontStep);
    return true;
}

bool MarioFrontStep::start() {
    changeAnimation("前壁ウエイト", static_cast<const char*>(nullptr));
    mActor->setBlendMtxTimer(10);
    getPlayer()->lockGroundCheck(this, true);
    return true;
}

MarioFrontStep::MarioFrontStep(MarioActor* pActor) : MarioState(pActor, MarioStatus_FrontStep) {
}

bool MarioFrontStep::update() {
    getPlayer()->stopWalk();

    if (getPlayer()->_1C._5) {
        return false;
    }

    const TVec3f& checkStart = mActor->_2A0;
    if (!MR::isExistMapCollision(checkStart, getFrontVec() * 200.0f)) {
        return false;
    }

    if (getStickP() < 0.1f) {
    }
    else {
        TVec3f padDir(getWorldPadDir());
        MR::vecKillElement(padDir, getPlayer()->getAirGravityVec(), &padDir);
        MR::normalizeOrZero(&padDir);

        TVec3f negFront = -getPlayer()->mFrontVec;
        TVec3f unused;
        f32 frontDot = MR::vecKillElement(padDir, negFront, &unused);
        if (frontDot < -0.86602575f) {
        }
        else if (frontDot > 0.0f) {
            return false;
        }
    }

    if (checkTrgA()) {
        Mario* pPlayer = getPlayer();
        pPlayer->_74C = 0.0f;
        pPlayer->_750 = 0;
        pPlayer->_754 = 0;
        getPlayer()->setFrontVecKeepUp(-getPlayer()->getWallNorm());
        getPlayer()->tryJump();
        return false;
    }

    if (mActor->isRequestRush()) {
        getPlayer()->tryWallPunch();
        return false;
    }

    if (!getPlayer()->mMovementStates._1) {
        getPlayer()->tryDrop();
        return false;
    }

    return true;
}

bool MarioFrontStep::close() {
    stopAnimation("前壁ウエイト", static_cast<const char*>(nullptr));
    getPlayer()->lockGroundCheck(this, false);
    return true;
}

bool MarioFrontStep::postureCtrl(MtxPtr pMtx) {
    TVec3f negFront = -getPlayer()->mFrontVec;
    TVec3f negGravity = -getPlayer()->getAirGravityVec();
    TVec3f up;

    if (MR::vecBlendSphere(negGravity, negFront, &up, 0.25f)) {
        MR::normalizeOrZero(&up);
    }
    else {
        up = -getPlayer()->getAirGravityVec();
    }

    MR::makeMtxUpFront(reinterpret_cast<TPos3f*>(pMtx), up, getPlayer()->mFrontVec);
    return true;
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
