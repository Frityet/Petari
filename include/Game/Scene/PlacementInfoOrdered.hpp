#pragma once

#include "Game/Util/BothDirList.hpp"
#include "Game/Util/JMapInfo.hpp"

class NameObj;

typedef NameObj* (*CreationFuncPtr)(const char*);

class PlacementInfoOrdered {
public:
    class Identifier {
    public:
        Identifier(const char* pName, s32 modelNo) : mName(pName), mModelNo(modelNo) {
        }

        const char* mName;  // 0x0
        s32 mModelNo;       // 0x4
    };

    class Index {
    public:
        Index();
        ~Index();

        MR::BothDirPtrLink mLink;  // 0x0
        JMapInfoIter mInfoIter;   // 0x10
    };

    class SameIdSet : public Identifier {
    public:
        SameIdSet();
        ~SameIdSet();

        s32 mPriority;                     // 0x8
        MR::BothDirList< Index > mList;     // 0xC
    };

    PlacementInfoOrdered(int);

    void sort();
    void requestFileLoad();
    void initPlacement();
    void insert(const Identifier&, const JMapInfoIter&);
    u32 getUsedArrayNum() const;
    SameIdSet* find(const Identifier&) const;
    SameIdSet* createSameIdSet(const Identifier&);
    Index* createIndex(const JMapInfoIter&);

    void attach(const JMapInfo*, PlacementInfoOrdered*);

    Index* mIndexArray;  // 0x0
    u32 _4;
    SameIdSet* mSetArray;          // 0x8
    SameIdSet** mIdentiferArray;  // 0xC
    int mCount;                    // 0x10
};
