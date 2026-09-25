#include "Game/System/BinaryDataContentAccessor.hpp"
#include "Game/Util/HashUtil.hpp"
#include <JSystem/JSupport/JSUInputStream.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JSupport/JSUMemoryOutputStream.hpp>

BinaryDataContentHeaderSerializer::BinaryDataContentHeaderSerializer(u8* pData, u32 dataSize)
    : mStream(pData, dataSize), mAttributeNum(0), mDataSize(0) {
    u16 numAttributes = 0;
    mStream.writeBig(numAttributes);
    u16 streamDataSize = 0;
    mStream.writeBig(streamDataSize);
}

void BinaryDataContentHeaderSerializer::addAttribute(const char* pName, u32 attributeSize) {
    u16 hash = MR::getHashCode(pName);
    mStream.writeBig(hash);
    u16 offset = mDataSize;
    mStream.writeBig(offset);
    mDataSize += attributeSize;
    mAttributeNum++;
}

void BinaryDataContentHeaderSerializer::flush() {
    s32 restorePos = mStream.getPosition();
    mStream.seek(0, SEEK_FROM_START);
    u16 attributeNum = mAttributeNum;
    mStream.writeBig(attributeNum);
    u16 dataSize = mDataSize;
    mStream.writeBig(dataSize);
    mStream.seek(restorePos, SEEK_FROM_START);
}

u32 BinaryDataContentHeaderSerializer::getHeaderSize() const {
    return mStream.getPosition();
}

u32 BinaryDataContentHeaderSerializer::getDataSize() const {
    return mDataSize;
}

BinaryDataContentAccessor::BinaryDataContentAccessor(u8* pData) : mData(pData) {
}

s32 BinaryDataContentAccessor::getHeaderSize() const {
    return aurora::endian::read_u16(mData) * 4 + 4;
}

s32 BinaryDataContentAccessor::getDataSize() const {
    return aurora::endian::read_u16(mData + 2);
}

// stripped
s32 BinaryDataContentAccessor::getAttributeNum() const {
    return aurora::endian::read_u16(mData);
}

void* BinaryDataContentAccessor::getPointer(const char* pAttributeName, u8* pData) const {
    JSUMemoryInputStream inStream = JSUMemoryInputStream(mData, getHeaderSize());
    inStream.skip(4);

    for (s32 idx = 0; idx < getAttributeNum(); idx++) {
        u16 hash = inStream.readBig< u16 >();
        u16 position = inStream.readBig< u16 >();
        if (hash == static_cast< u16 >(MR::getHashCode(pAttributeName))) {
            return pData + position;
        }
    }

    return nullptr;
}

bool BinaryDataContentAccessor::validate(const u8* pData, u32 size, u32 recordCount, std::span< const Attribute > attributes) {
    if (pData == nullptr || size < 4)
        return false;
    BinaryDataContentAccessor accessor(const_cast< u8* >(pData));
    const u32 headerSize = accessor.getHeaderSize();
    const u32 recordSize = accessor.getDataSize();
    if (headerSize > size || (recordCount != 0 && recordSize == 0) || u64(recordCount) * recordSize > size - headerSize)
        return false;
    for (s32 idx = 0; idx < accessor.getAttributeNum(); ++idx) {
        if (aurora::endian::read_u16(pData + 6 + idx * 4) > recordSize)
            return false;
    }
    // The accessor deliberately uses the first matching descriptor, including
    // files written by versions with additional or reordered attributes.
    auto offset = [&](const Attribute& field) -> u32 {
        for (s32 idx = 0; idx < accessor.getAttributeNum(); ++idx) {
            if (aurora::endian::read_u16(pData + 4 + idx * 4) == static_cast< u16 >(MR::getHashCode(field.mName)))
                return aurora::endian::read_u16(pData + 6 + idx * 4);
        }
        return 0xffffffffU;
    };
    for (unsigned idx = 0; idx < attributes.size(); ++idx) {
        const auto begin = offset(attributes[idx]);
        if (begin == 0xffffffffU) {
            if (attributes[idx].mRequired && recordCount != 0)
                return false;
            continue;
        }
        if (begin > recordSize || attributes[idx].mSize > recordSize - begin)
            return false;
        for (unsigned other = 0; other < idx; ++other) {
            const auto otherBegin = offset(attributes[other]);
            if (otherBegin != 0xffffffffU && begin < otherBegin + attributes[other].mSize && otherBegin < begin + attributes[idx].mSize)
                return false;
        }
    }
    return true;
}
