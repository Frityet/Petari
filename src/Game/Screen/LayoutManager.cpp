#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutCoreUtil.hpp"
#include "Game/Screen/LayoutGroupCtrl.hpp"
#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/System/Language.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include "nw4r/lyt/group.h"
#include "nw4r/lyt/layout.h"
#include "nw4r/lyt/material.h"
#include "nw4r/lyt/pane.h"
#include "nw4r/lyt/picture.h"
#include "nw4r/lyt/texMap.h"
#include "nw4r/lyt/textBox.h"
#include "nw4r/ut/Rect.h"
#include "nw4r/ut/RuntimeTypeInfo.h"
#include "revolution/mtx.h"
#include "revolution/types.h"
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


namespace {
    static const char* const cRemoveString[] = {
        "4x3",
        "16x9",
        "Replace",
    };
}

LayoutManager::LayoutManager(const char* pLayoutName, bool a2, u32 rootPaneAnimLayerNum, u32 textBoxBufferLength)
    : mLayoutHolder(), mLayout(), mAnimTransList(), mDrawInfo(), mIsScreenHidden(), _61(true), mIndDummyTexMap(), mPaneCount(), mPaneInfoList(),
      mGroupCtrlCount(), mGroupCtrlList(), mLayoutName() {
    if (a2) {
        char fileNameWithoutExtension[0x60];
        char fileNameFromPrefix[0x80];
        MR::makeLayoutArchiveFileNameFromPrefix(fileNameFromPrefix, sizeof(fileNameFromPrefix), pLayoutName, true);
        MR::removeExtensionString(fileNameWithoutExtension, sizeof(fileNameWithoutExtension), fileNameFromPrefix);
        pLayoutName = MR::getBasename(fileNameWithoutExtension);
    }

    char archiveName[0x40];
    snprintf(archiveName, sizeof(archiveName), "%s.arc", pLayoutName);

    char fileName[0x40];
    MR::copyString(fileName, pLayoutName, sizeof(fileName));

    for (u32 i = 0; i < ARRAY_SIZE(cRemoveString); i++) {
        char* pos = strstr(fileName, cRemoveString[i]);

        if (pos != nullptr) {
            pos[0] = '\0';
            break;
        }
    }

    u32 len = strlen(fileName) + 1;
    mLayoutName = new char[len];
    MR::copyString(mLayoutName, fileName, len);

    initArc(archiveName, pLayoutName);
    initPaneInfo();
    initGroupCtrlList();
    initDrawInfo();
    initTextBoxRecursive(mLayout->mpRootPane, nullptr, mLayoutName, textBoxBufferLength);
    replaceIndDummyTexture();

    if (rootPaneAnimLayerNum != 0) {
        createAndAddRootPaneCtrl(rootPaneAnimLayerNum);
    }

    mLayout->Animate(0);
    mLayout->CalculateMtx(mDrawInfo);
}

void LayoutManager::movement() {
    for (u32 i = 0; i < mPaneCount; i++) {
        if (mPaneInfoList[i].mPaneCtrl != nullptr) {
            mPaneInfoList[i].mPaneCtrl->movement();
        }
    }

    for (u32 i = 0; i < mGroupCtrlCount; i++) {
        if (mGroupCtrlList[i] != nullptr) {
            mGroupCtrlList[i]->movement();
        }
    }
}

void LayoutManager::calcAnim() {
    if (MR::isScreen16Per9()) {
        nw4r::math::VEC2 scale;
        scale.x = 0.75f;
        scale.y = 1.0f;
        mDrawInfo.SetLocationAdjustScale(scale);
    } else {
        nw4r::math::VEC2 scale;
        scale.x = 1.0f;
        scale.y = 1.0f;
        mDrawInfo.SetLocationAdjustScale(scale);
    }

    calcAnimWithoutLocationAdjust(mDrawInfo);
}

void LayoutManager::draw() const {
    if (mIsScreenHidden) {
        return;
    }

    MR::setupDrawForNW4RLayout(1.0f, true);
    mLayout->Draw(mDrawInfo);
}

