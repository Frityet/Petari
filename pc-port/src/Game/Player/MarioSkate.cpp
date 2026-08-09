#include "Game/Player/MarioSkate.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

bool Mario::isSkatableFloor() const {
    if (_960 == 5) {
        return true;
    }

    return _960 == 33;
}

bool Mario::doSkate() {
    changeStatus(mSkate);
    return true;
}

MarioSkate::MarioSkate(MarioActor* pActor) : MarioState(pActor, MarioStatus_Skate), _14(0) {
    _20 = 0.0f;
    _18 = false;
    _19 = false;
    _1A = false;
    _1B = 0;
    _1C = false;
    _1D = 0;
    _24 = 0.0f;
}

bool MarioSkate::postureCtrl(MtxPtr mtx) {
    getPlayer()->postureCtrl(mtx);
    f32 angle = _20;
    angle *= PI;
    PSMTXConcat(mtx, MR::tmpMtxRotYRad(angle), mtx);
    PSMTXConcat(mtx, MR::tmpMtxRotZRad(_24), mtx);

    Mtx direction;
    getPlayer()->createDirectionMtx(direction);
    PSMTXConcat(direction, mtx, mtx);
    return true;
}

void MarioSkate::exitJump() {
    _18 = true;
    getPlayer()->tryJump();
    playSound("スケートジャンプ", -1);
}

bool MarioSkate::start() {
    _14 = 0;
    _1B = 0;
    _1C = false;
    _18 = false;
    _1D = 1;
    _19 = false;
    _24 = 0.0f;
    _1A = false;

    if (isAnimationRun("スケートジャンプ") || isAnimationRun("スケートジャンプ2") || isAnimationRun("スケートジャンプ3")) {
        _1D = 1;
        changeAnimation(static_cast< const char* >(nullptr), "基本");
        changeAnimationNonStop("スケート着地");
        playEffect("スケート右");
        playEffect("スケート左");
        playSound("スケート着地", -1);
        _19 = false;
        _20 = 0.0f;
    } else {
        _20 = 0.0f;
        if (getPlayer()->_71C < 3) {
            _1A = true;
            changeAnimationNonStop("アイスひねり静止");
        } else {
            changeAnimationNonStop("アイスひねり");
            playEffect("スケート左");
        }

        playSound("スケートスピン", -1);
        playSound("スピンジャンプ", -1);
        playSound("声スピン", -1);
    }

    return true;
}

