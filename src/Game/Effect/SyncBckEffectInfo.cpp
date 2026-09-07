#include "Game/Effect/SyncBckEffectInfo.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/Util/StringUtil.hpp"
#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>

SyncBckEffectInfo::BckResourceInfo::BckResourceInfo(const XanimePlayer* pPlayer, const char* pName) : mName(pName), mResource(nullptr) {
    mResource = static_cast< J3DAnmTransform* >(pPlayer->mResourceTable->findResMotion(pName));
}

bool SyncBckEffectInfo::BckResourceInfo::isLoop() const {
    return mResource->getAttribute() == 2 || mResource->getAttribute() == 4;
}

SyncBckEffectInfo::SyncBckEffectInfo(const XanimePlayer* pPlayer, const char* pName, s32 count, f32 startFrame, f32 endFrame, bool continueBckEnd)
    : mBckResources(), mStartFrame(startFrame), mEndFrame(endFrame), mContinueBckEnd(continueBckEnd) {
    mBckResources.init(count);
    addBck(pPlayer, pName);
}

void SyncBckEffectInfo::addBck(const XanimePlayer* pPlayer, const char* pName) {
    mBckResources.push_back(new BckResourceInfo(pPlayer, pName));
}

bool SyncBckEffectInfo::isRegisteredBck(const char* pName) const {
    if (pName == nullptr) {
        return false;
    }

    for (BckResourceInfo* const* pInfo = mBckResources.begin(); pInfo != mBckResources.end(); pInfo++) {
        if (MR::isEqualStringCase(pName, (*pInfo)->mName)) {
            return true;
        }
    }

    return false;
}

bool SyncBckEffectInfo::isBckLoop(const char* pName) const {
    if (pName == nullptr) {
        return false;
    }

    BckResourceInfo* pResource = nullptr;
    for (BckResourceInfo* const* pInfo = mBckResources.begin(); pInfo != mBckResources.end(); pInfo++) {
        if (MR::isEqualStringCase((*pInfo)->mName, pName)) {
            pResource = *pInfo;
            break;
        }
    }

    if (pResource == nullptr) {
        return false;
    }

    return pResource->isLoop();
}

namespace MR {
    namespace Effect {
        bool isExistSyncBckDeleteFrame(const SyncBckEffectInfo* pInfo) {
            f32 endFrame = pInfo->mEndFrame;
            return 0.0f <= endFrame;
        }
    }
}
