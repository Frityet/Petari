#include "Game/Player/MarioWarp.hpp"
#include "Game/AreaObj/WarpCube.hpp"
#include "Game/MapObj/WarpPod.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/MathUtil.hpp"

namespace JGeometry {
    TVec3< f32 > TVec3< f32 >::operator*(f32) const NO_INLINE;
}

bool Mario::doObjWarp(LiveActor* pActor) {
    if (getPlayer()->getMovementStates().debugMode) {
        return false;
    }

    if (isStatusActive(MarioStatus_Warp)) {
        return false;
    }

    if (isStatusActive(MarioStatus_Warp)) {
        return false;
    }

    WarpPod* pPod = MR::getWarpPodManager()->getPairPod(pActor);

    if (pPod != nullptr) {
        mWarp->_4C = pActor;

        mVelocity.zero();

        stopWalk();

        mActor->setBlendMtxTimer(16);

        TVec3f axisY;
        TVec3f pairAxisY;

        MR::calcActorAxisY(&axisY, pActor);
        MR::calcActorAxisY(&pairAxisY, pPod);

        mWarp->_14 = pPod->mPosition + pairAxisY * 160.0f;
        mWarp->_20 = pActor->mPosition + axisY * 160.0f;

        MR::getRotatedAxisZ(&mWarp->_38, pPod->mRotation);
        MR::getRotatedAxisY(&mWarp->_2C, pPod->mRotation);

        mWarp->_45 = pPod->mArg1;
        mWarp->_44 = pPod->mArg2;

        stopWalk();

        changeStatus(mWarp);

        stopJump();

        return true;
    }

    return false;
}

bool Mario::doPointWarp(const TVec3f& rVec1, const TVec3f& rVec2, s32 myInt) {
    if (getPlayer()->getMovementStates().debugMode) {
        return false;
    }

    if (isStatusActive(MarioStatus_Warp)) {
        return false;
    }

    if (isStatusActive(MarioStatus_Warp)) {
        return false;
    }

    mVelocity.zero();

    stopWalk();

    mActor->setBlendMtxTimer(16);

    mWarp->_14 = rVec1;
    mWarp->_20 = mPosition;
    mWarp->_38 = rVec2;
    mWarp->_45 = 2;
    mWarp->_44 = false;

    if (myInt != 0) {
        mWarp->_48 = myInt;
    } else {
        mWarp->_48 = 90;
    }

    changeStatus(mWarp);

    stopJump();

    return true;
}

bool Mario::isVisibleRecoveryWarpBubble() const {
    if (isStatusActive(MarioStatus_Warp) && mWarp->_45 == 3) {
        return true;
    }

    return false;
}

MarioWarp::MarioWarp(MarioActor* pActor) : MarioState(pActor, MarioStatus_Warp), _52(), _58(-1) {
    _4C = nullptr;
    _48 = 0;

    _14.zero();
    _20.zero();
    _2C.zero();
    _38.zero();

    _44 = false;
    _45 = 0;
    _50 = false;

    _54 = 0;
    _56 = 0;

    _5C.zero();
    _68.zero();
    _74.zero();

    _88 = 0;
    _80 = 0.0f;
    _84 = 0.0f;
    _8C = 0;
}

void MarioWarp::calcAxis() {
    TVec3f difference(_14 - getTrans());
    f32 distance = difference.length();
    TVec3f axis;
    axis.cross(difference, getGravityVec());
    MR::normalizeOrZero(&axis);
    TVec3f half(difference * 0.5f);
    TVec3f center(getTrans() + half);
    TVec3f normal;
    normal.cross(axis, difference);
    MR::normalizeOrZero(&normal);

    f32 angle = MR::pi() * 0.25f;
    if (_45 == 3) {
        angle = MR::pi() * 0.4f;
    }
    f32 radius = (0.5f * distance) / MR::sin(angle);
    f32 height = MR::sqrt< f32 >(radius * radius - 0.5f * (0.5f * distance * distance));
    _5C = axis;
    _68 = center + normal * height;
    _74 = -normal * radius;
    _84 = angle;
    _80 = -angle;
    _88 = radius / 25.0f;
    if (_88 == 0) {
        _88 = 1;
    }
    if (_88 < 120) {
        _88 = 120;
    }
    if (_45 == 2) {
        _88 = _48;
    }
    _8C = _88;
}

