#include "Game/System/BinaryDataContentAccessor.hpp"

#include "compat/SaveDataEndian.hpp"
#include "Game/Util/HashUtil.hpp"

BinaryDataContentHeaderSerializer::BinaryDataContentHeaderSerializer(u8 *pData, u32 dataSize)
    : mBuffer(pData), mCapacity(dataSize), mPosition(0U), mAttributeNum(0U), mDataSize(0U) {
    if (mCapacity >= 4U) {
        SaveDataEndian::write_u16(mBuffer, 0U);
        SaveDataEndian::write_u16(mBuffer + 2U, 0U);
        mPosition = 4U;
    }
}

void BinaryDataContentHeaderSerializer::addAttribute(const char *pName, u32 attributeSize) {
    if (mBuffer == nullptr || mPosition + 4U > mCapacity) {
        return;
    }

    SaveDataEndian::write_u16(mBuffer + mPosition, static_cast<u16>(MR::getHashCode(pName)));
    SaveDataEndian::write_u16(mBuffer + mPosition + 2U, static_cast<u16>(mDataSize));
    mPosition += 4U;
    mDataSize += attributeSize;
    ++mAttributeNum;
}

void BinaryDataContentHeaderSerializer::flush() {
    if (mBuffer == nullptr || mCapacity < 4U) {
        return;
    }

    SaveDataEndian::write_u16(mBuffer, static_cast<u16>(mAttributeNum));
    SaveDataEndian::write_u16(mBuffer + 2U, static_cast<u16>(mDataSize));
}

u32 BinaryDataContentHeaderSerializer::getHeaderSize() const {
    return mPosition;
}

u32 BinaryDataContentHeaderSerializer::getDataSize() const {
    return mDataSize;
}

void *BinaryDataContentHeaderSerializer::getBuffer() const {
    return mBuffer;
}

BinaryDataContentAccessor::BinaryDataContentAccessor(u8 *pData)
    : mData(pData) {
}

s32 BinaryDataContentAccessor::getHeaderSize() const {
    if (mData == nullptr) {
        return 0;
    }
    return static_cast<s32>(SaveDataEndian::read_u16(mData) * 4U + 4U);
}

s32 BinaryDataContentAccessor::getDataSize() const {
    if (mData == nullptr) {
        return 0;
    }
    return SaveDataEndian::read_u16(mData + 2U);
}

s32 BinaryDataContentAccessor::getAttributeNum() const {
    if (mData == nullptr) {
        return 0;
    }
    return SaveDataEndian::read_u16(mData);
}

void *BinaryDataContentAccessor::getPointer(const char *pAttributeName, u8 *pData) const {
    if (mData == nullptr || pData == nullptr) {
        return nullptr;
    }

    const u16 needle = static_cast<u16>(MR::getHashCode(pAttributeName));
    for (s32 idx = 0; idx < getAttributeNum(); ++idx) {
        const auto offset = static_cast<u32>(4 + idx * 4);
        const u16 hash = SaveDataEndian::read_u16(mData + offset);
        const u16 position = SaveDataEndian::read_u16(mData + offset + 2U);
        if (hash == needle) {
            return pData + position;
        }
    }

    return nullptr;
}
