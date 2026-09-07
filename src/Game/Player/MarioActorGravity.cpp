#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioShadow.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"

const TVec3f& MarioActor::getGravityVec() const {
    return *mMario->getGravityVec();
}

const TVec3f& MarioActor::getGravityVector() const {
    return *mMario->getGravityVec();
}

const TVec3f& MarioActor::getAirGravityVec() const {
    return mMario->getAirGravityVec();
}

void MarioActor::getGravityVector(TVec3f* pVec) const {
    pVec->set(mMario->getAirGravityVec());
}

f32 MarioActor::getGravityRatio() const {
    return mGravityRatio;
}

GravityInfo* MarioActor::getGravityInfo() const {
    return mGravityInfo;
}

u8 MarioActor::getGravityLevel() const {
    return MR::isLightGravity(*mGravityInfo);
}

bool MarioActor::checkBeeWallStick(TVec3f& rVec) {
    if (getMovementStates()._8 && mMario->checkWallCode("Fur", false) && mBeeWallWalk == 0 && !_9F2) {
        TVec3f vec20 = mMario->mHeadVec;

        mBeeWallWalk = 5;

        rVec = -mMario->getWallNorm();

        mPosition = mMario->getWallPos();

        mMario->stopJump();
        mMario->stopAnimation(nullptr);
        mMario->stopWalk();

        _240 = rVec;
        mMario->setGravityVec(rVec);
        mMario->setHeadVec(-rVec);
        mMario->setFrontVecKeepUp(vec20, 1UL);
        setBlendMtxTimer(2);

        _38C = 5;
        mMario->mMovementStates._38 = false;
        _214->_305 = true;

        return true;
    }

    return false;
}

bool MarioActor::checkBeeFloorStick(TVec3f& rVec) {
    if (mMario->getMovementStates()._1 && strcmp("Fur", MR::getWallCodeString(mMario->getGroundPolygon())) == 0 && mBeeWallWalk == 0) {
        mBeeWallWalk = 5;
        rVec = -mMario->_368;

        return true;
    }

    return false;
}

void MarioActor::syncJumpBeeStickMode() {
    if (mBeeWallWalk != 0 && selectQuickResetBeeWallGravity(mMario->_45C->mSensor->mHost->mName)) {
        mBeeWallWalk = 0;
        _9F2 = 30;
    }

    if (!(MR::diffAngleAbs(_360, getGravityVec()) < MR::pi() / 36.0f)) {
        return;
    }

    _33C = mPosition - mMario->mFrontVec * 2.0f;
    _354 = mPosition - _33C;
    _360 = _2A0 - _33C;
    MR::normalize(&_360);
}

void MarioActor::updateBeeModeGravity(TVec3f& rVec) {
    u8 alpha = mBeeWallWalk;

    if (_9F2 != 0) {
        _9F2--;
    }

    if (_9F2 != 0 || (!checkBeeCeilStick(rVec) && !checkBeeWallStick(rVec) && !checkBeeFloorStick(rVec))) {
        updateBeeStickMode(rVec);
    }

    if (mBeeWallWalk == 5) {
        MR::vecBlendSphere(_240, rVec, &rVec, 0.1f);
        return;
    }

    if (mBeeWallWalk != 0) {
        MR::vecBlendSphere(_240, rVec, &rVec, 0.3f);
        return;
    }

    if (alpha != 0) {
        _9F2 = 60;
    }
}

bool MarioActor::isInZeroGravitySpot() const {
    return MR::isNearZero(_24C);
}

// void MarioActor::updateGravityVec(bool, bool) {}

bool MarioActor::checkBeeCeilStick(TVec3f& rVec) {
    if ((mMario->isCeiling() || getDrawStates()._15) && mBeeWallWalk == 0 && _9F2 == 0) {
        bool out = false;
        if (getDrawStates()._15) {
            out = true;
        } else {
            const char* wallCodeString = MR::getWallCodeString(mMario->_4C8);
            if (wallCodeString != nullptr && strcmp(wallCodeString, "Fur") == 0) {
                out = true;
            }
        }

        if (out) {
            Triangle triangle = Triangle();
            TVec3f vec;
            if (MR::getFirstPolyOnLineToMap(&vec, &triangle, mPosition, (-_240).multiplyOperatorInline(200.0f))) {
                entryWallWalkMode(vec, *(triangle.getNormal(0)));
            }
        }
    }

    return false;
}


void MarioActor::updateBeeStickMode(TVec3f& rVec) {
    if (mBeeWallWalk == 0) {
        return;
    }

    bool cancel = false;
    bool fur = mMario->mDrawStates._19;
    if (!fur) {
        const char* wallCode = MR::getWallCodeString(mMario->_45C);
        if (wallCode != nullptr && strcmp(wallCode, "Fur") == 0) {
            fur = true;
        }
    }

    if (fur && _9F2 == 0) {
        f32 radius = mConst->getTable()->mBeeWallWalkCancelRadius;
        if (MR::getAreaObj("BeeWallShortDistArea", mPosition) != nullptr) {
            radius = mConst->getTable()->mBeeWallWalkCancelRadiusShort;
        }
        if (mMario->mVerticalSpeed > radius) {
            cancel = true;
        } else if (isJumping() && mMario->checkWallCode("Normal", false)) {
            cancel = true;
        } else {
            mBeeWallWalk = 5;
        }
        if (isJumping() && isRequestRush()) {
            mBeeWallWalk = 0;
        }
    } else if (isJumping()) {
        if (mMario->mVerticalSpeed < 100.0f) {
            mBeeWallWalk--;
        }
        if (mMario->checkWallCode("Normal", false)) {
            cancel = true;
        }
        f32 radius = mConst->getTable()->mBeeWallWalkCancelRadius;
        if (MR::getAreaObj("BeeWallShortDistArea", mPosition) != nullptr) {
            radius = mConst->getTable()->mBeeWallWalkCancelRadiusShort;
        }
        if (mMario->mVerticalSpeed > radius) {
            cancel = true;
        }
    } else {
        if (mMario->mMovementStates._B) {
            mMario->mMovementStates._B = false;
        }
        cancel = true;
    }

    if (cancel && mBeeWallWalk != 0) {
        mBeeWallWalk--;
    }
    if (mBeeWallWalk == 0) {
        mMario->stopWalk();
        mMario->tryJump();
        mMario->_408 = mConst->getTable()->mBeeGravityPowerTime;
        mMario->_3BC = mConst->getTable()->mBeeAirWalkInhibitTime - 5;
        TVec3f push(*mMario->_45C->getNormal(0));
        push *= 100.0f;
        mMario->push(push);
        mMario->cutVecElementFromJumpVec(_24C);
    } else {
        rVec = -*mMario->_45C->getNormal(0);
    }
}