void LayoutManager::addPaneCtrl(LayoutPaneCtrl* pPaneCtrl) {
    s32 index = getIndexOfPane(pPaneCtrl->mPane->mName);

    if (mPaneInfoList[index].mPaneCtrl == nullptr) {
        pPaneCtrl->mPaneIndex = index;
        mPaneInfoList[index].mPaneCtrl = pPaneCtrl;
    }
}

LayoutPaneCtrl* LayoutManager::createAndAddRootPaneCtrl(u32 animLayerNum) {
    s32 index = getIndexOfPane(mLayout->mpRootPane->mName);

    if (mPaneInfoList[index].mPaneCtrl != nullptr) {
        return mPaneInfoList[index].mPaneCtrl;
    }

    LayoutPaneCtrl* pPaneCtrl = new LayoutPaneCtrl(this, mLayout->mpRootPane->mName, animLayerNum);
    addPaneCtrl(pPaneCtrl);
    return pPaneCtrl;
}

LayoutPaneCtrl* LayoutManager::createAndAddPaneCtrl(const char* pName, u32 animLayerNum) {
    s32 index = getIndexOfPane(pName);

    if (mPaneInfoList[index].mPaneCtrl != nullptr) {
        return mPaneInfoList[index].mPaneCtrl;
    }

    LayoutPaneCtrl* pPaneCtrl = new LayoutPaneCtrl(this, pName, animLayerNum);
    addPaneCtrl(pPaneCtrl);
    return pPaneCtrl;
}

LayoutPaneCtrl* LayoutManager::getPaneCtrl(const char* pName) const {
    if (pName == nullptr) {
        return mPaneInfoList[0].mPaneCtrl;
    }

    s32 index = getIndexOfPane(pName);
    return mPaneInfoList[index].mPaneCtrl;
}

s32 LayoutManager::getIndexOfPane(const char* pName) const {
    u32 paneCount = mPaneCount;

    for (u32 i = 0; i < paneCount; i++) {
        if (strcmp(mPaneInfoList[i].mName, pName) == 0) {
            return i;
        }
    }

    return 0;
}

bool LayoutManager::isExistPaneCtrl(const char* pName) const {
    if (pName == nullptr) {
        return mPaneInfoList[0].mPaneCtrl != nullptr;
    }

    s32 index = getIndexOfPane(pName);
    return mPaneInfoList[index].mPaneCtrl != nullptr;
}

void LayoutManager::initArc(const char* pArchiveName, const char* pLayoutName) {
    mLayoutHolder = MR::createAndAddLayoutHolder(pArchiveName);

    char layoutResName[0x80];
    snprintf(layoutResName, sizeof(layoutResName), "%s.brlyt", pLayoutName);
    void* pLayoutRes = mLayoutHolder->GetResource('blyt', layoutResName, nullptr);

    mLayout = new nw4r::lyt::Layout();
    mLayout->Build(pLayoutRes, mLayoutHolder);

    if (mLayoutHolder->mAnimRes.mCount != 0) {
        mAnimTransList = new nw4r::lyt::AnimTransform*[mLayoutHolder->mAnimRes.mCount];

        for (u32 i = 0; i < mLayoutHolder->mAnimRes.mCount; i++) {
            void* pAnimRes = mLayoutHolder->mAnimRes.getRes(i);
            mAnimTransList[i] = mLayout->CreateAnimTransform(pAnimRes, mLayoutHolder);
        }
    }

    removeUnnecessaryPanes(mLayout->mpRootPane);
}

void LayoutManager::initDrawInfo() {
    nw4r::math::MTX34 mtx;
    PSMTXIdentity(mtx);

    mDrawInfo.mViewMtx = mtx;
    mDrawInfo.SetMultipleViewMtxOnDraw(true);
    mDrawInfo.mViewRect = mLayout->GetLayoutRect();
    mDrawInfo.SetLocationAdjust(true);
}

