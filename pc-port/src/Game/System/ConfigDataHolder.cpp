#include "Game/System/ConfigDataHolder.hpp"

#include <algorithm>
#include <cstring>
#include <cstdio>

namespace {
    constexpr u32 cIconIdMii = 0U;
    constexpr u32 cIconIdMario = 1U;

    struct ConfigDataBinary {
        char magic[4];
        bool created;
        bool last_loaded_mario;
        bool complete_ending_mario;
        bool complete_ending_luigi;
        bool uses_mii;
        u32 icon_id;
        s32 mii_index;
        u8 mii_id[16];
        OSTime last_modified;
    };
}  // namespace

ConfigDataHolder::ConfigDataHolder() {
    resetAllData();
    setName("config1");
}

void ConfigDataHolder::setIsCreated(bool isCreated) {
    mIsCreated = isCreated;
}

bool ConfigDataHolder::isCreated() const {
    return mIsCreated;
}

void ConfigDataHolder::setLastLoadedMario(bool lastLoadedMario) {
    mLastLoadedMario = lastLoadedMario;
}

bool ConfigDataHolder::isLastLoadedMario() const {
    return mLastLoadedMario;
}

void ConfigDataHolder::onCompleteEndingMario() {
    mCompleteEndingMario = true;
}

void ConfigDataHolder::onCompleteEndingLuigi() {
    mCompleteEndingLuigi = true;
}

bool ConfigDataHolder::isOnCompleteEndingMario() {
    return mCompleteEndingMario;
}

bool ConfigDataHolder::isOnCompleteEndingLuigi() {
    return mCompleteEndingLuigi;
}

void ConfigDataHolder::updateLastModified() {
    mLastModified = OSGetTime();
}

OSTime ConfigDataHolder::getLastModified() const {
    return mLastModified;
}

void ConfigDataHolder::setMiiOrIconId(const void* pMiiId, const u32* pIconId) {
    if (pMiiId != nullptr) {
        std::memcpy(mMiiId.data(), pMiiId, mMiiId.size());
        auto miiIndex = s32{};
        std::memcpy(&miiIndex, pMiiId, std::min(sizeof(miiIndex), mMiiId.size()));
        mMiiIndex = miiIndex;
        mUsesMii = true;
        mIconId = cIconIdMii;
        return;
    }

    mMiiId.fill(0U);
    mMiiIndex.reset();
    mUsesMii = false;
    mIconId = pIconId != nullptr ? *pIconId : cIconIdMario;
}

bool ConfigDataHolder::getMiiId(void* pMiiId) const {
    if (pMiiId != nullptr) {
        std::memcpy(pMiiId, mMiiId.data(), mMiiId.size());
    }

    return mUsesMii;
}

bool ConfigDataHolder::getIconId(u32* pIconId) const {
    if (pIconId != nullptr) {
        *pIconId = mIconId;
    }

    return !mUsesMii;
}

void ConfigDataHolder::resetAllData() {
    mIsCreated = false;
    mLastLoadedMario = true;
    mCompleteEndingMario = false;
    mCompleteEndingLuigi = false;
    mUsesMii = false;
    mIconId = cIconIdMario;
    mMiiIndex.reset();
    mMiiId.fill(0U);
    mLastModified = 0;
}

s32 ConfigDataHolder::makeFileBinary(u8* pBuffer, u32 size) {
    if (pBuffer == nullptr || size < sizeof(ConfigDataBinary)) {
        return 0;
    }

    auto binary = ConfigDataBinary{
        .magic = {'C', 'F', 'G', '1'},
        .created = mIsCreated,
        .last_loaded_mario = mLastLoadedMario,
        .complete_ending_mario = mCompleteEndingMario,
        .complete_ending_luigi = mCompleteEndingLuigi,
        .uses_mii = mUsesMii,
        .icon_id = mIconId,
        .mii_index = mMiiIndex.value_or(-1),
        .mii_id = {},
        .last_modified = mLastModified,
    };
    std::copy(mMiiId.begin(), mMiiId.end(), std::begin(binary.mii_id));
    std::memcpy(pBuffer, &binary, sizeof(binary));
    return static_cast<s32>(sizeof(binary));
}

bool ConfigDataHolder::loadFromFileBinary(const char* pName, const u8* pBuffer, u32 size) {
    setName(pName);
    if (pBuffer == nullptr || size < sizeof(ConfigDataBinary)) {
        resetAllData();
        return true;
    }

    auto binary = ConfigDataBinary{};
    std::memcpy(&binary, pBuffer, sizeof(binary));
    if (std::memcmp(binary.magic, "CFG1", 4U) != 0) {
        resetAllData();
        return false;
    }

    mIsCreated = binary.created;
    mLastLoadedMario = binary.last_loaded_mario;
    mCompleteEndingMario = binary.complete_ending_mario;
    mCompleteEndingLuigi = binary.complete_ending_luigi;
    mUsesMii = binary.uses_mii;
    mIconId = binary.icon_id;
    mMiiIndex = binary.mii_index >= 0 ? std::optional<s32>(binary.mii_index) : std::nullopt;
    std::copy(std::begin(binary.mii_id), std::end(binary.mii_id), mMiiId.begin());
    mLastModified = binary.last_modified;
    return true;
}

void ConfigDataHolder::setCompatMiiIndex(std::optional<s32> miiIndex) {
    mMiiIndex = miiIndex;
    mUsesMii = miiIndex.has_value();
    if (mUsesMii) {
        mIconId = cIconIdMii;
        mMiiId.fill(0U);
        const auto index = *miiIndex;
        std::memcpy(mMiiId.data(), &index, std::min(sizeof(index), mMiiId.size()));
    }
}

std::optional<s32> ConfigDataHolder::getCompatMiiIndex() const {
    return mUsesMii ? mMiiIndex : std::nullopt;
}

void ConfigDataHolder::setName(const char* pName) {
    std::snprintf(mName, sizeof(mName), "%s", pName != nullptr ? pName : "config1");
}
