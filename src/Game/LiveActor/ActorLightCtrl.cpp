#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/System/DrawBuffer.hpp"
#include "Game/Util.hpp"

ActorLightCtrl::ActorLightCtrl(const LiveActor* pActor) : mActor(pActor), _4(-1), _8(0), _C(0), mAreaLightInf(0), mLightID() {
    _1C = 0;
    mInterpolate = -1;
    _54 = -1;
}

// initActorLightInfo call is getting inlined
void ActorLightCtrl::init(int interpolate, bool /* unused */) {
    if (interpolate >= 0) {
        _4 = interpolate;
        _C = 1;
    }

    initActorLightInfo();
    tryFindNewAreaLight(false);
    mAreaLightInf = LightFunction::getAreaLightInfo(mLightID);
    mLightInfo = *getTargetActorLight(mAreaLightInf);
}

void ActorLightInfo::operator=(const ActorLightInfo& rInfo) {
    u32* pDst = mInfo0.mWords;
    const u32* pSrc = rInfo.mInfo0.mWords;

    for (s32 i = 0; i < 2; i++) {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pDst += 2;
        pSrc += 2;
    }

    pDst[0] = pSrc[0];

    pDst = mInfo1.mWords;
    pSrc = rInfo.mInfo1.mWords;

    for (s32 i = 0; i < 2; i++) {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pDst += 2;
        pSrc += 2;
    }

    pDst[0] = pSrc[0];
    mAlpha2 = rInfo.mAlpha2;
    mColor.r = rInfo.mColor.r;
    mColor.g = rInfo.mColor.g;
    mColor.b = rInfo.mColor.b;
    mColor.a = rInfo.mColor.a;
}

void ActorLightCtrl::update(bool direct) {
    if (!MR::isHiddenModel(mActor)) {
        tryFindNewAreaLight(direct);
        updateLightBlend();
    }
}

void ActorLightCtrl::loadLight() const {
    if (mAreaLightInf) {
        if (_1C) {
            LightFunction::loadActorLightInfo(&mLightInfo);
        } else {
            LightFunction::loadActorLightInfo(getTargetActorLight(mAreaLightInf));
        }
    }
}

void ActorLightCtrl::reset() {
    mLightID.clear();

    if (LightFunction::tryFindNewAreaLightID(mActor->mPosition, &mLightID)) {
        resetCurrentLightInfo();
        _1C = 0;
        mLightInfo = *getTargetActorLight(mAreaLightInf);
    }

    mAreaLightInf = LightFunction::getAreaLightInfo(mLightID);

    if (!_C) {
        _8->resetLightSort(this);
    }
}

void ActorLightCtrl::copy(const ActorLightCtrl* pLight) {
    mAreaLightInf = pLight->mAreaLightInf;
    mLightID = pLight->mLightID;
    _1C = pLight->_1C;
    mLightInfo = pLight->mLightInfo;
    mInterpolate = pLight->mInterpolate;
    _54 = pLight->_54;
}

bool ActorLightCtrl::isSameLight(const ActorLightCtrl* pLight) const {
    if (_1C) {
        return false;
    }

    return mAreaLightInf == pLight->mAreaLightInf;
}

const ActorLightInfo* ActorLightCtrl::getActorLight() const {
    if (_1C) {
        return &mLightInfo;
    }

    return getTargetActorLight(mAreaLightInf);
}

void ActorLightCtrl::initActorLightInfo() {
    if (_C) {
        return;
    }

    MR::findActorLightInfo(mActor);
    return;
}

void ActorLightCtrl::tryFindNewAreaLight(bool direct) {
    if (LightFunction::tryFindNewAreaLightID(mActor->mPosition, &mLightID)) {
        if (mAreaLightInf) {
            _1C = getTargetActorLight(mAreaLightInf);
            mLightInfo = *_1C;
        }

        resetCurrentLightInfo();

        if (mInterpolate == 0 || direct) {
            mInterpolate = 0;
            _1C = 0;
            mLightInfo = *getTargetActorLight(mAreaLightInf);
        }

        if (mInterpolate < 0) {
            mInterpolate = LightFunction::getDefaultStepInterpolate();
        }

        if (!_C) {
            _8->resetLightSort(this);
        }
    }
}

void ActorLightCtrl::updateLightBlend() {
    if (_1C) {
        _54++;

        if (_54 >= mInterpolate) {
            mLightInfo = *getTargetActorLight(mAreaLightInf);
            _1C = 0;
            _54 = -1;

            if (!_C) {
                _8->resetLightSort(this);
            }
        } else {
            f32 rate = static_cast<f32>(_54) / static_cast<f32>(mInterpolate);
            LightFunction::blendActorLightInfo(&mLightInfo, *_1C, *getTargetActorLight(mAreaLightInf), rate);
        }
    }
}

void ActorLightCtrl::resetCurrentLightInfo() {
    mAreaLightInf = LightFunction::getAreaLightInfo(mLightID);
    mInterpolate = mAreaLightInf->mInterpolate;
    mLightInfo.mInfo0.mIsFollowCamera = getTargetActorLight(mAreaLightInf)->mInfo0.mIsFollowCamera;
    mLightInfo.mInfo1.mIsFollowCamera = getTargetActorLight(mAreaLightInf)->mInfo1.mIsFollowCamera;
    _54 = 0;
}

const ActorLightInfo* ActorLightCtrl::getTargetActorLight(const AreaLightInfo* pInfo) const {
    s32 type = _4;

    if (type == 0) {
        return &pInfo->mPlayerLight;
    } else if (type == 1) {
        return &pInfo->mStrongLight;
    } else if (type == 2) {
        return &pInfo->mWeakLight;
    } else if (type == 3) {
        return &pInfo->mPlanetLight;
    }

    return 0;
}
