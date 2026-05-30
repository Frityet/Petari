#include "Game/Util/JMapInfo.hpp"

#include "resource/BcsvTable.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <string_view>

namespace {
    [[nodiscard]] bool valid_entry(const smgpc::resource::BcsvTable* table, int entryIndex) {
        return table != nullptr && entryIndex >= 0 && static_cast< std::uint32_t >(entryIndex) < table->entry_count();
    }

    [[nodiscard]] s32 jmap_value_type(smgpc::resource::BcsvFieldType type) {
        switch (type) {
        case smgpc::resource::BcsvFieldType::Int32:
            return JMAP_VALUE_TYPE_LONG;
        case smgpc::resource::BcsvFieldType::InlineString:
            return JMAP_VALUE_TYPE_STRING;
        case smgpc::resource::BcsvFieldType::Float:
            return JMAP_VALUE_TYPE_FLOAT;
        case smgpc::resource::BcsvFieldType::UInt32:
            return JMAP_VALUE_TYPE_LONG_2;
        case smgpc::resource::BcsvFieldType::Int16:
            return JMAP_VALUE_TYPE_SHORT;
        case smgpc::resource::BcsvFieldType::Int8:
            return JMAP_VALUE_TYPE_BYTE;
        case smgpc::resource::BcsvFieldType::StringOffset:
            return JMAP_VALUE_TYPE_STRING_PTR;
        }

        return JMAP_VALUE_TYPE_NULL;
    }

    [[nodiscard]] int compare_no_case(std::string_view lhs, std::string_view rhs) {
        const auto length = std::min(lhs.size(), rhs.size());
        for (std::size_t i = 0U; i < length; ++i) {
            const auto lhs_char = static_cast< unsigned char >(lhs[i]);
            const auto rhs_char = static_cast< unsigned char >(rhs[i]);
            const auto diff = std::tolower(lhs_char) - std::tolower(rhs_char);
            if (diff != 0) {
                return diff;
            }
        }

        if (lhs.size() == rhs.size()) {
            return 0;
        }
        return lhs.size() < rhs.size() ? -1 : 1;
    }
}  // namespace

JMapInfo::JMapInfo(smgpc::resource::BcsvTable table) : mTable(std::make_shared< smgpc::resource::BcsvTable >(std::move(table))) {
}

JMapInfo JMapInfo::from_bcsv(std::span< const std::uint8_t > data) {
    return JMapInfo(smgpc::resource::BcsvTable::from_bytes(data));
}

bool JMapInfo::dataExists() const {
    return mTable != nullptr;
}

int JMapInfo::getNumEntries() const {
    if (mTable == nullptr) {
        return 0;
    }

    const auto count = mTable->entry_count();
    return count > static_cast< std::uint32_t >(std::numeric_limits< int >::max()) ? std::numeric_limits< int >::max() : static_cast< int >(count);
}

int JMapInfo::getNumFields() const {
    if (mTable == nullptr) {
        return 0;
    }

    const auto count = mTable->fields().size();
    return count > static_cast< std::size_t >(std::numeric_limits< int >::max()) ? std::numeric_limits< int >::max() : static_cast< int >(count);
}

bool JMapInfo::attach(const void*) {
    return false;
}

void JMapInfo::setName(const char* pName) {
    mName = pName != nullptr ? pName : "";
}

const char* JMapInfo::getName() const {
    return mName.c_str();
}

void JMapInfo::setChildObjInfo(JMapInfo info) {
    mChildObjInfo = std::make_shared< JMapInfo >(std::move(info));
}

const JMapInfo* JMapInfo::getChildObjInfo() const {
    return mChildObjInfo.get();
}

void JMapInfo::setValue(int entryIndex, const char* pKey, f32 value) {
    if (pKey == nullptr || entryIndex < 0 || entryIndex >= getNumEntries()) {
        return;
    }

    mFloatOverrides[{entryIndex, smgpc::resource::jmap_hash(pKey)}] = value;
}

s32 JMapInfo::searchItemInfo(const char* pKey) const {
    if (mTable == nullptr || pKey == nullptr) {
        return -1;
    }

    const auto index = mTable->field_index(pKey);
    if (!index.has_value() || *index > static_cast< std::size_t >(std::numeric_limits< s32 >::max())) {
        return -1;
    }

    return static_cast< s32 >(*index);
}

s32 JMapInfo::getValueType(const char* pKey) const {
    const auto item_index = searchItemInfo(pKey);
    if (mTable == nullptr || item_index < 0) {
        return JMAP_VALUE_TYPE_NULL;
    }

    return jmap_value_type(mTable->fields()[static_cast< std::size_t >(item_index)].type);
}

bool JMapInfo::getValueFast(int entryIndex, int itemIndex, const char** pValueOut) const {
    if (mTable == nullptr || pValueOut == nullptr || itemIndex < 0 || itemIndex >= getNumFields()) {
        return false;
    }

    return getStringValueByHash(entryIndex, mTable->fields()[static_cast< std::size_t >(itemIndex)].hash, pValueOut);
}

bool JMapInfo::getValueFast(int entryIndex, int itemIndex, u32* pValueOut) const {
    if (mTable == nullptr || pValueOut == nullptr || itemIndex < 0 || itemIndex >= getNumFields()) {
        return false;
    }

    return getUnsignedValueByHash(entryIndex, mTable->fields()[static_cast< std::size_t >(itemIndex)].hash, pValueOut);
}

