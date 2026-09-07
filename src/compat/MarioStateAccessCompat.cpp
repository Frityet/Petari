#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Util/MathUtil.hpp"

// Original accessor bodies from MarioActorGravity.cpp, MarioWall.cpp,
// MarioState.cpp, and MarioCollision.cpp. Their full source units are not yet
// in the native player build; these definitions preserve their source bodies.
const TVec3f& MarioActor::getGravityVec() const {
    return *mMario->getGravityVec();
}

void MarioActor::getGravityVector(TVec3f* pVec) const {
    pVec->set(mMario->getAirGravityVec());
}

bool Mario::isWalling() const {
    return getCurrentStatus() == MarioStatus_Wall;
}

u32 Mario::getCurrentStatus() const {
    MarioState* pState = _97C;

    if (pState == nullptr) {
        return MarioStatus_None;
    }

    return pState->mStatusId;
}

bool Mario::isStatusActive(u32 statusId) const {
    MarioState* pState = _97C;

    if (pState == nullptr) {
        return false;
    }

    if (statusId == pState->mStatusId) {
        return true;
    }

    while (pState != nullptr) {
        if (statusId == pState->mStatusId) {
            return true;
        }

        pState = pState->_8;
    }

    return false;
}

bool Mario::checkCurrentFloorCodeSevere(u32 code) const {
    if (_960 != code) {
        return false;
    }

    TVec3f shadowToGround = mShadowPos - mGroundPos;
    TVec3f horizontal;
    f32 vertical = MR::vecKillElement(shadowToGround, getAirGravityVec(), &horizontal);

    if (vertical > 10.0f) {
        return _95C->getCode(mGroundPolygon) == code;
    }

    u32 shadowCode = _95C->getCode(_45C);
    if (shadowCode != code) {
        return false;
    }

    return _95C->getCode(mGroundPolygon) == shadowCode;
}