bool MarioSkate::update() {
    if (!getPlayer()->mMovementStates._1 || getPlayer()->mMovementStates.jumping) {
        return false;
    }

    if (checkTrgA() || getPlayer()->mMovementStates._38) {
        _18 = true;
        getPlayer()->tryJump();
        playSound("スケートジャンプ", -1);
        return false;
    }

    if (!getPlayer()->isSkatableFloor()) {
        return false;
    }

    if (!_1A) {
        getPlayer()->mainMove();
    } else {
        return !isAnimationTerminate(static_cast< const char* >(nullptr));
    }

    if (getPlayer()->mMovementStates._10) {
        getPlayer()->mMovementStates._10 = false;
        getPlayer()->_3D2 = 0;
    }

    if (checkTrgZ()) {
        _19 = !_19;
        _14 = 20;
        _1C = true;
        _1B = 1 - _1B;
        playSound("声壁押し", -1);
    }

    getPlayer()->updateWalkSpeed();

    if (isAnimationRun("スケート着地")) {
        if (isAnimationTerminate(static_cast< const char* >(nullptr))) {
            _1C = true;
        }
    } else if (_1D == 1) {
        const f32 turnFrame = 20.0f + 50.0f * (1.0f - getStickP());
        if (getAnimator()->getFrame() > turnFrame && getStickP() != 0.0f) {
            _1C = true;
        }
    }

    if (mActor->isRequestSpin()) {
        if (_1D != 0) {
            if (!isAnimationRun("アイスひねり移動") || isAnimationTerminate(static_cast< const char* >(nullptr))) {
                stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
                changeAnimationNonStop("アイスひねり移動");
                playSound("スケートスピン", -1);
                playSound("スピンジャンプ", -1);
                playSound("声パンチ", -1);

                if (getPlayer()->_278 < 1.25f) {
                    const f32 acceleratedSpeed = 1.5f * getPlayer()->_278;
                    getPlayer()->_278 = acceleratedSpeed;
                }
            }
        }

        if (getAnimator()->getFrame() > 20.0f) {
            _1C = true;
        }
    }

    f32 speed = getPlayer()->_278;
    if (speed < 1.2f * getStickP()) {
        getPlayer()->_278 = 1.2f * getStickP();
    }

    if (speed > 0.0f && !isAnimationRun("基本")) {
        playSound("スケート滑り", -1);
    }

    if (_19) {
        _20 += 0.04f;
        _20 = MR::clamp(_20, -1.0f, 1.0f);
    } else {
        if (_20 == 1.0f) {
            _20 = -1.0f;
        }

        bool wasNonPositive = false;
        if (_20 <= 0.0f) {
            wasNonPositive = true;
        }

        _20 += 0.04f;
        if (wasNonPositive) {
            _20 = -MR::clamp(-_20, 0.0f, 1.0f);
        } else {
            _20 = MR::clamp(_20, -1.0f, 1.0f);
        }
    }

    f32 turnAngle = MR::diffAngleAbsHorizontal(getWorldPadDir(), getFrontVec(), getGravityVec());
    TVec3f cross;
    cross.cross(getFrontVec(), getWorldPadDir());
    if (cross.dot(getGravityVec()) < 0.0f) {
        turnAngle = -turnAngle;
    }

    if (mActor->_3E5) {
        _14 = 15;
    } else {
        ++_14;
    }

    u32 turnDelay = 30;
    if (_19) {
        turnDelay = 60;
    }

    if (_14 >= turnDelay) {
        if (_1C) {
            if (turnAngle >= 0.1f && _1B == 1) {
            } else if (turnAngle <= -0.1f && _1B == 0) {
            } else {
                _1C = false;
                _14 = 0;
                _1B = 1 - _1B;

                if (_1D < 1) {
                    ++_1D;
                } else {
                    switch (_1B) {
                    case 0:
                        if (_19) {
                            changeAnimationNonStop("氷上後行右");
                        } else {
                            changeAnimationNonStop("氷上力行左");
                        }
                        playEffect("スケート左");
                        stopEffect("スケート右");
                        playSound("スケート足", -1);
                        break;
                    case 1:
                        if (_19) {
                            changeAnimationNonStop("氷上後行左");
                        } else {
                            changeAnimationNonStop("氷上力行右");
                        }
                        playEffect("スケート右");
                        stopEffect("スケート左");
                        playSound("スケート足", -1);
                        break;
                    }
                }
            }
        } else if (_14 > 90) {
            return false;
        }
    }

    f32 animationSpeed = 1.0f;
    const f32 stickSpeedRatio = 1.0f - 0.5f * (1.0f - getStickP());
    if (!isAnimationRun("スケート着地")) {
        getAnimator()->setSpeed(animationSpeed * stickSpeedRatio);
    }

    f32 leanAngle;
    if (_19) {
        leanAngle = MR::clamp(turnAngle, -0.3926991f, 0.3926991f);
    } else {
        leanAngle = MR::clamp(turnAngle, -0.5235988f, 0.5235988f);
    }

    const f32 leanTarget = leanAngle * (1.0f - 0.8f * (1.0f - getStickP()));
    if (__fabsf(leanTarget) > __fabsf(_24)) {
        _24 = 0.9f * _24 + 0.1f * leanTarget;
    } else {
        _24 = 0.95f * _24 + 0.05f * leanTarget;
    }

    return true;
}

bool MarioSkate::close() {
    if (_18) {
        if (!_1A) {
            switch (getPlayer()->_430) {
            case 1:
                changeAnimationNonStop("スケートアクセルジャンプ");
                break;
            case 2:
                changeAnimationNonStop("スケートジャンプ2");
                break;
            default:
                changeAnimationNonStop("スケートジャンプ");
                break;
            }

            Mario* player = getPlayer();
            const TVec3f& gravity = getGravityVec();
            TVec3f horizontal;
            const f32 gravitySpeed = MR::vecKillElement(player->mJumpVec, gravity, &horizontal);
            horizontal *= 1.5f;
            TVec3f vertical(getGravityVec());
            vertical *= gravitySpeed;
            horizontal += vertical;
            getPlayer()->mJumpVec = horizontal;
        }
    } else if (getPlayer()->mMovementStates._1) {
        stopAnimation(static_cast< const char* >(nullptr), "基本");
    } else {
        stopAnimation(static_cast< const char* >(nullptr), "落下");
    }

    stopEffect("スケート左");
    stopEffect("スケート右");
    return true;
}

bool MarioSkate::notice() {
    return false;
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
