#include "Game/System/ConfigDataMisc.hpp"

#include "compat/SaveDataEndian.hpp"

#include <chrono>

namespace {
constexpr u8 FLAG_LAST_LOADED_MARIO = 0x1U;
constexpr u8 FLAG_COMPLETE_ENDING_MARIO = 0x2U;
constexpr u8 FLAG_COMPLETE_ENDING_LUIGI = 0x4U;

[[nodiscard]] OSTime get_host_time() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}
}  // namespace

ConfigDataMisc::ConfigDataMisc()
    : mFlag(FLAG_LAST_LOADED_MARIO), mLastModified(0) {
    initializeData();
}

u32 ConfigDataMisc::makeHeaderHashCode() const {
    return 0x1U;
}

u32 ConfigDataMisc::getSignature() const {
    return 0x4D495343U;
}

s32 ConfigDataMisc::serialize(u8 *pBuffer, u32 size) const {
    if (pBuffer == nullptr || size < 9U) {
        return 0;
    }

    pBuffer[0] = mFlag;
    SaveDataEndian::write_u64(pBuffer + 1U, static_cast<u64>(mLastModified));
    return 9;
}

s32 ConfigDataMisc::deserialize(const u8 *pBuffer, u32 size) {
    initializeData();
    if (pBuffer == nullptr || size < 1U) {
        return 1;
    }

    mFlag = pBuffer[0];
    if (size >= 9U) {
        mLastModified = static_cast<OSTime>(SaveDataEndian::read_u64(pBuffer + 1U));
    }
    return 0;
}

void ConfigDataMisc::initializeData() {
    mFlag = FLAG_LAST_LOADED_MARIO;
    mLastModified = 0;
}

bool ConfigDataMisc::isLastLoadedMario() const {
    return (mFlag & FLAG_LAST_LOADED_MARIO) != 0U;
}

void ConfigDataMisc::setLastLoadedMario(bool lastLoadedMario) {
    if (lastLoadedMario) {
        mFlag |= FLAG_LAST_LOADED_MARIO;
    } else {
        mFlag &= static_cast<u8>(~FLAG_LAST_LOADED_MARIO);
    }
}

void ConfigDataMisc::onCompleteEndingMario() {
    mFlag |= FLAG_COMPLETE_ENDING_MARIO;
}

void ConfigDataMisc::onCompleteEndingLuigi() {
    mFlag |= FLAG_COMPLETE_ENDING_LUIGI;
}

bool ConfigDataMisc::isOnCompleteEndingMario() {
    return (mFlag & FLAG_COMPLETE_ENDING_MARIO) != 0U;
}

bool ConfigDataMisc::isOnCompleteEndingLuigi() {
    return (mFlag & FLAG_COMPLETE_ENDING_LUIGI) != 0U;
}

OSTime ConfigDataMisc::getLastModified() const {
    return mLastModified;
}

void ConfigDataMisc::updateLastModified() {
    mLastModified = get_host_time();
}