void Mario::doCubeWarp() {
    if (getPlayer()->getMovementStates().debugMode) {
        return;
    }

    if (isStatusActive(MarioStatus_Warp)) {
        return;
    }

    if (isStatusActive(MarioStatus_Warp)) {
        return;
    }

    WarpCube* pWarpCube = static_cast< WarpCube* >(MR::getAreaObj("WarpCube", mActor->_2A0));
    WarpCubeMgr* pWarpCubeMgr = static_cast< WarpCubeMgr* >(MR::getAreaObjManager("WarpCube"));

    if (pWarpCube != nullptr) {
        if (_56C == pWarpCube) {
            return;
        }

        WarpCube* pPairCube = pWarpCubeMgr->getPairCube(pWarpCube);
        if (pPairCube == nullptr) {
            return;
        }

        pWarpCubeMgr->startEventCamera(pWarpCube);

        TVec3f cubePos;
        MR::calcCubePos(pPairCube, &cubePos);

        mVelocity.zero();
        stopWalk();
        mActor->_38C = 10;

        _56C = pPairCube;
        mWarp->_14 = cubePos;

        MR::calcCubePos(pWarpCube, &cubePos);
        mWarp->_20 = cubePos;
        mWarp->_38.zero();
        mWarp->_2C.zero();
        mWarp->_45 = 0;
        mWarp->_44 = 0;

        stopWalk();

        changeStatus(mWarp);

        stopJump();

        return;
    }

    if (_56C != nullptr) {
        pWarpCubeMgr->setInvalidateTimer(_56C, 300);
    }

    _56C = nullptr;
}

bool Mario::doPointWarpRecovery(const TVec3f& rVec1, const TVec3f& rVec2) {
    if (getPlayer()->getMovementStates().debugMode) {
        return false;
    }

    if (isStatusActive(MarioStatus_Warp)) {
        return false;
    }

    if (isStatusActive(MarioStatus_Warp)) {
        return false;
    }

    mVelocity.zero();

    stopWalk();

    mActor->setBlendMtxTimer(16);

    mWarp->_14 = rVec1;
    mWarp->_20 = mPosition;
    mWarp->_38 = rVec2;
    mWarp->_2C.zero();
    mWarp->_45 = 3;
    mWarp->_44 = false;

    changeStatus(mWarp);

    stopJump();

    MR::startGlobalEventCameraNoTarget("引き戻し", -1);

    return true;
}
void MarioWarp::updateJump() {
    if (_88 != 0) {
        f32 ratio = (1.0f + MR::sin(((static_cast< f32 >(_8C - _88) - 0.5f * static_cast< f32 >(_8C)) / static_cast< f32 >(_8C)) * MR::pi())) * 0.5f;
        if (_45 == 2) {
            ratio = 1.0f - static_cast< f32 >(_88 - 1) / static_cast< f32 >(_8C);
        }

        Mtx mtx;
        PSMTXRotAxisRad(mtx, &_5C, _80 * (1.0f - ratio) + _84 * ratio);
        TVec3f rotated;
        PSMTXMultVecSR(mtx, &_74, &rotated);
        TVec3f position(_68 + rotated);
        f32 speed = 0.5f;
        if (_45 == 2) {
            speed = 1.0f;
        }
        if (_45 == 3) {
            f32 remain = static_cast< f32 >(_88) / static_cast< f32 >(_8C);
            speed = (1.0f - remain) + speed * remain;
            position = position * remain + _14 * (1.0f - remain);
        }
        addTrans((position - getTrans()) * speed, "Module");
        if (_45 == 2) {
            getPlayer()->setFrontVecKeepUp(_38, 0.2f);
        }
    }
}

bool MarioWarp::start() {
    _52 = 0;
    _58 = 0;
    switch (_45) {
    case 0:
    case 2:
        _54 = 1;
        break;
    case 1:
        _54 = 60;
        break;
    case 3:
        _54 = 40;
        break;
    }
    _56 = _54;
    if (_45 == 3) {
        playEffect("引き戻し泡");
    }
    if (_45 == 1) {
        playEffect("ワープポッドブラー");
        playSound("ワープポッド入り");
    }
    if (_45 == 2) {
        playSound("声小ジャンプ");
    } else {
        playSound("声慌て");
    }
    switch (_45) {
    case 0:
        break;
    case 3:
        changeAnimationNonStop("引き戻し");
        break;
    case 1:
        changeAnimationNonStop("ポッドワープ開始");
        break;
    case 2:
        changeAnimationNonStop("しゃがみジャンプ");
        break;
    }
    mActor->_F44 = false;
    const TVec3f& actorPosition = mActor->mPosition;
    addTrans(actorPosition - getTrans(), "Module");
    update();
    if (_45 == 3 && getPlayerMode() != PlayerMode_Invincible) {
        mActor->_A6E = 2;
    }
    if (!_44) {
        getPlayer()->mMovementStates._2B = true;
    }
    return true;
}

