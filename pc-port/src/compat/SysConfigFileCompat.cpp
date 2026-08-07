#include "Game/System/SysConfigFile.hpp"

#include <cstring>
#include <limits>
#include <stdexcept>

#include "Game/Util/HashUtil.hpp"

namespace {
constexpr auto cAttributeCount = u16{3U};
constexpr auto cHeaderSize = u32{16U};
constexpr auto cDataSize = u32{20U};
constexpr auto cBinarySize = cHeaderSize + cDataSize;

[[nodiscard]] u16 read_be16(const u8* data) {
    return static_cast<u16>((static_cast<u16>(data[0]) << 8U) | static_cast<u16>(data[1]));
}

[[nodiscard]] u32 read_be32(const u8* data) {
    return (static_cast<u32>(data[0]) << 24U) | (static_cast<u32>(data[1]) << 16U) |
           (static_cast<u32>(data[2]) << 8U) | static_cast<u32>(data[3]);
}

[[nodiscard]] u64 read_be64(const u8* data) {
    auto value = u64{};
    for (auto index = u32{}; index < 8U; ++index) {
        value = (value << 8U) | static_cast<u64>(data[index]);
    }
    return value;
}

void write_be16(u8* data, u16 value) {
    data[0] = static_cast<u8>(value >> 8U);
    data[1] = static_cast<u8>(value);
}

void write_be32(u8* data, u32 value) {
    data[0] = static_cast<u8>(value >> 24U);
    data[1] = static_cast<u8>(value >> 16U);
    data[2] = static_cast<u8>(value >> 8U);
    data[3] = static_cast<u8>(value);
}

void write_be64(u8* data, u64 value) {
    for (auto index = u32{}; index < 8U; ++index) {
        data[7U - index] = static_cast<u8>(value >> (index * 8U));
    }
}

[[nodiscard]] u16 attribute_hash(const char* name) {
    return static_cast<u16>(MR::getHashCode(name));
}

void write_attribute(u8* data, u32 index, const char* name, u16 offset) {
    const auto entry = 4U + index * 4U;
    write_be16(data + entry, attribute_hash(name));
    write_be16(data + entry + 2U, offset);
}

[[nodiscard]] const u8* find_attribute(const u8* data, u32 size, const char* name, u32 value_size) {
    if (data == nullptr || size < 4U) {
        return nullptr;
    }
    const auto count = static_cast<u32>(read_be16(data));
    if (count > (std::numeric_limits<u32>::max() - 4U) / 4U) {
        return nullptr;
    }
    const auto header_size = 4U + count * 4U;
    const auto data_size = static_cast<u32>(read_be16(data + 2U));
    if (header_size > size || data_size > size - header_size) {
        return nullptr;
    }
    const auto expected_hash = attribute_hash(name);
    for (auto index = u32{}; index < count; ++index) {
        const auto entry = 4U + index * 4U;
        if (read_be16(data + entry) != expected_hash) {
            continue;
        }
        const auto offset = static_cast<u32>(read_be16(data + entry + 2U));
        if (offset > data_size || value_size > data_size - offset) {
            return nullptr;
        }
        return data + header_size + offset;
    }
    return nullptr;
}
}  // namespace

SysConfigChunk::SysConfigChunk() : mHeaderSerializer(nullptr) {
    initHeaderSerializer();
    initializeData();
}

u32 SysConfigChunk::makeHeaderHashCode() const {
    return 0x1;
}

u32 SysConfigChunk::getSignature() const {
    return 'SYSC';
}

s32 SysConfigChunk::deserialize(const u8* pBuffer, u32 size) {
    const auto* announced = find_attribute(pBuffer, size, "mTimeAnnounced", sizeof(OSTime));
    const auto* sent = find_attribute(pBuffer, size, "mTimeSent", sizeof(OSTime));
    const auto* sent_bytes = find_attribute(pBuffer, size, "mSentBytes", sizeof(u32));
    if (announced == nullptr || sent == nullptr || sent_bytes == nullptr) {
        return 3;
    }

    mTimeAnnounced = static_cast<OSTime>(read_be64(announced));
    mTimeSent = static_cast<OSTime>(read_be64(sent));
    mSentBytes = read_be32(sent_bytes);
    return 0;
}

void SysConfigChunk::initializeData() {
    mTimeAnnounced = 0;
    mTimeSent = 0;
    mSentBytes = 0;
}

void SysConfigChunk::initHeaderSerializer() {
    // The PPC serializer's pointer is retained in the exact class layout. The
    // host writes the proven retail field table explicitly below.
    mHeaderSerializer = nullptr;
}

s32 SysConfigChunk::serialize(u8* pBuffer, u32 size) const {
    if (pBuffer == nullptr || size < cBinarySize) {
        throw std::length_error("Retail SYSC chunk destination is unavailable or too small");
    }
    std::memset(pBuffer, 0, cBinarySize);
    write_be16(pBuffer, cAttributeCount);
    write_be16(pBuffer + 2U, static_cast<u16>(cDataSize));
    write_attribute(pBuffer, 0U, "mTimeAnnounced", 0U);
    write_attribute(pBuffer, 1U, "mTimeSent", 8U);
    write_attribute(pBuffer, 2U, "mSentBytes", 16U);
    write_be64(pBuffer + cHeaderSize, static_cast<u64>(mTimeAnnounced));
    write_be64(pBuffer + cHeaderSize + 8U, static_cast<u64>(mTimeSent));
    write_be32(pBuffer + cHeaderSize + 16U, mSentBytes);
    return static_cast<s32>(cBinarySize);
}

SysConfigFile::SysConfigFile() : mChunk(nullptr), mChunkHolder(nullptr) {
    mChunkHolder = new BinaryDataChunkHolder(0x3000, 8);
    mChunk = new SysConfigChunk();
    mChunkHolder->addChunk(mChunk);
}

OSTime SysConfigFile::getTimeAnnounced() {
    return mChunk->mTimeAnnounced;
}

void SysConfigFile::updateTimeAnnounced() {
    mChunk->mTimeAnnounced = OSGetTime();
}

OSTime SysConfigFile::getTimeSent() {
    return mChunk->mTimeSent;
}

void SysConfigFile::setTimeSent(OSTime timeSent) {
    mChunk->mTimeSent = timeSent;
}

u32 SysConfigFile::getSentBytes() {
    return mChunk->mSentBytes;
}

void SysConfigFile::setSentBytes(u32 sentBytes) {
    mChunk->mSentBytes = sentBytes;
}

void SysConfigFile::makeDataBinary(u8* pBuffer, u32 size) const {
    mChunkHolder->makeFileBinary(pBuffer, size);
}

void SysConfigFile::loadFromDataBinary(const u8* pBuffer, u32 size) {
    if (!mChunkHolder->loadFromFileBinary(pBuffer, size)) {
        throw std::invalid_argument("SYSC data is not a valid retail binary chunk file");
    }
}
