#include "Game/System/BinaryDataChunkHolder.hpp"

#include "compat/SaveDataEndian.hpp"
#include "Game/Util/MemoryUtil.hpp"

BinaryDataChunkHolder::BinaryDataChunkHolder(u32 bufferSize, int maxChunks)
    : mChunks(nullptr), mMaxChunks(maxChunks), mNumChunks(0), mData(nullptr), mBufferSize(bufferSize) {
    mData = new u8[mBufferSize];
    MR::zeroMemory(mData, mBufferSize);
    mChunks = new BinaryDataChunkBase *[maxChunks];
}

BinaryDataChunkHolder::~BinaryDataChunkHolder() {
    delete[] static_cast<u8 *>(mData);
    delete[] mChunks;
}

void BinaryDataChunkHolder::addChunk(BinaryDataChunkBase *pChunk) {
    if (mNumChunks >= mMaxChunks) {
        return;
    }

    mChunks[mNumChunks] = pChunk;
    ++mNumChunks;
}

u32 BinaryDataChunkHolder::makeFileBinary(u8 *pData, u32 size) {
    if (pData == nullptr || size < 4U) {
        return 0U;
    }

    s32 offset = 0;
    MR::zeroMemory(pData, 4U);
    pData[0] = 1U;
    pData[1] = static_cast<u8>(mNumChunks);
    offset += 4;

    for (s32 idx = 0; idx < mNumChunks; ++idx) {
        MR::zeroMemory(mData, mBufferSize);
        auto *chunkData = static_cast<BinaryDataChunkHolderChunkData *>(mData);
        makeChunkData(chunkData, mBufferSize, mChunks[idx]);
        const auto chunkSize = SaveDataEndian::read_u32(static_cast<u8 *>(mData) + 8U);

        if (offset + static_cast<s32>(chunkSize) > static_cast<s32>(size)) {
            return static_cast<u32>(offset);
        }
        MR::copyMemory(&pData[offset], chunkData, chunkSize);
        offset += static_cast<s32>(chunkSize);
    }

    return static_cast<u32>(offset);
}

bool BinaryDataChunkHolder::loadFromFileBinary(const u8 *pData, u32 dataSize) {
    if (pData == nullptr || dataSize < 4U) {
        return false;
    }
    if (dataSize < calcBinarySize(pData)) {
        return false;
    }

    s32 offset = 4;
    bool hashError = false;
    bool deserializeError = false;

    for (s32 idx = 0; idx < pData[1]; ++idx) {
        const auto *chunkData = &pData[offset];
        const auto signature = SaveDataEndian::read_u32(chunkData);
        const auto hash = SaveDataEndian::read_u32(chunkData + 4U);
        const auto chunkSize = SaveDataEndian::read_u32(chunkData + 8U);
        BinaryDataChunkBase *chunk = findFromSignature(signature);

        if (chunk != nullptr) {
            if (hash != chunk->makeHeaderHashCode()) {
                hashError = true;
            }

            const s32 status = chunk->deserialize(chunkData + 0x0CU, chunkSize - 0x0CU);
            deserializeError |= !(status == 0 || status == 1);
        }
        offset += static_cast<s32>(chunkSize);
    }

    return !hashError && !deserializeError && dataSize >= static_cast<u32>(offset);
}

void BinaryDataChunkHolder::makeChunkData(BinaryDataChunkHolderChunkData *pData, u32 bufferSize, const BinaryDataChunkBase *pChunk) {
    auto *bytes = reinterpret_cast<u8 *>(pData);
    SaveDataEndian::write_u32(bytes, pChunk->getSignature());
    SaveDataEndian::write_u32(bytes + 4U, pChunk->makeHeaderHashCode());
    const auto chunkSize = static_cast<u32>(pChunk->serialize(bytes + 0x0CU, bufferSize - 0x0CU)) + 0x0CU;
    SaveDataEndian::write_u32(bytes + 8U, chunkSize);
}

BinaryDataChunkBase *BinaryDataChunkHolder::findFromSignature(u32 signature) const {
    for (s32 idx = 0; idx < mNumChunks; ++idx) {
        if (mChunks[idx]->getSignature() == signature) {
            return mChunks[idx];
        }
    }
    return nullptr;
}

u32 BinaryDataChunkHolder::calcBinarySize(const u8 *pData) {
    if (pData == nullptr) {
        return 0U;
    }

    u32 size = 4U;
    for (s32 idx = 0; idx < pData[1]; ++idx) {
        size += SaveDataEndian::read_u32(&pData[size + 8U]);
    }
    return size;
}
