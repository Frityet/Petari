#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <revolution.h>

#define JMAP_VALUE_TYPE_LONG 0
#define JMAP_VALUE_TYPE_STRING 1
#define JMAP_VALUE_TYPE_FLOAT 2
#define JMAP_VALUE_TYPE_LONG_2 3
#define JMAP_VALUE_TYPE_SHORT 4
#define JMAP_VALUE_TYPE_BYTE 5
#define JMAP_VALUE_TYPE_STRING_PTR 6
#define JMAP_VALUE_TYPE_NULL 7

namespace smgpc::compat {
    class BcsvTable;
}

template < typename T >
inline bool compareValues(const T a, const T b) {
    return a == b;
}

template <>
inline bool compareValues< const char* >(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;
}

class JMapInfoIter;

class JMapInfo {
public:
    JMapInfo() = default;
    explicit JMapInfo(smgpc::compat::BcsvTable table);

    [[nodiscard]] bool operator==(const JMapInfo& rInfo) const {
        return mTable == rInfo.mTable;
    }

    [[nodiscard]] static JMapInfo from_bcsv(std::span< const std::uint8_t > data);

    [[nodiscard]] bool dataExists() const;
    [[nodiscard]] int getNumEntries() const;
    [[nodiscard]] int getNumFields() const;
    bool attach(const void* pData);
    void setName(const char* pName);
    [[nodiscard]] const char* getName() const;
    [[nodiscard]] s32 searchItemInfo(const char* pKey) const;
    [[nodiscard]] s32 getValueType(const char* pKey) const;
    [[nodiscard]] bool getValueFast(int entryIndex, int itemIndex, const char** pValueOut) const;
    [[nodiscard]] bool getValueFast(int entryIndex, int itemIndex, u32* pValueOut) const;
    [[nodiscard]] bool getValueFast(int entryIndex, int itemIndex, s32* pValueOut) const;
    [[nodiscard]] bool getValueFast(int entryIndex, int itemIndex, f32* pValueOut) const;
    [[nodiscard]] bool getValueFast(int entryIndex, int itemIndex, bool* pValueOut) const;
    [[nodiscard]] JMapInfoIter findElementBinary(const char* pKey, const char* pValue) const;

    template < typename T >
    [[nodiscard]] bool getValue(int entryIndex, const char* pKey, T* pValueOut) const {
        const auto item_index = searchItemInfo(pKey);
        if (item_index < 0) {
            return false;
        }

        return getValueFast(entryIndex, item_index, pValueOut);
    }

    template < typename T >
    [[nodiscard]] JMapInfoIter findElement(const char* pKey, T searchValue, int startIndex) const;

    [[nodiscard]] JMapInfoIter end() const;

private:
    [[nodiscard]] bool getSignedValue(int entryIndex, const char* pKey, s32* pValueOut) const;
    [[nodiscard]] bool getUnsignedValue(int entryIndex, const char* pKey, u32* pValueOut) const;
    [[nodiscard]] bool getFloatValue(int entryIndex, const char* pKey, f32* pValueOut) const;
    [[nodiscard]] bool getStringValue(int entryIndex, const char* pKey, const char** pValueOut) const;
    [[nodiscard]] bool getSignedValueByHash(int entryIndex, std::uint32_t hash, s32* pValueOut) const;
    [[nodiscard]] bool getUnsignedValueByHash(int entryIndex, std::uint32_t hash, u32* pValueOut) const;
    [[nodiscard]] bool getFloatValueByHash(int entryIndex, std::uint32_t hash, f32* pValueOut) const;
    [[nodiscard]] bool getStringValueByHash(int entryIndex, std::uint32_t hash, const char** pValueOut) const;

    std::shared_ptr< smgpc::compat::BcsvTable > mTable;
    std::string mName;
    mutable std::map< std::pair< int, std::uint32_t >, std::string > mStringCache;
};

class JMapInfoIter {
public:
    JMapInfoIter() = default;

    JMapInfoIter(const JMapInfo* pInfo, s32 index) : mInfo(pInfo), mIndex(index) {
    }

    [[nodiscard]] bool isValid() const {
        return mInfo != nullptr && mIndex >= 0 && mIndex < mInfo->getNumEntries();
    }

    template < typename T >
    [[nodiscard]] bool getValue(const char* pKey, T* pValueOut) const {
        if (pKey == nullptr || pValueOut == nullptr) {
            return false;
        }

        return mInfo != nullptr && mIndex >= 0 && mInfo->getValue(mIndex, pKey, pValueOut);
    }

    const JMapInfo* mInfo = nullptr;
    s32 mIndex = -1;
};

template < typename T >
JMapInfoIter JMapInfo::findElement(const char* pKey, T searchValue, int startIndex) const {
    auto entry_index = startIndex;
    while (entry_index < getNumEntries()) {
        auto value = T{};
        if (getValue< T >(entry_index, pKey, &value) && compareValues< T >(value, searchValue)) {
            return JMapInfoIter(this, entry_index);
        }
        ++entry_index;
    }
    return end();
}

namespace MR {
    JMapInfoIter findJMapInfoElementNoCase(const JMapInfo* pInfo, const char* pKey, const char* pValue, int startIndex);
}
