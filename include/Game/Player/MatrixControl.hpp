#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/HashUtil.hpp"

struct MatrixMap {
    const char* mName;  // 0x0
    u32 mBits;          // 0x4
    u8* mValues;        // 0x8
};

struct MatrixSelectList {
    u8 mValueCount;  // 0x0
    u8 _1[0x83];
};

struct MatrixValueTable {
    const char* mName;  // 0x0
    f32 mValue;         // 0x4
};

class MatrixControl : public NameObj {
public:
    MatrixControl(const char*, MatrixMap*, MatrixSelectList*, long);

    virtual ~MatrixControl();

    u8 getValue(const char*, u8) const;
    bool getValueOrNone(const char*, u8, u8*) const;
    bool getBit(const char*, u8) const;
    bool isExist(const char*) const;
    bool getBitOrNone(const char*, u8) const;

    MatrixMap* mMap;               // 0xC
    MatrixSelectList* mSelectList; // 0x10
    HashSortTable* mHashTable;     // 0x14
    long mSelectCount;             // 0x18
    u8 mUsePackedValues;           // 0x1C
    u8 mDefaultBit;                // 0x1D
};

class MatrixValueGetter : public NameObj {
public:
    MatrixValueGetter(const char*, MatrixValueTable*);

    virtual ~MatrixValueGetter();

    bool getValue(const char*, f32*) const;

    MatrixValueTable* mTable;  // 0xC
    HashSortTable* mHashTable; // 0x10
};
