#include "Game/Scene/PlacementInfoOrdered.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/Map/PlanetMapCreator.hpp"
#include "Game/NameObj/ModelChangableObjFactory.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/StringUtil.hpp"

namespace {
    CreationFuncPtr getCreator(const PlacementInfoOrdered::Identifier& rIdentifier) NO_INLINE {
        if (rIdentifier.mShapeId != -1) {
            return MR::getModelChangableObjCreator(rIdentifier.mName);
        }
        return NameObjFactory::getCreator(rIdentifier.mName);
    }

    bool isLess(const PlacementInfoOrdered::SameIdSet* pLhs, const PlacementInfoOrdered::SameIdSet* pRhs) {
        if (pLhs->mOrder == pRhs->mOrder) {
            return pRhs->mList.mCount < pLhs->mList.mCount;
        }
        return pLhs->mOrder < pRhs->mOrder;
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
    s32 count = getUsedArrayNum();
    for (s32 i = 0; i < count; i++) {
        SameIdSet* set = mIdentiferArray[i];
        if (NameObjFactory::isPlayerArchiveLoaderObj(set->mName)) {
            set->mOrder = 0;
        } else {
            const JMapInfoIter& iter = static_cast< Index* >(set->mList.mHead->mValue)->mInfoIter;
            s32 readFromDVD;
            if (set->mShapeId != -1) {
                readFromDVD = MR::isReadResourceFromDVDAtModelChangableObj(set->mName, set->mShapeId);
            } else {
                readFromDVD = NameObjFactory::isReadResourceFromDVD(set->mName, iter);
            }
            set->mOrder = (readFromDVD ? 1 : 0) + 1;
        }
    }

    s32 gap = 13;
    while (gap < count) {
        gap = gap * 3 + 1;
    }
    gap /= 9;
    while (gap > 0) {
        for (s32 i = gap; i < count; i++) {
            SameIdSet* value = mIdentiferArray[i];
            s32 j = i - gap;
            while (j >= 0 && ::isLess(value, mIdentiferArray[j])) {
                mIdentiferArray[j + gap] = mIdentiferArray[j];
                j -= gap;
            }
            mIdentiferArray[j + gap] = value;
        }
        gap /= 3;
    }
}

void PlacementInfoOrdered::requestFileLoad() {
    s32 count = getUsedArrayNum();
    for (s32 i = 0; i < count; i++) {
        SameIdSet* set = mIdentiferArray[i];
        if (!::getCreator(*set)) {
            continue;
        }
        if (set->mShapeId != -1) {
            MR::requestMountModelChangableObjArchives(set->mName, set->mShapeId);
        } else {
            for (MR::BothDirPtrLink* link = set->mList.mHead; link != nullptr; link = link->mNextLink) {
                NameObjFactory::requestMountObjectArchives(set->mName, static_cast< Index* >(link->mValue)->mInfoIter);
            }
        }
    }
}

void PlacementInfoOrdered::initPlacement() {
    MR::startInitLiveActorSystemInfo();
    s32 count = getUsedArrayNum();
    for (s32 i = 0; i < count; i++) {
        SameIdSet* set = mIdentiferArray[i];
        CreationFuncPtr creator = ::getCreator(*set);
        if (!creator) {
            continue;
        }
        const char* name = MR::getJapaneseObjectName(set->mName);
        for (MR::BothDirPtrLink* link = set->mList.mHead; link != nullptr; link = link->mNextLink) {
            const JMapInfoIter& iter = static_cast< Index* >(link->mValue)->mInfoIter;
            MR::setCurrentPlacementZoneId(MR::getPlacedZoneId(iter));
            NameObj* object = creator(name);
            MR::initLiveActorSystemInfo(iter);
            object->init(iter);
            MR::initLiveActorSystemInfo(iter);
            MR::clearCurrentPlacementZoneId();
        }
    }
}

void PlacementInfoOrdered::insert(const Identifier& rIdentifier, const JMapInfoIter& rIter) {
    SameIdSet* set = find(rIdentifier);
    if (!set) {
        set = createSameIdSet(rIdentifier);
    }
    set->mList.append(createIndex(rIter));
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
        SameIdSet* set = mIdentiferArray[i];
        if (set != nullptr) {
            bool same = MR::isEqualString(set->mName, rIdentifier.mName) && set->mShapeId == rIdentifier.mShapeId;
            if (same) {
                return set;
            }
        }
    }
    return nullptr;
}

PlacementInfoOrdered::SameIdSet* PlacementInfoOrdered::createSameIdSet(const Identifier& rIdentifier) {
    u32 index = getUsedArrayNum();
    SameIdSet* set = &mSetArray[index];
    mIdentiferArray[index] = set;
    *static_cast< Identifier* >(set) = rIdentifier;
    return set;
}

PlacementInfoOrdered::Index* PlacementInfoOrdered::createIndex(const JMapInfoIter& rIter) {
    Index* index = &mIndexArray[_4];
    _4++;
    index->mInfoIter = rIter;
    return index;
}

PlacementInfoOrdered::Index::Index() : MR::BothDirPtrLink(this), mInfoIter() {
}

PlacementInfoOrdered::Index::~Index() {
}

PlacementInfoOrdered::SameIdSet::SameIdSet() : Identifier(), mList(true) {
}

PlacementInfoOrdered::SameIdSet::~SameIdSet() {
}

void PlacementInfoOrdered::attach(const JMapInfo* pInfo, PlacementInfoOrdered* pAfterScenario) {
    s32 count = pInfo->getNumEntries();
    for (s32 i = 0; i < count; i++) {
        JMapInfoIter iter(pInfo, i);
        const char* name = "";
        MR::getObjectName(&name, iter);
        s32 shapeId = -1;
        MR::getJMapInfoShapeIdWithInit(iter, &shapeId);
        Identifier identifier;
        identifier.mName = name;
        identifier.mShapeId = shapeId;
        if (pAfterScenario != nullptr && shapeId == -1 && PlanetMapCreatorFunction::isLoadArchiveAfterScenarioSelected(name)) {
            pAfterScenario->insert(identifier, iter);
        } else {
            insert(identifier, iter);
        }
    }
}
