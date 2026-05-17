#pragma once

#include "Game/Util.hpp"

typedef NameObj* (*CreationFuncPtr)(const char*);

class PlacementInfoOrdered {
public:
    class Index : public MR::BothDirPtrLink {
    public:
        Index();
        ~Index();

        /* 0x10 */ JMapInfoIter mInfoIter;
    };

    class SameIdSet {
    public:
        SameIdSet();
        ~SameIdSet();

        /* 0x00 */ const char* mName;
        /* 0x04 */ s32 mShapeId;
        /* 0x08 */ s32 mOrder;
        /* 0x0C */ MR::BothDirList< Index > mList;
    };

    class Identifier {
    public:
        /* 0x00 */ const char* mName;
        /* 0x04 */ s32 mShapeId;
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
    SameIdSet** mIdentiferArray;   // 0xC
    int mCount;                    // 0x10
};
