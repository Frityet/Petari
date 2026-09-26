#include "Game/System/SysConfigFile.hpp"
#include "Game/System/BinaryDataContentAccessor.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include <aurora/endian.hpp>
#include <aurora/exception.hpp>
#include <stdexcept>

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
    if (!validateData(pBuffer, size))
        return 3;
    s32 result;
    const char* pName;

    BinaryDataContentAccessor accessor = BinaryDataContentAccessor(const_cast< u8* >(pBuffer));
    u32 headerSize = accessor.getHeaderSize();
    u8* pData = const_cast< u8* >(pBuffer) + headerSize;

    pName = "mTimeAnnounced";
    mTimeAnnounced = aurora::endian::read_big< OSTime >(accessor.getPointer(pName, pData));

    pName = "mTimeSent";
    mTimeSent = aurora::endian::read_big< OSTime >(accessor.getPointer(pName, pData));

    pName = "mSentBytes";
    mSentBytes = aurora::endian::read_big< u32 >(accessor.getPointer(pName, pData));

    s32 newSize = headerSize + mHeaderSerializer->getDataSize();

    result = 3;

    if (newSize <= size) {
        result = 0;
    }

    return result;
}

void SysConfigChunk::initializeData() {
    mTimeAnnounced = 0;
    mTimeSent = 0;
    mSentBytes = 0;
}

void SysConfigChunk::initHeaderSerializer() {
    mHeaderSerializer = new BinaryDataContentHeaderSerializer(new u8[32], 32);

    mHeaderSerializer->addAttribute("mTimeAnnounced", sizeof(mTimeAnnounced));
    mHeaderSerializer->addAttribute("mTimeSent", sizeof(mTimeSent));
    mHeaderSerializer->addAttribute("mSentBytes", sizeof(mSentBytes));
    mHeaderSerializer->flush();
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
    SysConfigChunk* pChunk = mChunk;

    pChunk->mTimeAnnounced = OSGetTime();
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
    mChunkHolder->loadFromFileBinary(pBuffer, size);
}

s32 SysConfigChunk::serialize(u8* pBuffer, u32 size) const {
    if (pBuffer == nullptr || size < mHeaderSerializer->getHeaderSize() + mHeaderSerializer->getDataSize()) {
        aurora::throw_host_exception< std::length_error >("SYSC output is too small");
    }
    void* pSrcBuffer = mHeaderSerializer->mStream.mBuffer;
    const char* pName;

    MR::copyMemory(pBuffer, pSrcBuffer, mHeaderSerializer->getHeaderSize());

    u32 headerSize = mHeaderSerializer->getHeaderSize();
    BinaryDataContentAccessor accessor = BinaryDataContentAccessor(static_cast< u8* >(mHeaderSerializer->mStream.mBuffer));
    u8* pData = pBuffer + headerSize;

    pName = "mTimeAnnounced";
    OSTime* pTimeAnnounced = static_cast< OSTime* >(accessor.getPointer(pName, pData));

    pName = "mTimeSent";
    OSTime* pTimeSent = static_cast< OSTime* >(accessor.getPointer(pName, pData));

    pName = "mSentBytes";
    u32* pSentBytes = static_cast< u32* >(accessor.getPointer(pName, pData));

    aurora::endian::write_big(pTimeAnnounced, static_cast< u64 >(mTimeAnnounced));
    aurora::endian::write_big(pTimeSent, static_cast< u64 >(mTimeSent));
    aurora::endian::write_big(pSentBytes, static_cast< u32 >(mSentBytes));

    return headerSize + mHeaderSerializer->getDataSize();
}

bool SysConfigChunk::validateData(const u8* pData, u32 size) const {
    constexpr BinaryDataContentAccessor::Attribute attributes[] = {
        {"mTimeAnnounced", 8, true},
        {"mTimeSent", 8, true},
        {"mSentBytes", 4, true},
    };
    return BinaryDataContentAccessor::validate(pData, size, 1, attributes);
}
