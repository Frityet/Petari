#include "Game/Player/MatrixControl.hpp"

MatrixControl::MatrixControl(const char* pName, MatrixMap* pMap, MatrixSelectList* pSelectList, long selectCount)
    : NameObj(pName), mMap(pMap), mSelectList(pSelectList), mHashTable(nullptr), mSelectCount(selectCount), mUsePackedValues(true), mDefaultBit(0) {
    for (long i = 0; i < mSelectCount; i++) {
        if (mSelectList[i + 1].mValueCount > 2) {
            mUsePackedValues = false;
            break;
        }
    }

    u32 entryCount = 0;
    for (; mMap[entryCount].mName[0] != '\0'; entryCount++) {
        if (!mUsePackedValues) {
            mMap[entryCount].mValues = new u8[8];
            for (u32 i = 0; i < 8; i++) {
                mMap[entryCount].mValues[i] = (mMap[entryCount].mBits >> (4 * (7 - i))) & 0xF;
            }
        }
    }

    mHashTable = new HashSortTable(entryCount);
    for (u32 i = 0; i < entryCount; i++) {
        mHashTable->add(mMap[i].mName, i, false);
    }
    mHashTable->sort();
}

MatrixControl::~MatrixControl() {}

u8 MatrixControl::getValue(const char* pName, u8 idx) const {
    u32 tableIdx;
    mHashTable->search(pName, &tableIdx);
    return mMap[tableIdx].mValues[idx];
}

bool MatrixControl::getValueOrNone(const char* pName, u8 idx, u8* pOut) const {
    u32 tableIdx;
    if (!mHashTable->search(pName, &tableIdx)) {
        return false;
    }

    if (pOut != nullptr) {
        *pOut = mMap[tableIdx].mValues[idx];
    }

    return true;
}

bool MatrixControl::getBit(const char* pName, u8 idx) const {
    u32 tableIdx;
    mHashTable->search(pName, &tableIdx);
    return (mMap[tableIdx].mBits & (1 << (31 - idx))) != 0;
}

bool MatrixControl::isExist(const char* pName) const {
    u32 tableIdx;
    return mHashTable->search(pName, &tableIdx);
}

bool MatrixControl::getBitOrNone(const char* pName, u8 idx) const {
    u32 tableIdx;
    if (!mHashTable->search(pName, &tableIdx)) {
        return mDefaultBit;
    }

    return (mMap[tableIdx].mBits & (1 << (31 - idx))) != 0;
}

MatrixValueGetter::MatrixValueGetter(const char* pName, MatrixValueTable* pTable) : NameObj(pName), mTable(pTable), mHashTable(nullptr) {
    u32 entryCount = 0;
    for (; mTable[entryCount].mName[0] != '\0'; entryCount++) {
    }

    mHashTable = new HashSortTable(entryCount);
    for (u32 i = 0; i < entryCount; i++) {
        mHashTable->add(mTable[i].mName, i, false);
        mHashTable->sort();
    }
}

MatrixValueGetter::~MatrixValueGetter() {}

bool MatrixValueGetter::getValue(const char* pName, f32* pOut) const {
    u32 tableIdx;
    if (!mHashTable->search(pName, &tableIdx)) {
        return false;
    }

    if (pOut != nullptr) {
        *pOut = mTable[tableIdx].mValue;
    }

    return true;
}