bool JMapInfo::getValueFast(int entryIndex, int itemIndex, s32* pValueOut) const {
    if (mTable == nullptr || pValueOut == nullptr || itemIndex < 0 || itemIndex >= getNumFields()) {
        return false;
    }

    return getSignedValueByHash(entryIndex, mTable->fields()[static_cast< std::size_t >(itemIndex)].hash, pValueOut);
}

bool JMapInfo::getValueFast(int entryIndex, int itemIndex, f32* pValueOut) const {
    if (mTable == nullptr || pValueOut == nullptr || itemIndex < 0 || itemIndex >= getNumFields()) {
        return false;
    }

    const auto hash = mTable->fields()[static_cast< std::size_t >(itemIndex)].hash;
    if (getFloatValueByHash(entryIndex, hash, pValueOut)) {
        return true;
    }

    auto signed_value = s32{};
    if (!getSignedValueByHash(entryIndex, hash, &signed_value)) {
        return false;
    }

    *pValueOut = static_cast< f32 >(signed_value);
    return true;
}

bool JMapInfo::getValueFast(int entryIndex, int itemIndex, bool* pValueOut) const {
    if (pValueOut == nullptr) {
        return false;
    }

    auto value = s32{};
    if (!getValueFast(entryIndex, itemIndex, &value)) {
        return false;
    }

    *pValueOut = value != 0;
    return true;
}

JMapInfoIter JMapInfo::findElementBinary(const char* pKey, const char* pValue) const {
    return findElement(pKey, pValue, 0);
}

JMapInfoIter JMapInfo::end() const {
    return JMapInfoIter(this, getNumEntries());
}

bool JMapInfo::getSignedValue(int entryIndex, const char* pKey, s32* pValueOut) const {
    if (mTable == nullptr || pKey == nullptr || pValueOut == nullptr || entryIndex < 0 || entryIndex >= getNumEntries()) {
        return false;
    }

    return getSignedValueByHash(entryIndex, smgpc::resource::jmap_hash(pKey), pValueOut);
}

bool JMapInfo::getUnsignedValue(int entryIndex, const char* pKey, u32* pValueOut) const {
    if (mTable == nullptr || pKey == nullptr || pValueOut == nullptr || entryIndex < 0 || entryIndex >= getNumEntries()) {
        return false;
    }

    return getUnsignedValueByHash(entryIndex, smgpc::resource::jmap_hash(pKey), pValueOut);
}

bool JMapInfo::getFloatValue(int entryIndex, const char* pKey, f32* pValueOut) const {
    if (mTable == nullptr || pKey == nullptr || pValueOut == nullptr || entryIndex < 0 || entryIndex >= getNumEntries()) {
        return false;
    }

    return getFloatValueByHash(entryIndex, smgpc::resource::jmap_hash(pKey), pValueOut);
}

bool JMapInfo::getStringValue(int entryIndex, const char* pKey, const char** pValueOut) const {
    if (mTable == nullptr || pKey == nullptr || pValueOut == nullptr || entryIndex < 0 || entryIndex >= getNumEntries()) {
        return false;
    }

    return getStringValueByHash(entryIndex, smgpc::resource::jmap_hash(pKey), pValueOut);
}

bool JMapInfo::getSignedValueByHash(int entryIndex, std::uint32_t hash, s32* pValueOut) const {
    if (!valid_entry(mTable.get(), entryIndex) || pValueOut == nullptr) {
        return false;
    }

    const auto value = mTable->get_s32(static_cast< std::size_t >(entryIndex), hash);
    if (!value.has_value()) {
        return false;
    }

    *pValueOut = *value;
    return true;
}

bool JMapInfo::getUnsignedValueByHash(int entryIndex, std::uint32_t hash, u32* pValueOut) const {
    if (!valid_entry(mTable.get(), entryIndex) || pValueOut == nullptr) {
        return false;
    }

    const auto value = mTable->get_u32(static_cast< std::size_t >(entryIndex), hash);
    if (!value.has_value()) {
        return false;
    }

    *pValueOut = *value;
    return true;
}

bool JMapInfo::getFloatValueByHash(int entryIndex, std::uint32_t hash, f32* pValueOut) const {
    if (!valid_entry(mTable.get(), entryIndex) || pValueOut == nullptr) {
        return false;
    }

    if (const auto override = mFloatOverrides.find({entryIndex, hash}); override != mFloatOverrides.end()) {
        *pValueOut = override->second;
        return true;
    }

    const auto value = mTable->get_float(static_cast< std::size_t >(entryIndex), hash);
    if (!value.has_value()) {
        return false;
    }

    *pValueOut = *value;
    return true;
}

bool JMapInfo::getStringValueByHash(int entryIndex, std::uint32_t hash, const char** pValueOut) const {
    if (!valid_entry(mTable.get(), entryIndex) || pValueOut == nullptr) {
        return false;
    }

    const auto value = mTable->get_string(static_cast< std::size_t >(entryIndex), hash);
    if (!value.has_value()) {
        return false;
    }

    auto& cached = mStringCache[{entryIndex, hash}];
    cached = *value;
    *pValueOut = cached.c_str();
    return true;
}

namespace MR {
    JMapInfoIter findJMapInfoElementNoCase(const JMapInfo* pInfo, const char* pKey, const char* pValue, int startIndex) {
        if (pInfo == nullptr || pKey == nullptr || pValue == nullptr) {
            return {};
        }

        for (auto entry_index = startIndex; entry_index < pInfo->getNumEntries(); ++entry_index) {
            const char* value = nullptr;
            if (pInfo->getValue(entry_index, pKey, &value) && value != nullptr && compare_no_case(value, pValue) == 0) {
                return JMapInfoIter(pInfo, entry_index);
            }
        }

        return pInfo->end();
    }
}  // namespace MR
