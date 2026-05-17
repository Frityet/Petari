#include "Game/Scene/PlacementInfoOrdered.hpp"
#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/Map/PlanetMapCreator.hpp"
#include "Game/NameObj/ModelChangableObjFactory.hpp"
#include "Game/NameObj/NameObjFactory.hpp"

namespace {
    CreationFuncPtr getCreator(const PlacementInfoOrdered::Identifier& rIdentifier) {
        if (rIdentifier.mShapeId != -1) {
            return MR::getModelChangableObjCreator(rIdentifier.mName);
        }

        return NameObjFactory::getCreator(rIdentifier.mName);
    }

    bool isLessPlacementInfo(const PlacementInfoOrdered::SameIdSet* pLeft, const PlacementInfoOrdered::SameIdSet* pRight) {
        if (pLeft->mOrder != pRight->mOrder) {
            return pLeft->mOrder < pRight->mOrder;
        }

        return pLeft->mList.mCount > pRight->mList.mCount;
    }
};  // namespace

PlacementInfoOrdered::PlacementInfoOrdered(int count) {
    mIndexArray = nullptr;
    _4 = 0;
    mSetArray = nullptr;
    mIdentiferArray = nullptr;
    mCount = count;
    mIndexArray = new Index[count];
    mSetArray = new SameIdSet[count];
    mIdentiferArray = new SameIdSet*[count];
    MR::zeroMemory(mIdentiferArray, count * sizeof(SameIdSet*));
}

void PlacementInfoOrdered::sort() {
    u32 usedNum = getUsedArrayNum();

    for (u32 i = 0; i < usedNum; i++) {
        SameIdSet* pSet = mIdentiferArray[i];

        if (NameObjFactory::isPlayerArchiveLoaderObj(pSet->mName)) {
            pSet->mOrder = 0;
        } else {
            bool isReadResourceFromDVD;

            if (pSet->mShapeId != -1) {
                isReadResourceFromDVD = MR::isReadResourceFromDVDAtModelChangableObj(pSet->mName, pSet->mShapeId);
            } else {
                Index* pFirstIndex = static_cast< Index* >(pSet->mList.mHead);
                isReadResourceFromDVD = NameObjFactory::isReadResourceFromDVD(pSet->mName, pFirstIndex->mInfoIter);
            }

            pSet->mOrder = isReadResourceFromDVD ? 2 : 1;
        }
    }

    s32 gap = 13;

    while (gap < static_cast< s32 >(usedNum)) {
        gap = gap * 3 + 1;
    }

    for (gap /= 9; gap > 0; gap /= 3) {
        for (s32 i = gap; i < static_cast< s32 >(usedNum); i++) {
            SameIdSet* pSet = mIdentiferArray[i];
            s32 j = i - gap;

            while (j >= 0 && isLessPlacementInfo(pSet, mIdentiferArray[j])) {
                mIdentiferArray[j + gap] = mIdentiferArray[j];
                j -= gap;
            }

            mIdentiferArray[j + gap] = pSet;
        }
    }
}

void PlacementInfoOrdered::requestFileLoad() {
    u32 usedNum = getUsedArrayNum();

    for (u32 i = 0; i < usedNum; i++) {
        SameIdSet* pSet = mIdentiferArray[i];
        Identifier identifier = { pSet->mName, pSet->mShapeId };

        if (getCreator(identifier) != nullptr) {
            if (pSet->mShapeId != -1) {
                MR::requestMountModelChangableObjArchives(pSet->mName, pSet->mShapeId);
            } else {
                for (Index* pIndex = static_cast< Index* >(pSet->mList.mHead); pIndex != nullptr;
                     pIndex = static_cast< Index* >(pIndex->mNextLink)) {
                    NameObjFactory::requestMountObjectArchives(pSet->mName, pIndex->mInfoIter);
                }
            }
        }
    }
}

void PlacementInfoOrdered::initPlacement() {
    MR::startInitLiveActorSystemInfo();
    u32 usedNum = getUsedArrayNum();

    for (u32 i = 0; i < usedNum; i++) {
        SameIdSet* pSet = mIdentiferArray[i];
        Identifier identifier = { pSet->mName, pSet->mShapeId };
        CreationFuncPtr creator = getCreator(identifier);

        if (creator != nullptr) {
            const char* pJapaneseName = MR::getJapaneseObjectName(pSet->mName);

            for (Index* pIndex = static_cast< Index* >(pSet->mList.mHead); pIndex != nullptr;
                 pIndex = static_cast< Index* >(pIndex->mNextLink)) {
                const JMapInfoIter& rIter = pIndex->mInfoIter;

                MR::setCurrentPlacementZoneId(MR::getPlacedZoneId(rIter));

                NameObj* pObj = creator(pJapaneseName);
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

    pSet->mList.append(createIndex(rIter));
}

u32 PlacementInfoOrdered::getUsedArrayNum() const {
    u32 usedNum = 0;

    while (usedNum < static_cast< u32 >(mCount) && mIdentiferArray[usedNum] != nullptr) {
        usedNum++;
    }

    return usedNum;
}

PlacementInfoOrdered::SameIdSet* PlacementInfoOrdered::find(const Identifier& rIdentifier) const {
    u32 usedNum = getUsedArrayNum();

    for (u32 i = 0; i < usedNum; i++) {
        SameIdSet* pSet = mIdentiferArray[i];

        if (pSet != nullptr && MR::isEqualString(pSet->mName, rIdentifier.mName) && pSet->mShapeId == rIdentifier.mShapeId) {
            return pSet;
        }
    }

    return nullptr;
}

PlacementInfoOrdered::SameIdSet* PlacementInfoOrdered::createSameIdSet(const Identifier& rIdentifier) {
    u32 usedNum = getUsedArrayNum();
    SameIdSet* pSet = &mSetArray[usedNum];

    mIdentiferArray[usedNum] = pSet;
    pSet->mName = rIdentifier.mName;
    pSet->mShapeId = rIdentifier.mShapeId;

    return pSet;
}

PlacementInfoOrdered::Index* PlacementInfoOrdered::createIndex(const JMapInfoIter& rIter) {
    Index* pIndex = &mIndexArray[_4++];

    pIndex->mInfoIter = rIter;

    return pIndex;
}

PlacementInfoOrdered::Index::Index() : MR::BothDirPtrLink(this) {
    mInfoIter.mInfo = nullptr;
    mInfoIter.mIndex = -1;
}

PlacementInfoOrdered::Index::~Index() {}

PlacementInfoOrdered::SameIdSet::SameIdSet() : mList(false) {
    mName = nullptr;
    mShapeId = -1;
    mList.initiate();
}

PlacementInfoOrdered::SameIdSet::~SameIdSet() {}

void PlacementInfoOrdered::attach(const JMapInfo* pInfo, PlacementInfoOrdered* pAfterScenarioOrdered) {
    s32 entryNum = pInfo->getNumEntries();

    for (s32 i = 0; i < entryNum; i++) {
        JMapInfoIter iter(pInfo, i);
        const char* pObjectName = nullptr;
        s32 shapeId = -1;

        MR::getObjectName(&pObjectName, iter);
        MR::getJMapInfoShapeIdWithInit(iter, &shapeId);

        Identifier identifier = { pObjectName, shapeId };

        if (pAfterScenarioOrdered != nullptr && shapeId == -1 && PlanetMapCreatorFunction::isLoadArchiveAfterScenarioSelected(pObjectName)) {
            pAfterScenarioOrdered->insert(identifier, iter);
        } else {
            insert(identifier, iter);
        }
    }
}
