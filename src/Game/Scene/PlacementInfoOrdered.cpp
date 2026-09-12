#include "Game/Scene/PlacementInfoOrdered.hpp"
#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/Map/PlanetMapCreator.hpp"
#include "Game/NameObj/ModelChangableObjFactory.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/StringUtil.hpp"

namespace {
    CreationFuncPtr getCreator(const PlacementInfoOrdered::Identifier& rIdentifier) NO_INLINE {
        if (rIdentifier.mModelNo != -1) {
            return MR::getModelChangableObjCreator(rIdentifier.mName);
        }
        return NameObjFactory::getCreator(rIdentifier.mName);
    }

    inline bool isBefore(const PlacementInfoOrdered::SameIdSet* pSet, const PlacementInfoOrdered::SameIdSet* pOther) {
        if (pSet->mPriority == pOther->mPriority) {
            return pOther->mList.mCount < pSet->mList.mCount;
        }
        return pSet->mPriority < pOther->mPriority;
    }
};  // namespace

PlacementInfoOrdered::PlacementInfoOrdered(int count)
    : mIndexArray(nullptr), _4(0), mSetArray(nullptr), mIdentiferArray(nullptr), mCount(count) {
    mIndexArray = new Index[count];
    mSetArray = new SameIdSet[count];
    mIdentiferArray = new SameIdSet*[count];
    MR::zeroMemory(mIdentiferArray, sizeof(*mIdentiferArray) * count);
}

void PlacementInfoOrdered::sort() {
    s32 count = getUsedArrayNum();
    for (s32 i = 0; i < count; i++) {
        SameIdSet* pSet = mIdentiferArray[i];
        if (NameObjFactory::isPlayerArchiveLoaderObj(pSet->mName)) {
            pSet->mPriority = 0;
        } else {
            const JMapInfoIter& rIter = static_cast< Index* >(pSet->mList.mHead->mValue)->mInfoIter;
            bool readFromDVD;
            if (pSet->mModelNo != -1) {
                readFromDVD = MR::isReadResourceFromDVDAtModelChangableObj(pSet->mName, pSet->mModelNo);
            } else {
                readFromDVD = NameObjFactory::isReadResourceFromDVD(pSet->mName, rIter);
            }
            pSet->mPriority = readFromDVD ? 2 : 1;
        }
    }

    s32 gap = 13;
    while (gap < count) {
        gap = gap * 3 + 1;
    }
    for (gap /= 9; gap > 0; gap /= 3) {
        for (s32 i = gap; i < count; i++) {
            SameIdSet* pSet = mIdentiferArray[i];
            s32 j = i - gap;
            while (j >= 0 && ::isBefore(pSet, mIdentiferArray[j])) {
                mIdentiferArray[j + gap] = mIdentiferArray[j];
                j -= gap;
            }
            mIdentiferArray[j + gap] = pSet;
        }
    }
}

void PlacementInfoOrdered::requestFileLoad() {
    s32 count = getUsedArrayNum();
    for (s32 i = 0; i < count; i++) {
        SameIdSet* pSet = mIdentiferArray[i];
        if (::getCreator(*pSet) != nullptr) {
            if (pSet->mModelNo != -1) {
                MR::requestMountModelChangableObjArchives(pSet->mName, pSet->mModelNo);
            } else {
                for (MR::BothDirPtrLink* pLink = pSet->mList.mHead; pLink != nullptr; pLink = pLink->mNextLink) {
                    NameObjFactory::requestMountObjectArchives(pSet->mName, static_cast< Index* >(pLink->mValue)->mInfoIter);
                }
            }
        }
    }
}

void PlacementInfoOrdered::initPlacement() {
    MR::startInitLiveActorSystemInfo();
    s32 count = getUsedArrayNum();
    for (s32 i = 0; i < count; i++) {
        SameIdSet* pSet = mIdentiferArray[i];
        CreationFuncPtr creator = ::getCreator(*pSet);
        if (creator != nullptr) {
            const char* pName = MR::getJapaneseObjectName(pSet->mName);
            for (MR::BothDirPtrLink* pLink = pSet->mList.mHead; pLink != nullptr; pLink = pLink->mNextLink) {
                const JMapInfoIter& rIter = static_cast< Index* >(pLink->mValue)->mInfoIter;
                MR::setCurrentPlacementZoneId(MR::getPlacedZoneId(rIter));
                NameObj* pObj = creator(pName);
                MR::initLiveActorSystemInfo(rIter);
                pObj->init(rIter);
                MR::initLiveActorSystemInfo(rIter);
                MR::clearCurrentPlacementZoneId();
            }
        }
    }
}

void PlacementInfoOrdered::insert(const Identifier& rIdentifier, const JMapInfoIter& rIter) {
    SameIdSet* pSet = find(rIdentifier);
    if (pSet == nullptr) {
        pSet = createSameIdSet(rIdentifier);
    }
    Index* pIndex = createIndex(rIter);
    pSet->mList.append(&pIndex->mLink);
}

u32 PlacementInfoOrdered::getUsedArrayNum() const {
    u32 count = 0;
    for (s32 i = 0; i < mCount && mIdentiferArray[i] != nullptr; i++) {
        count++;
    }
    return count;
}

PlacementInfoOrdered::SameIdSet* PlacementInfoOrdered::find(const Identifier& rIdentifier) const {
    s32 count = getUsedArrayNum();
    for (s32 i = 0; i < count; i++) {
        SameIdSet* pSet = mIdentiferArray[i];
        if (pSet != nullptr) {
            bool equal = false;
            if (MR::isEqualString(pSet->mName, rIdentifier.mName) && pSet->mModelNo == rIdentifier.mModelNo) {
                equal = true;
            }
            if (equal) {
                return pSet;
            }
        }
    }
    return nullptr;
}

PlacementInfoOrdered::SameIdSet* PlacementInfoOrdered::createSameIdSet(const Identifier& rIdentifier) {
    u32 index = getUsedArrayNum();
    SameIdSet* pSet = &mSetArray[index];
    mIdentiferArray[index] = pSet;
    static_cast< Identifier& >(*pSet) = rIdentifier;
    return pSet;
}

PlacementInfoOrdered::Index* PlacementInfoOrdered::createIndex(const JMapInfoIter& rIter) {
    Index* pIndex = &mIndexArray[_4++];
    pIndex->mInfoIter = rIter;
    return pIndex;
}

PlacementInfoOrdered::Index::Index() : mLink(this), mInfoIter() {
}

PlacementInfoOrdered::Index::~Index() {
}

PlacementInfoOrdered::SameIdSet::SameIdSet() : Identifier(nullptr, -1), mList(true) {
}

PlacementInfoOrdered::SameIdSet::~SameIdSet() {
}

void PlacementInfoOrdered::attach(const JMapInfo* pInfo, PlacementInfoOrdered* pDeferred) {
    s32 count = pInfo->getNumEntries();
    for (s32 i = 0; i < count; i++) {
        JMapInfoIter iter(pInfo, i);
        const char* pName = "";
        MR::getObjectName(&pName, iter);
        s32 modelNo = -1;
        MR::getJMapInfoShapeIdWithInit(iter, &modelNo);
        Identifier identifier(pName, modelNo);
        if (pDeferred != nullptr && modelNo == -1 && PlanetMapCreatorFunction::isLoadArchiveAfterScenarioSelected(pName)) {
            pDeferred->insert(identifier, iter);
        } else {
            insert(identifier, iter);
        }
    }
}
