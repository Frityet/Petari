#include "Game/LiveActor/ActorPadAndCameraCtrl.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>

namespace {
    const char* sFileName = "PadAndCameraCtrl";
}

ActorPadAndCameraCtrl::ActorPadAndCameraCtrl(const ModelManager* pModelManager, const TVec3f* pPosition)
    : _0(pModelManager), _4(pPosition), _8(pModelManager->getResourceHolder()), _C(nullptr), mInfoNum(0), mInfo(nullptr) {
    JMapInfo* parser = MR::tryCreateCsvParser(_8, "%s.bcsv", sFileName);
    if (parser == nullptr) {
        return;
    }

    s32 infoNum = 0;
    if (parser != nullptr) {
        infoNum = MR::getCsvDataElementNum(parser);
    }
    mInfoNum = infoNum;
    ActorPadAndCameraCtrlInfo* infoArray = new ActorPadAndCameraCtrlInfo[mInfoNum];
    mInfo = infoArray;
    if (mInfoNum > 0) {
        const char* distanceInvalidKey = "DistanceInvalid";
        for (s32 i = 0; i < mInfoNum; i++) {
            ActorPadAndCameraCtrlInfo* info = &infoArray[i];
            MR::getCsvDataStr(&info->mBckName, parser, "BckName", i);
            MR::getCsvDataF32(&info->mStartFrame, parser, "StartFrame", i);
            MR::getCsvDataF32(&info->mEndFrame, parser, "EndFrame", i);
            MR::getCsvDataStrOrNULL(&info->mPadRumbleName, parser, "PadRumbleName", i);
            MR::getCsvDataStrOrNULL(&info->mCameraShakeName, parser, "CameraShakeName", i);
            MR::getCsvDataF32(&info->mDistanceNear, parser, "DistanceNear", i);
            MR::getCsvDataF32(&info->mDistanceFar, parser, "DistanceFar", i);
            info->mDistanceInvalid = 3000.0f;
            parser->getValue(i, distanceInvalidKey, &info->mDistanceInvalid);
            MR::getCsvDataStrOrNULL(&info->mPadRumbleNameMiddle, parser, "PadRumbleNameMiddle", i);
            MR::getCsvDataStrOrNULL(&info->mPadRumbleNameFar, parser, "PadRumbleNameFar", i);
            MR::getCsvDataStrOrNULL(&info->mCameraShakeNameMiddle, parser, "CameraShakeNameMiddle", i);
            MR::getCsvDataStrOrNULL(&info->mCameraShakeNameFar, parser, "CameraShakeNameFar", i);
            info->_30 = false;
            info->_31 = false;
        }
    }
}

ActorPadAndCameraCtrl* ActorPadAndCameraCtrl::tryCreate(const ModelManager* pModelManager, const TVec3f* pPosition) {
    const ResourceHolder* resourceHolder = pModelManager->getResourceHolder();
    if (!MR::isExistFileInArc(resourceHolder, "%s.bcsv", sFileName)) {
        return nullptr;
    }
    if (resourceHolder->mMotionResTable->mCount == 0) {
        return nullptr;
    }
    return new ActorPadAndCameraCtrl(pModelManager, pPosition);
}

void ActorPadAndCameraCtrl::update() {
    if (_C != _0->getPlayingBckName()) {
        _C = _0->getPlayingBckName();
        updateInfoBck();
    }

    J3DFrameCtrl* frameCtrl = _0->getBckCtrl();
    for (s32 i = 0; i < mInfoNum; i++) {
        ActorPadAndCameraCtrlInfo* info = &mInfo[i];
        if (info->_30) {
            if (frameCtrl->checkPass(info->mStartFrame)) {
                info->_31 = true;
            }
            if (info->_31) {
                updatePadAndCamera(info);
                if (info->mEndFrame < 0.0f || frameCtrl->getRate() <= 0.0f || frameCtrl->checkPass(info->mEndFrame)) {
                    info->_31 = false;
                }
            }
        }
    }
}

void ActorPadAndCameraCtrl::updateInfoBck() {
    if (_C != nullptr) {
        for (s32 i = 0; i < mInfoNum; i++) {
            ActorPadAndCameraCtrlInfo* info = &mInfo[i];
            if (info->mBckName != nullptr) {
                if (MR::isEqualStringCase(_C, info->mBckName)) {
                    info->_30 = true;
                } else {
                    info->_30 = false;
                }
            }
        }
    }
}

namespace {
    void tryUpdateCameraShake(const char* pName) {
        if (pName == nullptr) {
            return;
        }
        if (MR::isEqualStringCase(pName, "最強")) {
            MR::shakeCameraVeryStrong();
        } else if (MR::isEqualStringCase(pName, "強")) {
            MR::shakeCameraStrong();
        } else if (MR::isEqualStringCase(pName, "中強")) {
            MR::shakeCameraNormalStrong();
        } else if (MR::isEqualStringCase(pName, "中")) {
            MR::shakeCameraNormal();
        } else if (MR::isEqualStringCase(pName, "中弱")) {
            MR::shakeCameraNormalWeak();
        } else if (MR::isEqualStringCase(pName, "弱")) {
            MR::shakeCameraWeak();
        } else if (MR::isEqualStringCase(pName, "最弱")) {
            MR::shakeCameraVeryWeak();
        }
    }

    bool isDistanceExistAndFar(f32 distance, f32 threshold) {
        if (threshold < 0.0f) {
            return false;
        }
        return distance > threshold;
    }
}  // namespace

void ActorPadAndCameraCtrl::updatePadAndCamera(const ActorPadAndCameraCtrlInfo* pInfo) {
    f32 distance = MR::getPlayerPos()->distance(*_4);
    isDistanceExistAndFar(distance, pInfo->mDistanceInvalid);
    if (isDistanceExistAndFar(distance, pInfo->mDistanceFar)) {
        if (pInfo->mPadRumbleNameFar != nullptr) {
            MR::tryRumblePad(this, pInfo->mPadRumbleNameFar, 0);
        }
        tryUpdateCameraShake(pInfo->mCameraShakeNameFar);
    } else if (isDistanceExistAndFar(distance, pInfo->mDistanceNear)) {
        if (pInfo->mPadRumbleNameMiddle != nullptr) {
            MR::tryRumblePad(this, pInfo->mPadRumbleNameMiddle, 0);
        }
        tryUpdateCameraShake(pInfo->mCameraShakeNameMiddle);
    } else {
        if (pInfo->mPadRumbleName != nullptr) {
            MR::tryRumblePad(this, pInfo->mPadRumbleName, 0);
        }
        tryUpdateCameraShake(pInfo->mCameraShakeName);
    }
}
