#pragma once

#include "JSystem/JKernel/JKRDisposer.hpp"
#include <aurora/endian.hpp>
#include <memory>
#include <utility>
#include <cstring>
#include <revolution.h>

#define JMAP_VALUE_TYPE_LONG 0
#define JMAP_VALUE_TYPE_STRING 1
#define JMAP_VALUE_TYPE_FLOAT 2
#define JMAP_VALUE_TYPE_LONG_2 3
#define JMAP_VALUE_TYPE_SHORT 4
#define JMAP_VALUE_TYPE_BYTE 5
#define JMAP_VALUE_TYPE_STRING_PTR 6
#define JMAP_VALUE_TYPE_NULL 7

class JMapInfoIter;

struct JMapItem {
    /* 0x00 */ aurora::endian::BigEndian< u32 > mHash;
    /* 0x04 */ aurora::endian::BigEndian< u32 > mMask;
    /* 0x08 */ aurora::endian::BigEndian< u16 > mOffsData;
    /* 0x0A */ u8 mShift;
    /* 0x0B */ u8 mType;
};

struct JMapData {
    /* 0x00 */ aurora::endian::BigEndian< s32 > mNumEntries;
    /* 0x04 */ aurora::endian::BigEndian< s32 > mNumFields;
    /* 0x08 */ aurora::endian::BigEndian< s32 > mDataOffset;
    /* 0x0C */ aurora::endian::BigEndian< u32 > mEntrySize;
    /* 0x10 */ const JMapItem mItems[];
};

template < typename T >
inline bool compareValues(const T a, const T b) {
    return a == b;
}

template <>
inline bool compareValues< const char* >(const char* pA, const char* pB) {
    return strcmp(pA, pB) == 0;
}

inline const char* getEntryAddress(const JMapData* pData, s32 dataOffset, int entryIndex) {
    return reinterpret_cast< const char* >(pData) + dataOffset + entryIndex * pData->mEntrySize;
}

class JMapInfo : private JKRDisposer {
public:
    JMapInfo();
    ~JMapInfo() override;
    JMapInfo(const JMapInfo&) = default;
    JMapInfo& operator=(const JMapInfo&) = default;
    JMapInfo(JMapInfo&&) noexcept;
    JMapInfo& operator=(JMapInfo&&) noexcept;
    void releaseNativeResourceReferences() noexcept override;

    const void* getData() const { return mData; }
    const char* getEntryData(s32 index) const { return getEntryAddress(mData, mData->mDataOffset, index); }
    u32 getDataSize() const { return mData->mDataOffset + mData->mEntrySize * getNumEntries(); }

    inline bool operator==(const JMapInfo& rInfo) const {
        return mData == rInfo.mData;
    }

    inline bool dataExists() const {
        return !!mData;
    }

    inline int getNumEntries() const {
        return dataExists() ? mData->mNumEntries : 0;
    }

    inline int getNumFields() const {
        return dataExists() ? mData->mNumFields : 0;
    }

    bool attach(const void* pData);
    void setName(const char* pName);
    const char* getName() const;
    s32 searchItemInfo(const char* pKey) const;
    s32 getValueType(const char* pKey) const;
    bool getValueFast(int entryIndex, int itemIndex, const char** pValueOut) const;
    bool getValueFast(int entryIndex, int itemIndex, u32* pValueOut) const;
    bool getValueFast(int entryIndex, int itemIndex, s32* pValueOut) const;
    bool getValueFast(int entryIndex, int itemIndex, f32* pValueOut) const {
        const JMapItem* pItem = &mData->mItems[itemIndex];
        const char* pValue = getEntryAddress(mData, mData->mDataOffset, entryIndex) + pItem->mOffsData;
        *pValueOut = aurora::endian::read_big< f32 >(pValue);
        return true;
    }
    bool getValueFast(int entryIndex, int itemIndex, bool* pValueOut) const {
        const JMapItem* pItem = &mData->mItems[itemIndex];
        const char* pValue = getEntryAddress(mData, mData->mDataOffset, entryIndex) + pItem->mOffsData;
        *pValueOut = (aurora::endian::read_big< u32 >(pValue) & pItem->mMask) != 0;
        return true;
    }

    JMapInfoIter findElementBinary(const char* pKey, const char* pValue) const;

    template < typename T >
    const bool getValue(int entryIndex, const char* pKey, T* pValueOut) const;

    template < typename T >
    JMapInfoIter findElement(const char* pKey, T searchValue, int startIndex) const;

    inline JMapInfoIter begin() const;
    inline JMapInfoIter end() const;

    /* 0x00 */ const JMapData* mData;
    /* 0x04 */ const char* mName;

    // Keep a host archive borrow alive across original heap retirement.
    std::shared_ptr< const void > mResourceOwner;
};

template < typename T >
const bool JMapInfo::getValue(int entryIndex, const char* pKey, T* pValueOut) const {
    s32 itemIndex = searchItemInfo(pKey);
    if (itemIndex < 0) {
        return false;
    }

    return getValueFast(entryIndex, itemIndex, pValueOut);
}

class JMapInfoIter {
public:
    JMapInfoIter() : mInfo(), mIndex(-1) {
    }

    JMapInfoIter(const JMapInfo* pInfo, s32 index) : mInfo(pInfo), mIndex(index) {
    }

    bool operator==(const JMapInfoIter& rIter) const {
        return mIndex == rIter.mIndex && mInfo != nullptr && rIter.mInfo != nullptr && *mInfo == *rIter.mInfo;
    }

    bool operator!=(const JMapInfoIter& rIter) const {
        return !(*this == rIter);
    }

    bool isValid() const {
        return mInfo != nullptr && mIndex >= 0 && mIndex < mInfo->getNumEntries();
    }

    template < typename T >
    bool getValue(const char* pKey, T* pValueOut) const {
        return mInfo->getValue(mIndex, pKey, pValueOut);
    }

    /* 0x00 */ const JMapInfo* mInfo;
    /* 0x04 */ s32 mIndex;
};

template < typename T >
JMapInfoIter JMapInfo::findElement(const char* pKey, T searchValue, int startIndex) const {
    int entryIndex = startIndex;
    T value;

    while (entryIndex < getNumEntries()) {
        getValue< T >(entryIndex, pKey, &value);
        if (compareValues< T >(value, searchValue)) {
            return JMapInfoIter(this, entryIndex);
        }

        entryIndex++;
    }

    return end();
}

JMapInfoIter JMapInfo::begin() const {
    return JMapInfoIter(this, 0);
}

JMapInfoIter JMapInfo::end() const {
    return JMapInfoIter(this, getNumEntries());
}

namespace MR {
    JMapInfoIter findJMapInfoElementNoCase(const JMapInfo* pInfo, const char* pKey, const char* pValue, int startIndex);
};  // namespace MR
