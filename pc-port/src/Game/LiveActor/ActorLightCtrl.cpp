#include "Game/LiveActor/ActorLightCtrl.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace {
    void resetLightSort(DrawBuffer*, const ActorLightCtrl*) {
    }
}  // namespace

ActorLightCtrl::ActorLightCtrl(const LiveActor* pActor) : mActor(pActor), _4(-1), _8(0), _C(0), mAreaLightInf(nullptr), mLightID() {
    _1C = 0;
    mInterpolate = -1;
    _54 = -1;
}

void ActorLightCtrl::init(int interpolate, bool) {
    if (interpolate >= 0) {
        _4 = interpolate;
        _C = 1;
    }

    initActorLightInfo();
    tryFindNewAreaLight(false);
    mAreaLightInf = LightFunction::getAreaLightInfo(mLightID);
    const ActorLightInfo* lightInfo = getTargetActorLight(mAreaLightInf);
    if (lightInfo != nullptr) {
        mLightInfo = *lightInfo;
    }
}

void ActorLightInfo::operator=(const ActorLightInfo& rInfo) {
    mInfo0 = rInfo.mInfo0;
    mInfo1 = rInfo.mInfo1;
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
    if (mAreaLightInf != nullptr) {
        if (_1C != nullptr) {
            LightFunction::loadActorLightInfo(&mLightInfo);
        } else {
            LightFunction::loadActorLightInfo(getTargetActorLight(mAreaLightInf));
        }
    }
}

void ActorLightCtrl::reset() {
    mLightID.clear();

    if (mActor != nullptr && LightFunction::tryFindNewAreaLightID(mActor->mPosition, &mLightID)) {
        resetCurrentLightInfo();
        _1C = 0;

        const ActorLightInfo* target = getTargetActorLight(mAreaLightInf);
        if (target != nullptr) {
            mLightInfo = *target;
        }
    }

    mAreaLightInf = LightFunction::getAreaLightInfo(mLightID);

    if (!_C) {
        resetLightSort(_8, this);
    }
}

void ActorLightCtrl::copy(const ActorLightCtrl* pLight) {
    if (pLight == nullptr) {
        return;
    }

    mAreaLightInf = pLight->mAreaLightInf;
    mLightID = pLight->mLightID;
    _1C = pLight->_1C;
    mLightInfo = pLight->mLightInfo;
    mInterpolate = pLight->mInterpolate;
    _54 = pLight->_54;
}

bool ActorLightCtrl::isSameLight(const ActorLightCtrl* pLight) const {
    if (pLight == nullptr || _1C != nullptr) {
        return false;
    }

    return mAreaLightInf == pLight->mAreaLightInf;
}

const ActorLightInfo* ActorLightCtrl::getActorLight() const {
    if (_1C != nullptr || mAreaLightInf == nullptr) {
        return &mLightInfo;
    }

    return getTargetActorLight(mAreaLightInf);
}

void ActorLightCtrl::initActorLightInfo() {
}

void ActorLightCtrl::tryFindNewAreaLight(bool direct) {
    if (mAreaLightInf == nullptr) {
        mAreaLightInf = LightFunction::getAreaLightInfo(mLightID);
        const ActorLightInfo* target = getTargetActorLight(mAreaLightInf);
        if (target != nullptr) {
            mLightInfo = *target;
        }
    }

    if (mActor == nullptr) {
        return;
    }

    if (LightFunction::tryFindNewAreaLightID(mActor->mPosition, &mLightID)) {
        if (mAreaLightInf != nullptr) {
            _1C = getTargetActorLight(mAreaLightInf);
            if (_1C != nullptr) {
                mLightInfo = *_1C;
            }
        }

        resetCurrentLightInfo();

        const ActorLightInfo* target = getTargetActorLight(mAreaLightInf);
        if ((mInterpolate == 0 || direct) && target != nullptr) {
            mInterpolate = 0;
            _1C = 0;
            mLightInfo = *target;
        }

        if (mInterpolate < 0) {
            mInterpolate = LightFunction::getDefaultStepInterpolate();
        }

        if (!_C) {
            resetLightSort(_8, this);
        }
    }

    if (mAreaLightInf == nullptr) {
        mAreaLightInf = LightFunction::getAreaLightInfo(mLightID);
    }
}

void ActorLightCtrl::updateLightBlend() {
    if (_1C != nullptr) {
        const ActorLightInfo* target = getTargetActorLight(mAreaLightInf);
        if (target == nullptr || mInterpolate <= 0) {
            _1C = nullptr;
            _54 = -1;
            return;
        }

        _54++;

        if (_54 >= mInterpolate) {
            mLightInfo = *target;
            _1C = 0;
            _54 = -1;

            if (!_C) {
                resetLightSort(_8, this);
            }
        } else {
            f32 rate = static_cast< f32 >(_54) / static_cast< f32 >(mInterpolate);
            LightFunction::blendActorLightInfo(&mLightInfo, *_1C, *target, rate);
        }
    }
}

void ActorLightCtrl::resetCurrentLightInfo() {
    mAreaLightInf = LightFunction::getAreaLightInfo(mLightID);
    if (mAreaLightInf == nullptr) {
        mInterpolate = -1;
        _54 = -1;
        return;
    }

    const auto* target_light = getTargetActorLight(mAreaLightInf);
    if (target_light == nullptr) {
        mInterpolate = -1;
        _54 = -1;
        return;
    }

    mInterpolate = mAreaLightInf->mInterpolate;
    mLightInfo.mInfo0.mIsFollowCamera = target_light->mInfo0.mIsFollowCamera;
    mLightInfo.mInfo1.mIsFollowCamera = target_light->mInfo1.mIsFollowCamera;
    _54 = 0;
}

const ActorLightInfo* ActorLightCtrl::getTargetActorLight(const AreaLightInfo* pInfo) const {
    if (pInfo == nullptr) {
        return nullptr;
    }

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

    return nullptr;
}
