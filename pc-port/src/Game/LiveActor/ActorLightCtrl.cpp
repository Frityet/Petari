#include "Game/LiveActor/ActorLightCtrl.hpp"

#include "Game/Map/LightFunction.hpp"

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
    if (mAreaLightInf != nullptr) {
        const auto* light_info = getTargetActorLight(mAreaLightInf);
        if (light_info != nullptr) {
            mLightInfo = *light_info;
        }
    }
}

void ActorLightCtrl::update(bool direct) {
    tryFindNewAreaLight(direct);
    updateLightBlend();
}

void ActorLightCtrl::loadLight() const {
    if (_1C != 0) {
        LightFunction::loadActorLightInfo(&mLightInfo);
        return;
    }

    if (mAreaLightInf != nullptr) {
        LightFunction::loadActorLightInfo(getTargetActorLight(mAreaLightInf));
    }
}

void ActorLightCtrl::reset() {
    mAreaLightInf = nullptr;
    mLightID = ZoneLightID();
    _1C = 0;
    mInterpolate = -1;
    _54 = -1;
}

void ActorLightCtrl::copy(const ActorLightCtrl* pLight) {
    if (pLight == nullptr) {
        return;
    }

    _4 = pLight->_4;
    _8 = pLight->_8;
    _C = pLight->_C;
    mAreaLightInf = pLight->mAreaLightInf;
    mLightID = pLight->mLightID;
    _1C = pLight->_1C;
    mLightInfo = pLight->mLightInfo;
    mInterpolate = pLight->mInterpolate;
    _54 = pLight->_54;
}

bool ActorLightCtrl::isSameLight(const ActorLightCtrl* pLight) const {
    if (pLight == nullptr || _1C != 0) {
        return false;
    }

    return mAreaLightInf == pLight->mAreaLightInf;
}

const ActorLightInfo* ActorLightCtrl::getActorLight() const {
    if (_1C != 0 || mAreaLightInf == nullptr) {
        return &mLightInfo;
    }

    return getTargetActorLight(mAreaLightInf);
}

void ActorLightCtrl::initActorLightInfo() {
}

void ActorLightCtrl::tryFindNewAreaLight(bool) {
    if (mAreaLightInf == nullptr) {
        mAreaLightInf = LightFunction::getAreaLightInfo(mLightID);
    }
}

void ActorLightCtrl::updateLightBlend() {
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
