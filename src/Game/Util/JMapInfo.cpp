#include "Game/Util/JMapInfo.hpp"

#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <stdexcept>
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

JMapInfo::JMapInfo(const smgpc::resource::BcsvTable& table) {
    smgpc::compat::JkrHostAllocationScope host;
    mData = std::make_shared< DataCompat >(static_cast< s32 >(table.entry_count()));
    mTable = std::make_shared< smgpc::resource::BcsvTable >(table);
    mSourceData = mTable->bytes().data();
    mSourceOwner = mTable;
}

JMapInfo::JMapInfo(const JMapInfo& info) : smgpc::compat::NativeJkrDisposer(info) {
    *this = info;
}

JMapInfo::JMapInfo(JMapInfo&& info) noexcept : smgpc::compat::NativeJkrDisposer(std::move(info)) {
    *this = std::move(info);
}

JMapInfo& JMapInfo::operator=(const JMapInfo& info) {
    smgpc::compat::JkrHostAllocationScope host;
    if (this != &info) {
        mData = info.mData;
        mTable = info.mTable;
        mSourceData = info.mSourceData;
        mSourceOwner = info.mSourceOwner;
        mChildObjInfo = info.mChildObjInfo;
        mRailInfo = info.mRailInfo;
        mName = info.mName;
        mPlacedZoneId = info.mPlacedZoneId;
        mFloatOverrides = info.mFloatOverrides;
    }
    return *this;
}

JMapInfo& JMapInfo::operator=(JMapInfo&& info) noexcept {
    smgpc::compat::JkrHostAllocationScope host;
    if (this != &info) {
        mData = std::move(info.mData);
        mTable = std::move(info.mTable);
        mSourceData = std::exchange(info.mSourceData, nullptr);
        mSourceOwner = std::move(info.mSourceOwner);
        mChildObjInfo = std::move(info.mChildObjInfo);
        mRailInfo = std::move(info.mRailInfo);
        mName = std::move(info.mName);
        mPlacedZoneId = info.mPlacedZoneId;
        mFloatOverrides = std::move(info.mFloatOverrides);
    }
    return *this;
}

JMapInfo::~JMapInfo() = default;

JMapInfo JMapInfo::from_bcsv(std::span< const std::uint8_t > data) {
    smgpc::compat::JkrHostAllocationScope host;
    JMapInfo info;
    info.mTable = std::make_shared< smgpc::resource::BcsvTable >(smgpc::resource::BcsvTable::from_bytes(data));
    info.mData = std::make_shared< DataCompat >(static_cast< s32 >(info.mTable->entry_count()));
    info.mSourceData = info.mTable->bytes().data();
    info.mSourceOwner = info.mTable;
    return info;
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

const void* JMapInfo::getData() const {
    return mSourceData;
}

const char* JMapInfo::getEntryData(s32 entryIndex) const {
    if (!mTable) throw std::logic_error("JMap entry address requires attached table data");
    const auto address = reinterpret_cast<std::uintptr_t>(mSourceData) + mTable->data_offset() +
                         mTable->entry_size() * entryIndex;
    return reinterpret_cast<const char*>(address);
}

u32 JMapInfo::getDataSize() const {
    if (!mTable) throw std::logic_error("JMap data extent requires attached table data");
    return mTable->data_offset() + mTable->entry_size() * mTable->entry_count();
}

bool JMapInfo::attach(const void* data) {
    smgpc::compat::JkrHostAllocationScope host;
    if (data == nullptr) {
        return false;
    }
    const auto resource = smgpc::resource::find_jmap_resource(data);
    if (resource == nullptr) {
        throw std::invalid_argument("JMapInfo::attach requires a retained, bounded JMap resource.");
    }
    mData = resource->mData;
    mTable = resource->mTable;
    mSourceData = data;
    mSourceOwner = resource;
    mFloatOverrides.clear();
    return true;
}

void JMapInfo::setName(const char* pName) {
    smgpc::compat::JkrHostAllocationScope host;
    mName = pName != nullptr ? pName : "";
}

const char* JMapInfo::getName() const {
    return mName.c_str();
}

void JMapInfo::setPlacedZoneId(s32 zoneId) {
    mPlacedZoneId = zoneId;
}

s32 JMapInfo::getPlacedZoneId() const {
    return mPlacedZoneId;
}

void JMapInfo::setChildObjInfo(JMapInfo info) {
    smgpc::compat::JkrHostAllocationScope host;
    mChildObjInfo = std::make_shared< JMapInfo >(std::move(info));
}

const JMapInfo* JMapInfo::getChildObjInfo() const {
    return mChildObjInfo.get();
}

void JMapInfo::setRailInfo(int entryIndex, JMapInfo pathInfo, JMapInfo pointInfo, s32 pathInfoIndex) {
    smgpc::compat::JkrHostAllocationScope host;
    if (entryIndex < 0 || entryIndex >= getNumEntries() || pathInfoIndex < 0) {
        return;
    }

    mRailInfo[entryIndex] = RailInfo{
        .mPathInfo = std::make_shared< JMapInfo >(std::move(pathInfo)),
        .mPointInfo = std::make_shared< JMapInfo >(std::move(pointInfo)),
        .mPathInfoIndex = pathInfoIndex,
    };
}

bool JMapInfo::getRailInfo(int entryIndex, const JMapInfo** pPathInfo, const JMapInfo** pPointInfo, s32* pPathInfoIndex) const {
    if (pPathInfo != nullptr) {
        *pPathInfo = nullptr;
    }
    if (pPointInfo != nullptr) {
        *pPointInfo = nullptr;
    }
    if (pPathInfoIndex != nullptr) {
        *pPathInfoIndex = -1;
    }

    const auto found = mRailInfo.find(entryIndex);
    if (found == mRailInfo.end() || found->second.mPathInfo == nullptr || found->second.mPointInfo == nullptr) {
        return false;
    }

    if (pPathInfo != nullptr) {
        *pPathInfo = found->second.mPathInfo.get();
    }
    if (pPointInfo != nullptr) {
        *pPointInfo = found->second.mPointInfo.get();
    }
    if (pPathInfoIndex != nullptr) {
        *pPathInfoIndex = found->second.mPathInfoIndex;
    }
    return true;
}

void JMapInfo::setValue(int entryIndex, const char* pKey, f32 value) {
    smgpc::compat::JkrHostAllocationScope host;
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
    smgpc::compat::JkrHostAllocationScope host;
    if (!valid_entry(mTable.get(), entryIndex) || pValueOut == nullptr) {
        return false;
    }

    const auto value = mTable->get_string(static_cast< std::size_t >(entryIndex), hash);
    if (!value.has_value()) {
        return false;
    }

    const std::lock_guard lock(mData->mStringMutex);
    const auto [cached, inserted] = mData->mStringCache.try_emplace(std::pair{entryIndex, hash}, *value);
    *pValueOut = cached->second.c_str();
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
