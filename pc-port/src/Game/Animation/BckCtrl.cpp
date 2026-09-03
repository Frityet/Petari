#include "Game/Animation/BckCtrl.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/StringUtil.hpp"

namespace {
    static const char* sDefaultPlayDataName = "_default";

    inline int compareControlNames(const BckCtrlData& rLeft, const BckCtrlData& rRight) {
        if (rLeft.mName[0] == '\0') {
            if (rRight.mName[0] == '\0') {
                return 0;
            }
            return -1;
        }
        if (rRight.mName[0] == '\0') {
            if (rLeft.mName[0] == '\0') {
                return 0;
            }
            return 1;
        }
        return MR::strcasecmp(rLeft.mName, rRight.mName);
    }
};  // namespace

BckCtrl::BckCtrl(ResourceHolder* pResHolder, const char* pResName) : mControlData(nullptr), mControlDataCount(0), _1C(0) {
    mDefaultCtrlData.mName = sDefaultPlayDataName;
    s32 numCtrl = 0;
    JMapInfo info;
    if (pResHolder->mBanmtResTable->isExistRes(pResName)) {
        info.attach(pResHolder->mBanmtResTable->getRes(pResName));
        numCtrl = info.getNumEntries();
    }

    if (numCtrl > 0) {
        mControlData = new BckCtrlData[numCtrl];
        mControlDataCount = numCtrl;
    }

    if (info.mData != nullptr) {
        for (int i = 0; i < info.getNumEntries(); i++) {
            JMapInfoIter iter(&info, i);

            const char* name = "";
            iter.getValue("name", &name);
            s32 interpole = -1;
            iter.getValue("interpole", &interpole);
            s32 playFrame = -1;
            iter.getValue("play_frame", &playFrame);
            s32 startFrame = -1;
            iter.getValue("start_frame", &startFrame);
            s32 endFrame = -1;
            iter.getValue("end_frame", &endFrame);
            s32 attribute = -1;
            iter.getValue("attribute", &attribute);

            BckCtrlData data;
            data.mName = name;
            data.mInterpole = interpole;
            data.mPlayFrame = playFrame;
            data.mStartFrame = startFrame;
            data.mEndFrame = endFrame;
            data.mRepeatFrame = -1;

            data.mLoopMode = attribute >= 0 ? (u8)attribute : 0xFF;

            overWrite(data);
        }
    }
}

void BckCtrl::add(const BckCtrlData& rNew) {
    if (mControlDataCount > _1C) {
        mControlData[_1C++] = rNew;
        s32 insertIndex = 0;
        for (s32 i = _1C - 1; i > 0; i--) {
            if (compareControlNames(mControlData[i - 1], rNew) < 0) {
                insertIndex = i;
                break;
            }
            mControlData[i] = mControlData[i - 1];
        }
        mControlData[insertIndex] = rNew;
    }
}

void BckCtrlData::operator=(const BckCtrlData& rOther) {
    mName = rOther.mName;
    mPlayFrame = rOther.mPlayFrame;
    mStartFrame = rOther.mStartFrame;
    mEndFrame = rOther.mEndFrame;
    mRepeatFrame = rOther.mRepeatFrame;
    mInterpole = rOther.mInterpole;
    mLoopMode = rOther.mLoopMode;
    _F = rOther._F;
    _10 = rOther._10;
    _11 = rOther._11;
}

void BckCtrl::overWrite(const BckCtrlData& rNew) {
    if (MR::isEqualStringCase(rNew.mName, ::sDefaultPlayDataName)) {
        mDefaultCtrlData = rNew;
    } else {
        BckCtrlData* data = find(rNew.mName);

        if (data != nullptr) {
            *data = rNew;
        } else {
            add(rNew);
        }
    }
}

void BckCtrl::changeBckSetting(const char* pName, XanimePlayer* pPlayer) const {
    BckCtrlData* data = find(pName);

    if (data != nullptr) {
        bool isValidReflect = data->mInterpole >= 0 || data->mPlayFrame >= 0 || data->mStartFrame >= 0 || data->mEndFrame >= 0 ||
                              data->mRepeatFrame >= 0 || data->mLoopMode != 0xFF;

        if (isValidReflect) {
            BckCtrlFunction::reflectBckCtrlData(*data, pPlayer);
            return;
        }
    }

    BckCtrlFunction::reflectBckCtrlData(mDefaultCtrlData, pPlayer);
}

BckCtrlData* BckCtrl::find(const char* pName) const {
    BckCtrlData* pFirst = mControlData;
    s32 count = &mControlData[_1C] - mControlData;
    while (count > 0) {
        s32 half = count / 2;
        BckCtrlData* pMiddle = &pFirst[half];
        if (MR::strcasecmp(pMiddle->mName, pName) < 0) {
            pFirst = pMiddle + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    if (pFirst != &mControlData[_1C] && MR::strcasecmp(pFirst->mName, pName) == 0) {
        return pFirst;
    }
    return nullptr;
}
void BckCtrlFunction::reflectBckCtrlData(const BckCtrlData& rData, XanimePlayer* pPlayer) {
    XanimeFrameCtrl* pCtrl = pPlayer->_20;
    if (rData.mStartFrame >= 0 && rData.mStartFrame <= pCtrl->getEnd()) {
        pCtrl->setStart(rData.mStartFrame);
        pCtrl->setLoop(rData.mStartFrame);
        pPlayer->_84 = pPlayer->_20->getFrame();
    }
    if (rData.mEndFrame >= 0 && rData.mEndFrame <= pCtrl->getEnd()) {
        pCtrl->setEnd(rData.mEndFrame);
    }
    if (rData.mRepeatFrame >= 0 && rData.mRepeatFrame <= pCtrl->getEnd()) {
        pCtrl->setLoop(rData.mRepeatFrame);
    }
    if (rData.mPlayFrame >= 0) {
        f32 rate = 0.0f;
        if (rData.mPlayFrame != 0) {
            rate = 1.0f * (static_cast< f32 >(pCtrl->getEnd() - pCtrl->getStart()) / rData.mPlayFrame);
        }
        pPlayer->changeSpeed(rate);
    }
    if (rData.mInterpole >= 0) {
        pPlayer->changeInterpoleFrame(rData.mInterpole);
    }
    if (rData.mLoopMode != 0xFF) {
        pCtrl->setAttribute(rData.mLoopMode);
    }
}
