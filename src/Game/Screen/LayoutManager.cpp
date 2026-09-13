#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutCoreUtil.hpp"
#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <nw4r/lyt/textBox.h>
#include <nw4r/lyt/layout.h>
#include <cstdio>

nw4r::lyt::AnimTransform* LayoutManager::getAnimTransform(const char* pAnimName) const {
    char fileName[128];
    snprintf(fileName, sizeof(fileName), "%s.brlan", pAnimName);
    u32 hash = MR::getHashCodeLower(fileName);
    for (u32 i = 0; i < mLayoutHolder->mAnimRes.mCount; i++) {
        if (mLayoutHolder->isAnimationHashEqual(hash, i)) {
            return mAnimTransList[i];
        }
    }
    return nullptr;
}

void LayoutManager::bindPaneCtrlAnim(LayoutPaneCtrl* pPaneCtrl, nw4r::lyt::AnimTransform* pAnim) {
    u32 index = pPaneCtrl->mPaneIndex;
    mPaneInfos[index].mPane->UnbindAnimationSelf(pAnim);
    mPaneInfos[index].mPane->BindAnimation(pAnim, false);
    index++;
    while (index < pPaneCtrl->mPaneIndex + mPaneInfos[pPaneCtrl->mPaneIndex].mSubtreeSize) {
        bindPaneCtrlAnimSub(index, pAnim);
    }
}

void LayoutManager::bindPaneCtrlAnimSub(u32& rIndex, nw4r::lyt::AnimTransform* pAnim) {
    if (mPaneInfos[rIndex].mPaneCtrl != nullptr) {
        rIndex += mPaneInfos[rIndex].mSubtreeSize;
        return;
    }
    mPaneInfos[rIndex].mPane->UnbindAnimationSelf(pAnim);
    mPaneInfos[rIndex].mPane->BindAnimation(pAnim, false);
    u32 first = rIndex++;
    while (rIndex < first + mPaneInfos[first].mSubtreeSize) {
        bindPaneCtrlAnimSub(rIndex, pAnim);
    }
}

void LayoutManager::unbindPaneCtrlAnim(LayoutPaneCtrl* pPaneCtrl, nw4r::lyt::AnimTransform* pAnim) {
    u32 index = pPaneCtrl->mPaneIndex;
    mPaneInfos[index].mPane->UnbindAnimationSelf(pAnim);
    index++;
    while (index < pPaneCtrl->mPaneIndex + mPaneInfos[pPaneCtrl->mPaneIndex].mSubtreeSize) {
        unbindPaneCtrlAnimSub(index, pAnim);
    }
}

void LayoutManager::unbindPaneCtrlAnimSub(u32& rIndex, nw4r::lyt::AnimTransform* pAnim) {
    if (mPaneInfos[rIndex].mPaneCtrl != nullptr) {
        rIndex += mPaneInfos[rIndex].mSubtreeSize;
        return;
    }
    mPaneInfos[rIndex].mPane->UnbindAnimationSelf(pAnim);
    u32 first = rIndex++;
    while (rIndex < first + mPaneInfos[first].mSubtreeSize) {
        unbindPaneCtrlAnimSub(rIndex, pAnim);
    }
}

void LayoutManager::initPaneInfo() {
    mPaneCount = countPanes(mLayout->mpRootPane);
    mPaneInfos = new LayoutPaneInfo[mPaneCount];
    u32 index = 0;
    initPaneInfoRecursive(index, mLayout->mpRootPane);
}

void LayoutManager::initPaneInfoRecursive(u32& rIndex, nw4r::lyt::Pane* pPane) {
    mPaneInfos[rIndex].mName = pPane->mName;
    mPaneInfos[rIndex].mPaneCtrl = nullptr;
    mPaneInfos[rIndex].mGroupCtrlList = nullptr;
    mPaneInfos[rIndex].mMtxRef = nullptr;
    mPaneInfos[rIndex].mPane = pPane;
    u32 first = rIndex++;
    nw4r::lyt::PaneList& children = pPane->mChildList;
    for (nw4r::lyt::PaneList::Iterator it = children.GetBeginIter(); it != children.GetEndIter(); ++it) {
        initPaneInfoRecursive(rIndex, &*it);
    }
    mPaneInfos[first].mSubtreeSize = rIndex - first;
}

u32 LayoutManager::countPanes(nw4r::lyt::Pane* pPane) {
    u32 count = 1;
    nw4r::lyt::PaneList& children = pPane->mChildList;
    for (nw4r::lyt::PaneList::Iterator it = children.GetBeginIter(); it != children.GetEndIter(); ++it) {
        count += countPanes(&*it);
    }
    return count;
}

void LayoutManager::initTextBoxRecursive(nw4r::lyt::Pane* pPane, nw4r::lyt::Pane* pUserDataPane, const char* pLayoutName, u32 allocSize) {
    nw4r::lyt::TextBox* textBox = nw4r::ut::DynamicCast<nw4r::lyt::TextBox*>(pPane);
    char userData[9];
    LayoutCoreUtil::getPaneUserData(pPane, userData);
    if (!MR::isEqualString("", userData)) {
        pUserDataPane = pPane;
    }

    if (textBox != nullptr) {
        if (pUserDataPane != nullptr) {
            LayoutCoreUtil::getPaneUserData(pUserDataPane, userData);
            char messageId[256];
            MR::getLayoutMessageID(messageId, pLayoutName, userData);
            LayoutCoreUtil::initTextBoxPane(textBox, messageId, 256);
        } else {
            LayoutCoreUtil::initTextBoxPane(textBox, nullptr, allocSize);
        }
    }

    nw4r::lyt::PaneList& children = pPane->mChildList;
    for (nw4r::lyt::PaneList::Iterator it = children.GetBeginIter(); it != children.GetEndIter(); ++it) {
        initTextBoxRecursive(&*it, pUserDataPane, pLayoutName, allocSize);
    }
}