void LayoutManager::initPaneInfo() {
    mPaneCount = countPanes(mLayout->mpRootPane);
    mPaneInfoList = new LayoutPaneInfo[mPaneCount];

    u32 rIndex = 0;
    initPaneInfoRecursive(rIndex, mLayout->mpRootPane);
}

void LayoutManager::initPaneInfoRecursive(u32& rIndex, nw4r::lyt::Pane* pPane) {
    nw4r::lyt::PaneList& rPaneList = pPane->mChildList;

    mPaneInfoList[rIndex].mName = pPane->mName;
    mPaneInfoList[rIndex].mPaneCtrl = nullptr;
    mPaneInfoList[rIndex].mGroupCtrlList = nullptr;
    mPaneInfoList[rIndex].mMtxRef = nullptr;
    mPaneInfoList[rIndex].mPane = pPane;

    u32 startIndex = rIndex++;

    for (nw4r::lyt::PaneList::Iterator it = rPaneList.GetBeginIter(); it != rPaneList.GetEndIter(); ++it) {
        initPaneInfoRecursive(rIndex, &*it);  // TODO * operator being inlined
    }

    mPaneInfoList[startIndex].mSubtreeSize = rIndex - startIndex;
}

// TODO: instruction swap
u32 LayoutManager::countPanes(nw4r::lyt::Pane* pPane) {
    u32 count = 1;

    nw4r::lyt::PaneList& rPaneList = pPane->mChildList;
    for (nw4r::lyt::PaneList::Iterator it = rPaneList.GetBeginIter(); it != rPaneList.GetEndIter(); ++it) {
        count += countPanes(&*it);
    }

    return count;
}

void LayoutManager::initGroupCtrlList() {
    mGroupCtrlCount = mLayout->mpGroupContainer->mGroupList.GetSize();
    mGroupCtrlList = new LayoutGroupCtrl*[mGroupCtrlCount];

    for (u32 i = 0; i < mGroupCtrlCount; i++) {
        mGroupCtrlList[i] = nullptr;
    }
}

void LayoutManager::initTextBoxRecursive(nw4r::lyt::Pane* pPane, nw4r::lyt::Pane* pUserDataPane, const char* pLayoutName, u32 textBoxBufferLength) {
    nw4r::lyt::TextBox* pTextBox;

    const nw4r::ut::detail::RuntimeTypeInfo* pTextBoxRuntimeInfo = &nw4r::lyt::TextBox::typeInfo;
    if (pPane && pPane->GetRuntimeTypeInfo()->IsDerivedFrom(pTextBoxRuntimeInfo)) {
        pTextBox = static_cast< nw4r::lyt::TextBox* >(pPane);
    } else {
        pTextBox = nullptr;
    }

    char userData[9];
    LayoutCoreUtil::getPaneUserData(pPane, userData);

    if (!MR::isEqualString("", userData)) {
        pUserDataPane = pPane;
    }

    if (pTextBox != nullptr) {
        if (pUserDataPane != nullptr) {
            char messageID[0x100];
            LayoutCoreUtil::getPaneUserData(pUserDataPane, userData);
            MR::getLayoutMessageID(messageID, pLayoutName, userData);
            LayoutCoreUtil::initTextBoxPane(pTextBox, messageID, 0x100);
        } else {
            LayoutCoreUtil::initTextBoxPane(pTextBox, nullptr, textBoxBufferLength);
        }
    }

    nw4r::lyt::PaneList& rPaneList = pPane->mChildList;
    for (nw4r::lyt::PaneList::Iterator it = rPaneList.GetBeginIter(); it != rPaneList.GetEndIter(); ++it) {
        initTextBoxRecursive(&*it, pUserDataPane, pLayoutName, textBoxBufferLength);
    }
}