bool MarioWarp::update() {
    if (!_44) {
        getPlayer()->mMovementStates._2B = true;
        getPlayer()->mMovementStates._22 = true;
    }
    clearVelocity();
    if (_54 != 0) {
        if (_45 == 1 || _45 == 3) {
            playSound("引き戻し基本");
        }
        if (_45 == 1) {
            playSound("ワープポッド移動");
        }
        --_54;
        TVec3f difference(_20 - getTrans());
        addTrans(difference * (1.0f - static_cast< f32 >(_54) / static_cast< f32 >(_56)), "Module");
        if (_54 == 0) {
            calcAxis();
            if (_45 == 1 && getPlayerMode() != PlayerMode_Teresa) {
                MarioActor* actor = mActor;
                actor->_481 = 1;
                actor->updateHand();
                actor->updateFace();
            }
        }
        if (_4C != nullptr) {
            MR::getWarpPodManager()->startEventCamera(_4C);
        }
        return true;
    }

    updateJump();
    if (_45 == 0) {
        TVec3f difference(getTrans() - _20);
        TVec3f direction(_14 - _20);
        MR::normalize(&direction);
        f32 distance = MR::vecKillElement(difference, direction, &difference);
        addTrans(_20 + direction * distance - getTrans(), "Module");
    }
    switch (_45) {
    case 0:
        playSound("惑星貫通中");
        break;
    case 1:
    case 3: {
        s32 volume;
        if (_88 == 0) {
            volume = 0;
        } else {
            f32 half = static_cast< f32 >(_8C) * 0.5f;
            f32 time = static_cast< f32 >(_8C - _88);
            if (time < half) {
                volume = 100.0f * (time / half);
            } else {
                volume = 100.0f * (1.0f - (time - half) / half);
            }
        }
        if (volume < 0) {
            volume = 0;
        }
        if (volume > 100) {
            volume = 100;
        }
        playSound("引き戻し基本");
        playSound("引き戻し浮遊", volume);
        if (_45 == 1) {
            playSound("ワープポッド移動");
        }
        break;
    }
    }
    if (_58 == 1 && !isAnimationRun("ポッドワープ終了")) {
        return false;
    }
    if (_88 == 0) {
        if (_45 == 1 && getPlayerMode() != PlayerMode_Teresa) {
            if (_58 != 1) {
                playSound("ワープポッド出");
            }
            _58 = 1;
            mActor->_481 = 0;
            changeAnimation("ポッドワープ終了", static_cast< const char* >(nullptr));
            WarpPodMgr* manager = MR::getWarpPodManager();
            if (manager != nullptr) {
                manager->endEventCamera();
            }
        } else {
            return false;
        }
    } else {
        --_88;
    }
    return true;
}

bool MarioWarp::close() {
    getPlayer()->mMovementStates._1 = true;
    getPlayer()->mMovementStates.jumping = false;
    getPlayer()->_420 = 16;
    stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
    changeAnimation(static_cast< const char* >(nullptr), "基本");
    switch (_45) {
    case 0:
        playSound("惑星貫通終了");
        break;
    case 3:
        playSound("引き戻し泡破裂");
        stopEffectForce("引き戻し泡");
        playEffect("引き戻し泡破裂");
        break;
    case 1:
        stopEffect("ワープポッドブラー");
        break;
    case 2:
        changeAnimation("しゃがみジャンプ着地", static_cast< const char* >(nullptr));
        break;
    }
    mActor->_F44 = true;
    WarpCubeMgr* cubeMgr = static_cast< WarpCubeMgr* >(MR::getAreaObjManager("WarpCube"));
    if (cubeMgr != nullptr) {
        cubeMgr->endEventCamera();
    }
    if (_45 != 1) {
        WarpPodMgr* podMgr = MR::getWarpPodManager();
        if (podMgr != nullptr) {
            podMgr->endEventCamera();
        }
    } else {
        if (_88 != 0 || getPlayerMode() == PlayerMode_Teresa) {
            mActor->_481 = 0;
            WarpPodMgr* podMgr = MR::getWarpPodManager();
            if (podMgr != nullptr) {
                podMgr->endEventCamera();
            }
        }
        WarpPodMgr* podMgr = MR::getWarpPodManager();
        if (podMgr != nullptr) {
            podMgr->notifyWarpEnd(static_cast< WarpPod* >(_4C));
        }
    }
    switch (_45) {
    case 0: {
        playSound("声中ジャンプ");
        const MarioConstTable* table = mActor->getConst().getTable();
        getPlayer()->tryForcePowerJump(_2C * table->mWarpPodJumpY + _38 * table->mWarpPodJumpX, false);
        break;
    }
    case 3:
        MR::endGlobalEventCamera("引き戻し", -1, true);
        if (getPlayerMode() != PlayerMode_Invincible) {
            mActor->_A6E = 0;
        }
        getPlayer()->mMovementStates._1 = false;
        getPlayer()->mMovementStates.jumping = true;
        getPlayer()->mMovementStates._2B = true;
        getPlayer()->_402 = 0;
        getPlayer()->mJumpVec = TVec3f(0.0f, 0.0f, 0.0f);
        changeAnimation("落下", static_cast< const char* >(nullptr));
        {
            Mario* player = getPlayer();
            player->_42A = 0;
            player->_430 = 0;
        }
        break;
    case 1:
        if (getPlayerMode() != PlayerMode_Invincible) {
            mActor->_A6E = 0;
        }
        if (getPlayerMode() != PlayerMode_Teresa) {
            playSound("声中ジャンプ");
            TVec3f jump(_2C * 12.0f + _38 * 5.0f);
            getPlayer()->tryForcePowerJump(jump, false);
        }
        break;
    case 2:
        getPlayer()->setFrontVecKeepUp(_38);
        break;
    }
    if (getPlayerMode() != PlayerMode_Teresa) {
        getPlayer()->mMovementStates._22 = true;
    } else {
        getPlayer()->mMovementStates._22 = false;
    }
    _4C = nullptr;
    return true;
}
