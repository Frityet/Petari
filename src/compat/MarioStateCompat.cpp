#include "Game/Player/MarioState.hpp"
#include "Game/Player/Mario.hpp"

// Original MarioState lifecycle and base virtual bodies. The two read-only
// status queries stay in MarioStateAccessCompat.cpp so ordinary Game archive
// users do not pull the optional native MarioModule/player implementation.
// Source correspondence and retail vtable proof:
// notes/original-mario-state-lifecycle-20260903/.

MarioState::MarioState(MarioActor* pActor, u32 statusId) : MarioModule(pActor), _8(), mStatusId(statusId), _10() {
}

bool MarioState::proc(u32 msg) {
    switch (msg) {
    case MarioStateMsg_Start:
        return start();
    case MarioStateMsg_Close:
        if (_10) {
            break;
        }

        _10 = true;
        close();
        _10 = false;
        break;
    case MarioStateMsg_Update:
        return update();
    case MarioStateMsg_Notice:
        return notice();
    case MarioStateMsg_Keep:
        return keep();
    }

    return true;
}

void Mario::sendStateMsg(u32 msg) {
    MarioState* pNext;

    for (MarioState* pState = _97C; pState != nullptr;) {
        pNext = pState->_8;

        if (isStatusActive(pState->mStatusId) == MarioStatus_None) {
            pState = pNext;
            continue;
        }

        if (!pState->proc(msg)) {
            if (isStatusActive(pState->mStatusId) != MarioStatus_None) {
                closeStatus(pState);
            }
        } else if (msg == MarioStateMsg_Update) {
            msg = MarioStateMsg_Keep;
        }

        pState = pNext;
    }
}

bool Mario::updatePosture(MtxPtr mtx) {
    if (_97C != nullptr) {
        return _97C->postureCtrl(mtx);
    }

    postureCtrl(mtx);

    return false;
}

bool MarioState::postureCtrl(MtxPtr mtx) {
    getPlayer()->postureCtrl(mtx);

    return false;
}

void Mario::changeStatus(MarioState* pState) {
    if (isStatusActive(pState->mStatusId)) {
        return;
    }

    _980 = pState;

    sendStateMsg(MarioStateMsg_Notice);

    if (_97C) {
        pState->_8 = _97C;
    }

    _97C = pState;

    if (!pState->proc(MarioStateMsg_Start)) {
        closeStatus(pState);

        _97C = pState->_8;
    }
}

void Mario::closeStatus(MarioState* pState) {
    if (pState == nullptr) {
        while (_97C != nullptr) {
            closeStatus(_97C);
        }

        return;
    }

    MarioState* pCurr = _97C;

    if (pCurr == pState) {
        _97C = pState->_8;
    } else {
        while (pCurr != nullptr) {
            MarioState* pNext = pCurr->_8;

            if (pNext == pState) {
                break;
            }

            pCurr = pNext;
        }

        pCurr->_8 = pState->_8;
    }

    pState->_8 = nullptr;
    pState->proc(MarioStateMsg_Close);
}

u32 MarioState::getNoticedStatus() const {
    return getPlayer()->_980->mStatusId;
}

void MarioState::init() {
}

bool MarioState::notice() {
    return false;
}

bool MarioState::keep() {
    return true;
}

void MarioState::hitPoly(u8, const TVec3f&, HitSensor*) {
}

f32 MarioState::getBlurOffset() const {
    return 0.0f;
}

void MarioState::draw3D() const {
}

bool MarioState::start() {
    return true;
}

bool MarioState::close() {
    return true;
}

bool MarioState::update() {
    return true;
}

void MarioState::hitWall(const TVec3f&, HitSensor*) {
}

bool MarioState::passRing(const HitSensor*) {
    return false;
}
