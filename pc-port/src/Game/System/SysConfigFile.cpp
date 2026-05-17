#include "Game/System/SysConfigFile.hpp"

#include <cstring>

namespace {
    struct SysConfigBinary {
        char magic[4];
        OSTime time_announced;
        OSTime time_sent;
        u32 sent_bytes;
    };
}  // namespace

SysConfigFile::SysConfigFile() = default;

OSTime SysConfigFile::getTimeAnnounced() const {
    return mTimeAnnounced;
}

void SysConfigFile::setTimeAnnounced(OSTime time) {
    mTimeAnnounced = time;
}

void SysConfigFile::updateTimeAnnounced() {
    mTimeAnnounced = OSGetTime();
}

OSTime SysConfigFile::getTimeSent() const {
    return mTimeSent;
}

void SysConfigFile::setTimeSent(OSTime time) {
    mTimeSent = time;
}

u32 SysConfigFile::getSentBytes() const {
    return mSentBytes;
}

void SysConfigFile::setSentBytes(u32 bytes) {
    mSentBytes = bytes;
}

void SysConfigFile::makeDataBinary(u8* pBuffer, u32 size) const {
    if (pBuffer == nullptr || size < sizeof(SysConfigBinary)) {
        return;
    }

    const auto binary = SysConfigBinary{
        .magic = {'S', 'Y', 'S', '1'},
        .time_announced = mTimeAnnounced,
        .time_sent = mTimeSent,
        .sent_bytes = mSentBytes,
    };
    std::memcpy(pBuffer, &binary, sizeof(binary));
}

void SysConfigFile::loadFromDataBinary(const u8* pBuffer, u32 size) {
    if (pBuffer == nullptr || size < sizeof(SysConfigBinary)) {
        return;
    }

    auto binary = SysConfigBinary{};
    std::memcpy(&binary, pBuffer, sizeof(binary));
    if (std::memcmp(binary.magic, "SYS1", 4U) != 0) {
        return;
    }

    mTimeAnnounced = binary.time_announced;
    mTimeSent = binary.time_sent;
    mSentBytes = binary.sent_bytes;
}
