#include "Game/System/ConfigDataMisc.hpp"

#include <stdexcept>

namespace {
constexpr auto cFlagLastLoadedMario = u8{0x1U};
constexpr auto cFlagCompleteEndingMario = u8{0x2U};
constexpr auto cFlagCompleteEndingLuigi = u8{0x4U};

[[nodiscard]] u64 read_be64(const u8* pData) {
    auto value = u64{};
    for (auto index = u32{}; index < sizeof(value); ++index) {
        value = (value << 8U) | pData[index];
    }
    return value;
}

void write_be64(u8* pData, u64 value) {
    for (auto index = u32{}; index < sizeof(value); ++index) {
        const auto shift = static_cast<unsigned>((sizeof(value) - 1U - index) * 8U);
        pData[index] = static_cast<u8>(value >> shift);
    }
}
}  // namespace

ConfigDataMisc::ConfigDataMisc() : mFlag(cFlagLastLoadedMario), mLastModified() {
    initializeData();
}

bool ConfigDataMisc::isLastLoadedMario() const {
    return !((mFlag & cFlagLastLoadedMario) - 1);
}

void ConfigDataMisc::setLastLoadedMario(bool lastLoadedMario) {
    if (lastLoadedMario) {
        mFlag |= cFlagLastLoadedMario;
    } else {
        mFlag &= ~cFlagLastLoadedMario;
    }
}

void ConfigDataMisc::onCompleteEndingMario() {
    mFlag |= cFlagCompleteEndingMario;
}

void ConfigDataMisc::onCompleteEndingLuigi() {
    mFlag |= cFlagCompleteEndingLuigi;
}

bool ConfigDataMisc::isOnCompleteEndingMario() {
    return (mFlag & cFlagCompleteEndingMario) != 0U;
}

bool ConfigDataMisc::isOnCompleteEndingLuigi() {
    return (mFlag & cFlagCompleteEndingLuigi) != 0U;
}

OSTime ConfigDataMisc::getLastModified() const {
    return mLastModified;
}

void ConfigDataMisc::updateLastModified() {
    mLastModified = OSGetTime();
}

u32 ConfigDataMisc::makeHeaderHashCode() const {
    return 0x1U;
}

u32 ConfigDataMisc::getSignature() const {
    return 'MISC';
}

s32 ConfigDataMisc::serialize(u8* pBuffer, u32 size) const {
    if (pBuffer == nullptr || size < 1U + sizeof(OSTime)) {
        throw std::length_error("Retail MISC chunk destination is too small");
    }
    pBuffer[0] = mFlag;
    write_be64(pBuffer + 1U, static_cast<u64>(mLastModified));
    return static_cast<s32>(1U + sizeof(OSTime));
}

s32 ConfigDataMisc::deserialize(const u8* pBuffer, u32 size) {
    initializeData();
    if (pBuffer == nullptr || size == 0U) {
        return 2;
    }

    mFlag = pBuffer[0];
    if (size == 1U) {
        mLastModified = 0;
        return 0;
    }
    if (size < 1U + sizeof(OSTime)) {
        return 2;
    }

    mLastModified = static_cast<OSTime>(read_be64(pBuffer + 1U));
    return 0;
}

void ConfigDataMisc::initializeData() {
    mFlag = cFlagLastLoadedMario;
    mLastModified = 0;
}
