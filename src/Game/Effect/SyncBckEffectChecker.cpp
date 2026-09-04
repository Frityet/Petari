#include "Game/Effect/SyncBckEffectChecker.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Effect/SyncBckEffectInfo.hpp"
#include "Game/Util/MathUtil.hpp"

SyncBckEffectChecker::SyncBckEffectChecker(XanimePlayer* pPlayer) : _0(pPlayer), _4(0.0f), _8(false), _C(nullptr), _10(nullptr) {
}

void SyncBckEffectChecker::updateBefore() {
    bool isRunning = !_0->_24[_0->_54].checkState(1) ? (_0->_20->getRate() != 0.0f || _4 != _0->_20->getFrame()) : false;
    _C = isRunning ? _0->getCurrentBckName() : nullptr;
}

void SyncBckEffectChecker::updateAfter() {
    _8 = false;
    _10 = _C;
    _4 = _0->_20->getFrame();
}

void SyncBckEffectChecker::reset() {
    _8 = true;
    _4 = 0.0f;
}

bool SyncBckEffectChecker::isCreate(const SyncBckEffectInfo* pInfo, bool isOneTime) const {
    if (!pInfo->isRegisteredBck(_C)) {
        return false;
    }

    if (!isOneTime) {
        return true;
    }

    if (!pInfo->isRegisteredBck(_C)) {
        return false;
    }

    f32 startFrame = pInfo->mStartFrame;
    XanimeFrameCtrl* pFrameCtrl = _0->_20;
    if (_8 && pFrameCtrl->getRate() != 0.0f && MR::isNearZero(startFrame + pFrameCtrl->getRate() - pFrameCtrl->getFrame())) {
        return true;
    }

    return checkPass(startFrame);
}

bool SyncBckEffectChecker::isDelete(const SyncBckEffectInfo* pInfo) const {
    if (!pInfo->isRegisteredBck(_C)) {
        if (pInfo->isBckLoop(_C)) {
            return _C != _10;
        }

        if (pInfo->mContinueBckEnd) {
            const char* pName = _0->getCurrentBckName();
            if (pName == nullptr) {
                return false;
            }

            if (!pInfo->isRegisteredBck(pName)) {
                return _0->isTerminate(pName);
            }
        } else {
            return _C != _10;
        }
    }

    if (!MR::Effect::isExistSyncBckDeleteFrame(pInfo)) {
        return false;
    }

    return checkPass(pInfo->mEndFrame);
}

bool SyncBckEffectChecker::checkPass(f32 frame) const {
    if (_0->_20->getRate() == 0.0f) {
        return checkPassIfRate0(frame);
    }

    return _0->checkPass(frame) == true;
}

bool SyncBckEffectChecker::checkPassIfRate0(f32 frame) const {
    XanimeFrameCtrl* pFrameCtrl = _0->_20;
    f32 currentFrame = pFrameCtrl->getFrame();

    if (pFrameCtrl->getAttribute() == 2 && currentFrame < _4) {
        if ((_4 <= frame && frame < pFrameCtrl->getEnd()) || (pFrameCtrl->getLoop() <= frame && frame < currentFrame)) {
            return true;
        }
    } else if (_4 <= currentFrame) {
        if (_4 <= frame && frame < currentFrame) {
            return true;
        }
    } else if (currentFrame <= frame && frame < _4) {
        return true;
    }

    return false;
}