void LayoutManager::replaceIndDummyTexture() {
    if (!mLayoutHolder->isExistResOther("IndDummy.tpl")) {
        return;
    }

    JUTTexture screenTex(MR::getScreenResTIMG(), static_cast< u8 >(0));

    mIndDummyTexMap = new nw4r::lyt::TexMap(screenTex.getTexObj());

    // ???
    void* pIndDummyResStart = mLayoutHolder->getResOther("IndDummy.tpl");
    void* pIndDummyResEnd = reinterpret_cast< void* >(-1);

    for (u32 i = 0; i < mLayoutHolder->getResOtherNum(); i++) {
        void* pRes = mLayoutHolder->getResOther(i);

        if (pIndDummyResStart < pRes && pIndDummyResEnd > pRes) {
            pIndDummyResEnd = pRes;
        }
    }

    for (u32 paneIdx = 0; paneIdx < mPaneCount; paneIdx++) {
        nw4r::lyt::Pane* pCurrPane = mPaneInfoList[paneIdx].mPane;
        nw4r::lyt::Picture* pPicPane;
        const nw4r::ut::detail::RuntimeTypeInfo* pPictureRuntimeInfo = &nw4r::lyt::Picture::typeInfo;

        if (pCurrPane && pCurrPane->GetRuntimeTypeInfo()->IsDerivedFrom(pPictureRuntimeInfo)) {
            pPicPane = static_cast<nw4r::lyt::Picture*>(pCurrPane);
        }
        else {
            pPicPane = nullptr;
        }

        if (pPicPane == nullptr) {
            continue;
        }

        nw4r::lyt::Material* pMaterial = pPicPane->GetMaterial();

        for (u8 texMapIdx = 0; texMapIdx < pMaterial->GetTextureNum(); texMapIdx++) {
            void* pImage = pMaterial->GetTexture(texMapIdx).mImage;

            if (pImage < pIndDummyResStart) {
                continue;
            }

            if (pImage >= pIndDummyResEnd) {
                continue;
            }

            pMaterial->SetTexture(texMapIdx, *mIndDummyTexMap);
        }
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
    mPaneInfoList[index].mPane->UnbindAnimationSelf(pAnim);
    mPaneInfoList[index].mPane->BindAnimation(pAnim, false);
    index++;
    while (index < pPaneCtrl->mPaneIndex + mPaneInfoList[pPaneCtrl->mPaneIndex].mSubtreeSize) {
        bindPaneCtrlAnimSub(index, pAnim);
    }
}

void LayoutManager::bindPaneCtrlAnimSub(u32& rIndex, nw4r::lyt::AnimTransform* pAnim) {
    if (mPaneInfoList[rIndex].mPaneCtrl != nullptr) {
        rIndex += mPaneInfoList[rIndex].mSubtreeSize;
        return;
    }
    mPaneInfoList[rIndex].mPane->UnbindAnimationSelf(pAnim);
    mPaneInfoList[rIndex].mPane->BindAnimation(pAnim, false);
    u32 first = rIndex++;
    while (rIndex < first + mPaneInfoList[first].mSubtreeSize) {
        bindPaneCtrlAnimSub(rIndex, pAnim);
    }
}

void LayoutManager::unbindPaneCtrlAnim(LayoutPaneCtrl* pPaneCtrl, nw4r::lyt::AnimTransform* pAnim) {
    u32 index = pPaneCtrl->mPaneIndex;
    mPaneInfoList[index].mPane->UnbindAnimationSelf(pAnim);
    index++;
    while (index < pPaneCtrl->mPaneIndex + mPaneInfoList[pPaneCtrl->mPaneIndex].mSubtreeSize) {
        unbindPaneCtrlAnimSub(index, pAnim);
    }
}

void LayoutManager::unbindPaneCtrlAnimSub(u32& rIndex, nw4r::lyt::AnimTransform* pAnim) {
    if (mPaneInfoList[rIndex].mPaneCtrl != nullptr) {
        rIndex += mPaneInfoList[rIndex].mSubtreeSize;
        return;
    }
    mPaneInfoList[rIndex].mPane->UnbindAnimationSelf(pAnim);
    u32 first = rIndex++;
    while (rIndex < first + mPaneInfoList[first].mSubtreeSize) {
        unbindPaneCtrlAnimSub(rIndex, pAnim);
    }
}
