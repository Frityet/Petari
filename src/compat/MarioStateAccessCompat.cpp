#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioState.hpp"

// Original accessor bodies from MarioActorGravity.cpp and MarioState.cpp.
// The collision and wall queries live in their complete original source units.
const TVec3f& MarioActor::getGravityVec() const {
    return *mMario->getGravityVec();
}

void MarioActor::getGravityVector(TVec3f* pVec) const {
    pVec->set(mMario->getAirGravityVec());
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
