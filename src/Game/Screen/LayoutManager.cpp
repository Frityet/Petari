#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutCoreUtil.hpp"
#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "Game/System/Language.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <nw4r/lyt/textBox.h>
#include <nw4r/lyt/layout.h>
#include <cstdio>
#include <cstring>

namespace {
    namespace Local {
        template < int N >
        class BitFlag {
        public:
            BitFlag() {
                MR::zeroMemory(mFlags, sizeof(mFlags));
            }

            virtual ~BitFlag() {}

            virtual void onBit(int index) {
                mFlags[index / 32] |= 1 << (index % 32);
            }

            virtual void offBit(int index) {
                mFlags[index / 32] &= ~(1 << (index % 32));
            }

            virtual bool isTrue(int index) const {
                return (mFlags[index / 32] & (1 << (index % 32))) != 0;
            }

            virtual bool isAnythingTrue() const {
                for (int i = 0; i < N / 32; i++) {
                    if (mFlags[i] != 0) {
                        return true;
                    }
                }
                return false;
            }

            u32 mFlags[N / 32];
        };
    };  // namespace Local
};  // namespace

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

void LayoutManager::removeUnnecessaryPanes(nw4r::lyt::Pane* pPane) {
    Local::BitFlag< 128 > languagePanes;
    Local::BitFlag< 128 > currentLanguagePanes;
    nw4r::lyt::PaneList& children = pPane->mChildList;
    s32 index = -1;
    for (nw4r::lyt::PaneList::Iterator it = children.GetBeginIter(); it != children.GetEndIter(); ++it) {
        ++index;
        const char* name = (*it).mName;
        u32 length = strlen(name);
        if (length < 4) {
            continue;
        }
        const char* suffix = name + length - 4;
        for (u32 language = 0; language < MR::getLanguageNum(); ++language) {
            if (strncmp(suffix, MR::getLanguagePrefixByIndex(language), 4) == 0) {
                languagePanes.onBit(index);
                if (strncmp(suffix, MR::getCurrentLanguagePrefix(), 4) == 0) {
                    currentLanguagePanes.onBit(index);
                }
                break;
            }
        }
    }

    if (languagePanes.isAnythingTrue()) {
        index = 0;
        nw4r::lyt::PaneList::Iterator it = children.GetBeginIter();
        if (currentLanguagePanes.isAnythingTrue()) {
            while (it != children.GetEndIter()) {
                nw4r::lyt::PaneList::Iterator current = it++;
                if (currentLanguagePanes.isTrue(index)) {
                    char name[20];
                    const char* source = (*current).mName;
                    u32 length = strlen(source) - 4;
                    strncpy(name, source, length);
                    name[length] = '\0';
                    (*current).SetName(name);
                } else {
                    pPane->RemoveChild(&*current);
                }
                ++index;
            }
        } else {
            while (it != children.GetEndIter()) {
                nw4r::lyt::PaneList::Iterator current = it++;
                if (languagePanes.isTrue(index)) {
                    pPane->RemoveChild(&*current);
                }
                ++index;
            }
        }
    } else {
        for (nw4r::lyt::PaneList::Iterator it = children.GetBeginIter(); it != children.GetEndIter(); ++it) {
            removeUnnecessaryPanes(&*it);
        }
    }
}
