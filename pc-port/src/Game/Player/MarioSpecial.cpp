#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include <cstring>

void Mario::checkOnimasu(const HitSensor* pSensor) {
    if (strstr(pSensor->mHost->mName, "オニマス") == nullptr) {
        return;
    }

    if (_5FC == nullptr) {
        _5FC = const_cast< HitSensor* >(pSensor);
        _60C = true;
        return;
    }

    if (_5FC != pSensor) {
        f32 oldDistance = (_5FC->mPosition - mPosition).length();
        f32 newDistance = (pSensor->mPosition - mPosition).length();
        if (newDistance < oldDistance) {
            _5FC = const_cast< HitSensor* >(pSensor);
            _60C = true;
        }
    }
}

bool Mario::isDossun(const Triangle* pTriangle) const {
    if (pTriangle->isValid() == 0) {
        return false;
    }

    return strstr(pTriangle->mSensor->mHost->mName, "ドッスン") != nullptr;
}

bool Mario::isStageCameraRotate2D() const {
    bool result = false;
    if (MR::isEqualStageName("HellProminenceGalaxy") && MR::getCurrentScenarioNo() == 3) {
        result = true;
    }

    return result;
}

bool Mario::isNoWalkFallOnDossun() const {
    if (MR::isEqualStageName("CannonFleetGalaxy")) {
        if (isDossun(mGroundPolygon) && isDossun(_45C)) {
            return true;
        }
    }

    return false;
}

bool Mario::isNotReflectGlassGround() const {
    if (!MR::isEqualStageName("AstroGalaxy")) {
        return false;
    }

    TVec3f target(1627.0f, 783.0f, -2152.0f);
    TVec3f direction = mPosition - target;
    f32 angle = MR::diffAngleAbs(getCamDirZ(), direction);
    if (angle >= HALF_PI) {
        angle = PI - angle;
    }

    f32 sine = MR::sin(angle);
    f32 sineSquared = MR::sin(angle) * sine;
    f32 minDistance = 400.0f + 300.0f * sineSquared;
    MR::vecKillElement(direction, getAirGravityVec(), &direction);
    f32 distance = direction.length();
    if (distance >= minDistance && distance <= 700.0f) {
        return true;
    }

    return false;
}

bool Mario::isUseAnotherMovingPolygon() const {
    if (MR::isEqualStageName("BattleShipGalaxy")) {
        return true;
    }

    if (MR::isEqualStageName("SandClockGalaxy")) {
        return true;
    }

    return MR::isEqualStageName("TriLegLv1Galaxy");
}

bool Mario::isUseFooSpecialGravity(const TVec3f& rPosition, TVec3f* pGravity) const {
    if (!isStatusActive(0x18)) {
        return false;
    }

    if (MR::isEqualStageName("HeavensDoorGalaxy") && MR::getCurrentScenarioNo() == 2) {
        TVec3f center(14760.0f, -10676.2f, 6770.0f);
        TVec3f gravity = center - rPosition;
        pGravity->x = gravity.x;
        pGravity->y = gravity.y;
        pGravity->z = gravity.z;
        MR::normalizeOrZero(pGravity);
        return true;
    }

    return false;
}

