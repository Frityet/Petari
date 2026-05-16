#include "Game/System/SysConfigFile.hpp"

#include "Game/System/BinaryDataContentAccessor.hpp"
#include "compat/SaveDataEndian.hpp"
#include "Game/Util/MemoryUtil.hpp"

#include <chrono>

namespace {
[[nodiscard]] OSTime get_host_time() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}
}  // namespace

SysConfigChunk::SysConfigChunk()
    : mTimeAnnounced(0), mTimeSent(0), mSentBytes(0), mHeaderSerializer(nullptr) {
    initHeaderSerializer();
    initializeData();
}

SysConfigChunk::~SysConfigChunk() {
    delete[] static_cast<u8 *>(mHeaderSerializer->getBuffer());
    delete mHeaderSerializer;
}

u32 SysConfigChunk::makeHeaderHashCode() const {
    return 0x1U;
}

u32 SysConfigChunk::getSignature() const {
    return 0x53595343U;
}

s32 SysConfigChunk::serialize(u8 *pBuffer, u32 size) const {
    if (pBuffer == nullptr || size < mHeaderSerializer->getHeaderSize() + mHeaderSerializer->getDataSize()) {
        return 0;
    }

    MR::copyMemory(pBuffer, mHeaderSerializer->getBuffer(), mHeaderSerializer->getHeaderSize());
    BinaryDataContentAccessor accessor(static_cast<u8 *>(mHeaderSerializer->getBuffer()));
    u8 *pData = pBuffer + mHeaderSerializer->getHeaderSize();

    SaveDataEndian::write_u64(static_cast<u8 *>(accessor.getPointer("mTimeAnnounced", pData)), static_cast<u64>(mTimeAnnounced));
    SaveDataEndian::write_u64(static_cast<u8 *>(accessor.getPointer("mTimeSent", pData)), static_cast<u64>(mTimeSent));
    SaveDataEndian::write_u32(static_cast<u8 *>(accessor.getPointer("mSentBytes", pData)), mSentBytes);

    return static_cast<s32>(mHeaderSerializer->getHeaderSize() + mHeaderSerializer->getDataSize());
}

s32 SysConfigChunk::deserialize(const u8 *pBuffer, u32 size) {
    if (pBuffer == nullptr || size < 4U) {
        initializeData();
        return 1;
    }

    BinaryDataContentAccessor accessor(const_cast<u8 *>(pBuffer));
    const auto headerSize = static_cast<u32>(accessor.getHeaderSize());
    if (size < headerSize + static_cast<u32>(accessor.getDataSize())) {
        initializeData();
        return 3;
    }

    u8 *pData = const_cast<u8 *>(pBuffer) + headerSize;
    mTimeAnnounced = static_cast<OSTime>(SaveDataEndian::read_u64(static_cast<u8 *>(accessor.getPointer("mTimeAnnounced", pData))));
    mTimeSent = static_cast<OSTime>(SaveDataEndian::read_u64(static_cast<u8 *>(accessor.getPointer("mTimeSent", pData))));
    mSentBytes = SaveDataEndian::read_u32(static_cast<u8 *>(accessor.getPointer("mSentBytes", pData)));
    return 0;
}

void SysConfigChunk::initializeData() {
    mTimeAnnounced = 0;
    mTimeSent = 0;
    mSentBytes = 0;
}

void SysConfigChunk::initHeaderSerializer() {
    mHeaderSerializer = new BinaryDataContentHeaderSerializer(new u8[32], 32);
    mHeaderSerializer->addAttribute("mTimeAnnounced", sizeof(u64));
    mHeaderSerializer->addAttribute("mTimeSent", sizeof(u64));
    mHeaderSerializer->addAttribute("mSentBytes", sizeof(u32));
    mHeaderSerializer->flush();
}

SysConfigFile::SysConfigFile()
    : mChunk(new SysConfigChunk()), mChunkHolder(new BinaryDataChunkHolder(0x3000U, 8)) {
    mChunkHolder->addChunk(mChunk);
}

SysConfigFile::~SysConfigFile() {
    delete mChunkHolder;
    delete mChunk;
}

OSTime SysConfigFile::getTimeAnnounced() {
    return mChunk->mTimeAnnounced;
}

void SysConfigFile::updateTimeAnnounced() {
    mChunk->mTimeAnnounced = get_host_time();
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

void SysConfigFile::makeDataBinary(u8 *pBuffer, u32 size) const {
    mChunkHolder->makeFileBinary(pBuffer, size);
}

void SysConfigFile::loadFromDataBinary(const u8 *pBuffer, u32 size) {
    mChunkHolder->loadFromFileBinary(pBuffer, size);
}
