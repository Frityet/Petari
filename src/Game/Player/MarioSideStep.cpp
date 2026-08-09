#include "Game/Player/MarioSideStep.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioWall.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

bool Mario::checkWallJumpCode() {
    if (checkWallFloorCode(11) || checkWallFloorCode(6) || checkWallFloorCode(34) || checkWallFloorCode(7) || checkWallFloorCode(8)) {
        if (mMovementStates._1 || isAnimationRun(_728)) {
            stopWalk();

            TVec3f pushVec(getWallNorm());
            pushVec *= 10.0f;
            push(pushVec);
            tryPushToVelocity();

            mJumpVec.set(-getPlayer()->getWallNorm() * 10.0f);
            MR::vecKillElement(mJumpVec, getAirGravityVec(), &mJumpVec);

            TVec3f gravity(getAirGravityVec());
            gravity *= 10.0f;
            mJumpVec -= gravity;
        }

        mWall->initTriangleJump();

        u32 jumpType = -1;
        if (checkWallFloorCode(7)) {
            jumpType = 1;
        }
        if (checkWallFloorCode(6)) {
            jumpType = 0;
        }
        if (checkWallFloorCode(34)) {
            jumpType = 2;
        }

        if (mWall->startBackJump(jumpType)) {
            return true;
        }
    }

    return false;
}

MarioSideStep::MarioSideStep(MarioActor* pActor) : MarioState(pActor, MarioStatus_SideStep) {
}

bool MarioSideStep::close() {
    stopAnimation("壁ウエイト");
    stopAnimation("壁右歩き");
    stopAnimation("壁左歩き");
    stopAnimation("壁押し");
    getPlayer()->startBas(nullptr, false, 0.0f, 0.0f);
    return true;
}

bool Mario::doSideStep() {
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

    if (_72C < 200.0f) {
        return false;
    }

    if (getPlayerMode() == 5) {
        return false;
    }

    if (calcPolygonAngleD(mFrontWallTriangle) > 91.0f) {
        return false;
    }

    const f32 wallAngle = MR::diffAngleAbs(getFrontWallNorm(), _368);
    if (wallAngle >= 1.6534699f || wallAngle < 1.4959966f) {
        return false;
    }

    const bool isNotSlipFloor = !isSlipFloorCode(_960);
    if (isNotSlipFloor) {
        return false;
    }

    if (_960 == 2) {
        return false;
    }

    if (!getPlayer()->mMovementStates._8) {
        return false;
    }

    const Triangle* pWall = getPlayer()->getWallPolygon();
    if (pWall == nullptr) {
        return false;
    }

    if (!MR::isSameMtx(*pWall->getBaseMtx(), *pWall->getPrevBaseMtx())) {
        return false;
    }

    stopWalk();
    forceStopTornado();
    changeStatus(mSideStep);
    return true;
}

bool MarioSideStep::start() {
    if (!isAnimationRun("壁押し")) {
        playSound("声壁押し", -1);
    }

    changeAnimation("壁押し", static_cast< u32 >(0));
    mActor->setBlendMtxTimer(10);
    return true;
}

bool MarioSideStep::update() {
    if (!getPlayer()->mMovementStates._8) {
        return false;
    }

    if (getPlayer()->_1C._5) {
        return false;
    }

    if (getPlayer()->mDrawStates._A) {
        return false;
    }

    if (MR::isNormalTalking()) {
        return false;
    }

    if (mActor->_468 != 0) {
        return false;
    }

    Mario* pPlayer = getPlayer();
    const bool isNotSlipFloor = !isSlipFloorCode(pPlayer->_960);
    if (isNotSlipFloor) {
        return false;
    }

    if (getStickP() < 0.1f) {
        changeAnimation("壁ウエイト", static_cast< const char* >(nullptr));
    } else {
        TVec3f padDir(getWorldPadDir());
        MR::vecKillElement(padDir, getPlayer()->getAirGravityVec(), &padDir);
        MR::normalizeOrZero(&padDir);

        TVec3f moveDir;
        const f32 wallElement = MR::vecKillElement(padDir, getPlayer()->getWallNorm(), &moveDir);
        if (wallElement < -0.866f) {
            if (!isAnimationRun("壁押し")) {
                playSound("声壁押し", -1);
            }

            changeAnimation("壁押し", static_cast< u32 >(0));
        } else if (wallElement > 0.0f) {
            if (wallElement > 0.707f) {
                const TVec3f* pWallNorm = &getPlayer()->getWallNorm();
                getPlayer()->setFrontVecKeepUp(*pWallNorm);
            }

            getPlayer()->mMovementStates._8 = false;
            return false;
        } else {
            moveDir *= getStickP() * (1.0f + 0.25f * wallElement);

            if (moveDir.dot(getPlayer()->mSideVec) < 0.0f) {
                if (!isAnimationRun("壁右歩き")) {
                    changeAnimation("壁右歩き", static_cast< const char* >(nullptr));
                }
            } else if (!isAnimationRun("壁左歩き")) {
                changeAnimation("壁左歩き", static_cast< const char* >(nullptr));
            }

            addVelocity(moveDir * 6.0f - getPlayer()->getWallNorm() * 6.0f);
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
        getPlayer()->keepDistFrontWall();
        getPlayer()->tryDrop();
        return false;
    }

    return !!getPlayer()->fixWallingPosition(false);
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