void Mario::updateOnimasu() {
    if (_5FC == nullptr) {
        return;
    }

    if ((_5FC->mPosition - mPosition).length() >= 1000.0f) {
        _5FC = nullptr;
        return;
    }

    TVec3f currentLocal;
    TVec3f previousLocal;
    HitSensor* sensor = _5FC;
    TPos3f* inverseMtx = &sensor->mHost->mCollisionParts->mInvBaseMatrix;
    previousLocal = _600;
    PSMTXMultVec(inverseMtx->toMtxPtr(), &mActor->mPosition, &currentLocal);

    if (_60C) {
        _600 = currentLocal;
        _60C = false;
        return;
    }

    if (!_10._1D) {
        if (previousLocal.y < -400.0f && currentLocal.y > -400.0f && MR::isInRange(currentLocal.x, -400.0f, 400.0f) &&
            MR::isInRange(currentLocal.z, -400.0f, 400.0f)) {
            currentLocal = _600;
            TVec3f worldPosition;
            PSMTXMultVec(sensor->mHost->mCollisionParts->mBaseMatrix.toMtxPtr(), &currentLocal, &worldPosition);
            setTrans(worldPosition, nullptr);
            mVelocity.zero();
        } else if (previousLocal.y > 400.0f && currentLocal.y <= 400.0f && MR::isInRange(currentLocal.x, -400.0f, 400.0f) &&
                   MR::isInRange(currentLocal.z, -400.0f, 400.0f)) {
            _10._1D = true;
            if (previousLocal.x < -320.0f) {
                previousLocal.x = -320.0f;
            } else if (previousLocal.x > 320.0f) {
                previousLocal.x = 320.0f;
            }

            if (previousLocal.z < -320.0f) {
                previousLocal.z = -320.0f;
            } else if (previousLocal.z > 320.0f) {
                previousLocal.z = 320.0f;
            }

            currentLocal = previousLocal;
        } else if (MR::isInRange(currentLocal.x, -400.0f, 400.0f) && MR::isInRange(currentLocal.y, -400.0f, 400.0f) &&
                   MR::isInRange(currentLocal.z, -400.0f, 400.0f)) {
            mVelocity.zero();
            currentLocal = _600;

            TVec3f worldPosition;
            PSMTXMultVec(sensor->mHost->mCollisionParts->mBaseMatrix.toMtxPtr(), &currentLocal, &worldPosition);
            TVec3f difference = worldPosition - mPosition;
            f32 limit = 10.0f + mVerticalSpeed;
            if (-difference.dot(getShadowNorm()) > limit) {
                MR::vecKillElement(difference, getShadowNorm(), &difference);
                difference += -getShadowNorm() * mVerticalSpeed;
                worldPosition = mPosition + difference;
            }

            setTrans(worldPosition, nullptr);
        }

        bool adjustX = false;
        bool adjustY = false;
        bool adjustZ = false;
        TVec3f axisX;
        TVec3f axisY;
        TVec3f axisZ;
        sensor->mHost->mCollisionParts->mBaseMatrix.getXDir(axisX);
        sensor->mHost->mCollisionParts->mBaseMatrix.getYDir(axisY);
        sensor->mHost->mCollisionParts->mBaseMatrix.getZDir(axisZ);

        f32 dotX = axisX.dot(*getGravityVec());
        f32 absDotX = __fabsf(dotX);
        f32 dotY = axisY.dot(*getGravityVec());
        f32 absDotY = __fabsf(dotY);
        f32 dotZ = axisZ.dot(*getGravityVec());
        f32 min = -500.0f;
        f32 max = -300.0f;
        f32 absDotZ = __fabsf(dotZ);
        if (absDotX > absDotY && absDotX > absDotZ) {
            if (dotX > 0.0f) {
                min = 300.0f;
                max = 500.0f;
            }

            if (MR::isInRange(currentLocal.x, min, max)) {
                bool inY = MR::isInRange(currentLocal.y, -320.0f, 320.0f);
                bool inZ = MR::isInRange(currentLocal.z, -320.0f, 320.0f);
                if (inY && inZ) {
                    mActor->_3B4 = axisX;
                    mActor->setPress(0, 0);
                } else {
                    if (inZ) {
                        adjustY = true;
                    }
                    if (inY) {
                        adjustZ = true;
                    }
                }
            }
        } else if (absDotY > absDotX && absDotY > absDotZ) {
            if (dotY > 0.0f) {
                min = 300.0f;
                max = 500.0f;
            }

            if (MR::isInRange(currentLocal.y, min, max)) {
                bool inX = MR::isInRange(currentLocal.x, -320.0f, 320.0f);
                bool inZ = MR::isInRange(currentLocal.z, -320.0f, 320.0f);
                if (inX && inZ) {
                    mActor->_3B4 = axisY;
                    if (dotY < 0.0f) {
                        mActor->setPress(0, 0);
                    }
                } else {
                    if (inZ) {
                        adjustX = true;
                    }
                    if (inX) {
                        adjustZ = true;
                    }
                }
            }
        } else {
            if (dotZ > 0.0f) {
                min = 300.0f;
                max = 500.0f;
            }

            if (MR::isInRange(currentLocal.z, min, max)) {
                bool inX = MR::isInRange(currentLocal.x, -320.0f, 320.0f);
                bool inY = MR::isInRange(currentLocal.y, -320.0f, 320.0f);
                if (inX && inY) {
                    mActor->_3B4 = axisZ;
                    mActor->setPress(0, 0);
                } else {
                    if (inY) {
                        adjustX = true;
                    }
                    if (inX) {
                        adjustY = true;
                    }
                }
            }
        }

        if (adjustX) {
            if (currentLocal.x > -400.0f && currentLocal.x < -320.0f) {
                currentLocal.x = -400.0f;
            } else if (currentLocal.x < 400.0f && currentLocal.x > 320.0f) {
                currentLocal.x = 400.0f;
            }
        }

        if (adjustY) {
            if (currentLocal.y > -400.0f && currentLocal.y < -320.0f) {
                currentLocal.y = -400.0f;
            } else if (currentLocal.y < 400.0f && currentLocal.y > 320.0f) {
                currentLocal.y = 400.0f;
            }
        }

        if (adjustZ) {
            if (currentLocal.z > -400.0f && currentLocal.z < -320.0f) {
                currentLocal.z = -400.0f;
            } else if (currentLocal.z < 400.0f && currentLocal.z > 320.0f) {
                currentLocal.z = 400.0f;
            }
        }

        if (adjustX | adjustY | adjustZ) {
            TVec3f worldPosition;
            PSMTXMultVec(sensor->mHost->mCollisionParts->mBaseMatrix.toMtxPtr(), &currentLocal, &worldPosition);
            if (mMovementStates._8 || mMovementStates._19 || mMovementStates._1A) {
                TVec3f difference = worldPosition - mPosition;
                if (getWallNorm().dot(difference) < 0.0f) {
                    addVelocity(-difference);
                } else {
                    setTrans(worldPosition, nullptr);
                }
            }
        }
    } else if (currentLocal.y > 400.0f) {
        _10._1D = false;
    } else {
        bool adjusted = false;
        if (previousLocal.y >= -320.0f && currentLocal.y < -320.0f) {
            currentLocal.y = previousLocal.y;
            adjusted = true;
        }
        if (previousLocal.x >= -320.0f && currentLocal.x < -320.0f) {
            currentLocal.x = previousLocal.x;
            adjusted = true;
        }
        if (previousLocal.x <= 320.0f && currentLocal.x > 320.0f) {
            currentLocal.x = previousLocal.x;
            adjusted = true;
        }
        if (previousLocal.z >= -320.0f && currentLocal.z < -320.0f) {
            currentLocal.z = previousLocal.z;
            adjusted = true;
        }
        if (previousLocal.z <= 320.0f && currentLocal.z > 320.0f) {
            currentLocal.z = previousLocal.z;
            adjusted = true;
        }

        if (adjusted) {
            TVec3f worldPosition;
            PSMTXMultVec(sensor->mHost->mCollisionParts->mBaseMatrix.toMtxPtr(), &_600, &worldPosition);
            setTrans(worldPosition, nullptr);
            currentLocal = _600;
        }
    }

    _600 = currentLocal;
}

bool Mario::isHeadPushEnableArea() const {
    if (!mMovementStates._37) {
        return false;
    }

    if (MR::isEqualStageName("SandClockGalaxy") && MR::isInRange(mPosition.x, -7020.0f, -6920.0f)) {
        return true;
    }

    return false;
}

bool Mario::isOnimasuBinderPressSkip() const {
    if (isStatusActive(0x15) && mFrontWallTriangle->mSensor != nullptr && strstr(mFrontWallTriangle->mSensor->mHost->mName, "オニマス") != nullptr) {
        return true;
    }

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
